// encoder.cpp
// Create DIMACS CNF clauses for a Sudoku board (for use with kissat)
//
// How to use:
// 1. Compile:
//    g++ -std=c++17 -O3 -march=native -funroll-loops -fomit-frame-pointer -DNDEBUG -o encoder encoder.cpp
// 2. Prepare a solved or partially-filled Sudoku file named `sudoku.txt` in the
//    current directory. Each row should be one line with N numbers separated
//    by spaces (use `0` for empty cells). Example for 9x9:
//      0 0 3 0 2 0 6 0 0
//      9 0 0 3 0 5 0 0 1
//      ...
// 3. Run the program to generate a DIMACS CNF file `clauses.cnf`:
//    ./encoder
// 4. Solve with kissat:
//    kissat clauses.cnf
// 5. Interpret model: positive integers in 1..N^3 encode (row, col, value) as:
//    var = row*N*N + col*N + val  (val is 1-indexed)

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>

using namespace std;

// write a sudoku board with unicode box-drawing to any ostream
// useColor=true adds ANSI codes (terminal), false = plain text (files)
void printBoard(ostream& out, const vector<vector<int>>& b, bool solved = false, bool useColor = false) {
    int N = (int)b.size();
    int n = (int)sqrt(N);
    int digits = (int)to_string(N).size();
    int w      = digits + 1;
    int boxW   = n * w + 1;

    auto hline = [&](const char* l, const char* mid, const char* r) {
        out << l;
        for (int bc = 0; bc < n; bc++) {
            for (int x = 0; x < boxW; x++) out << "═";
            if (bc < n - 1) out << mid;
        }
        out << r << "\n";
    };

    hline("╔", "╦", "╗");
    for (int i = 0; i < N; i++) {
        if (i > 0 && i % n == 0) hline("╠", "╬", "╣");
        for (int j = 0; j < N; j++) {
            if (j % n == 0) out << "║ ";
            if (useColor && solved) {
                out << "\033[92m" << setw(digits) << b[i][j] << "\033[0m" << " ";
            } else if (useColor && b[i][j] == 0) {
                out << "\033[2m"  << setw(digits) << "." << "\033[0m" << " ";
            } else if (useColor) {
                out << "\033[93;1m" << setw(digits) << b[i][j] << "\033[0m" << " ";
            } else if (b[i][j] == 0) {
                out << setw(digits) << "." << " ";
            } else {
                out << setw(digits) << b[i][j] << " ";
            }
        }
        out << "║\n";
    }
    hline("╚", "╩", "╝");
}

vector<vector<int>> board;
int N, n_blk;
int nextAux;          // next auxiliary variable index (for ladder AMO)
long numClauses;
long headerPos;       // byte offset of the DIMACS header in clauses.cnf
ofstream outFile;
vector<vector<int>> clauseBuf;

const int BATCH_SIZE = 100000;  // flush to disk every this many clauses

static bool isPerfectSquare(int x) {
    int r = (int)sqrt(x);
    return r * r == x;
}

// variable index for cell (i,j) with value k (k is 1-indexed)
inline int var(int i, int j, int k) {
    return i * N * N + j * N + k;
}

// flush clause buffer to file in one shot
void flush() {
    for (const auto& c : clauseBuf) {
        for (int lit : c) outFile << lit << ' ';
        outFile << "0\n";
    }
    numClauses += (long)clauseBuf.size();
    clauseBuf.clear();
}

void addClause(vector<int> c) {
    clauseBuf.push_back(std::move(c));
    if ((int)clauseBuf.size() >= BATCH_SIZE) flush();
}

// ------------------------------------------------------------------
// AMO encodings
// ------------------------------------------------------------------

void amo_pairwise(const vector<int>& vars) {
    int m = (int)vars.size();
    for (int a = 0; a < m; a++)
        for (int b = a + 1; b < m; b++)
            addClause({-vars[a], -vars[b]});
}

// Sinz sequential (ladder) AMO encoding
// Uses O(N) clauses and N-1 auxiliary variables instead of O(N^2) pairwise
// Reference: Sinz 2005 "Towards an Optimal CNF Encoding of Boolean Cardinality Constraints"
void amo_ladder(const vector<int>& vars) {
    int m = (int)vars.size();
    if (m <= 1) return;
    if (m == 2) { addClause({-vars[0], -vars[1]}); return; }

    // s[i] = auxiliary register variable
    vector<int> s(m - 1);
    for (int i = 0; i < m - 1; i++) s[i] = nextAux++;

    // -x[0] | s[0]
    addClause({-vars[0], s[0]});
    for (int i = 1; i < m - 1; i++) {
        addClause({-vars[i],  s[i]});       // -x[i] | s[i]
        addClause({-s[i-1],   s[i]});       // -s[i-1] | s[i]  (chain propagation)
        addClause({-s[i-1],  -vars[i]});    // -s[i-1] | -x[i] (at most one)
    }
    // -s[m-2] | -x[m-1]
    addClause({-s[m-2], -vars[m-1]});
}

// use pairwise for N<=9 (fewer clauses), ladder for N>9 (far fewer clauses)
void amo(const vector<int>& vars) {
    if (N <= 9) amo_pairwise(vars);
    else        amo_ladder(vars);
}

// ------------------------------------------------------------------
// Clause generators
// ------------------------------------------------------------------

void generate9() {
    // unit clues: pre-filled cells must take their given value
    // placed FIRST so the solver fixes all given cells before any branching
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (board[i][j] != 0)
                addClause({var(i, j, board[i][j])});
}

void generateGreedy() {
    // for every given clue, emit unit elimination clauses that the solver
    // would derive anyway via BCP — putting them explicitly lets kissat
    // propagate everything for free before the first branching decision
    vector<bool> negAdded(N * N * N + 1, false);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int k = board[i][j];
            if (k == 0) continue;

            // other values cannot go in this cell
            for (int k2 = 1; k2 <= N; k2++) {
                if (k2 == k) continue;
                int v = var(i, j, k2);
                if (!negAdded[v]) { negAdded[v] = true; addClause({-v}); }
            }
            // k cannot appear elsewhere in this row
            for (int j2 = 0; j2 < N; j2++) {
                if (j2 == j) continue;
                int v = var(i, j2, k);
                if (!negAdded[v]) { negAdded[v] = true; addClause({-v}); }
            }
            // k cannot appear elsewhere in this col
            for (int i2 = 0; i2 < N; i2++) {
                if (i2 == i) continue;
                int v = var(i2, j, k);
                if (!negAdded[v]) { negAdded[v] = true; addClause({-v}); }
            }
            // k cannot appear elsewhere in this box
            int bRow = i / n_blk, bCol = j / n_blk;
            for (int bi = 0; bi < n_blk; bi++) {
                for (int bj = 0; bj < n_blk; bj++) {
                    int r = bRow * n_blk + bi;
                    int c = bCol * n_blk + bj;
                    if (r == i && c == j) continue;
                    int v = var(r, c, k);
                    if (!negAdded[v]) { negAdded[v] = true; addClause({-v}); }
                }
            }
        }
    }
}

void generate1() {
    // each cell must have at least one value (ALO cell)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            vector<int> c;
            for (int k = 1; k <= N; k++) c.push_back(var(i, j, k));
            addClause(c);
        }
}

void generate2() {
    // each cell has at most one value (AMO cell)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            vector<int> vars;
            for (int k = 1; k <= N; k++) vars.push_back(var(i, j, k));
            amo(vars);
        }
}

void generate3() {
    // each value appears at least once per row (ALO row)
    for (int k = 1; k <= N; k++)
        for (int i = 0; i < N; i++) {
            vector<int> c;
            for (int j = 0; j < N; j++) c.push_back(var(i, j, k));
            addClause(c);
        }
}

void generate4() {
    // each value appears at least once per col (ALO col)
    for (int k = 1; k <= N; k++)
        for (int j = 0; j < N; j++) {
            vector<int> c;
            for (int i = 0; i < N; i++) c.push_back(var(i, j, k));
            addClause(c);
        }
}

void generate5() {
    // each value appears at least once per box (ALO box)
    for (int k = 1; k <= N; k++)
        for (int bRow = 0; bRow < n_blk; bRow++)
            for (int bCol = 0; bCol < n_blk; bCol++) {
                vector<int> c;
                for (int i = 0; i < n_blk; i++)
                    for (int j = 0; j < n_blk; j++)
                        c.push_back(var(bRow * n_blk + i, bCol * n_blk + j, k));
                addClause(c);
            }
}

void generate6() {
    // each value appears at most once per row (AMO row)
    for (int k = 1; k <= N; k++)
        for (int i = 0; i < N; i++) {
            vector<int> vars;
            for (int j = 0; j < N; j++) vars.push_back(var(i, j, k));
            amo(vars);
        }
}

void generate7() {
    // each value appears at most once per col (AMO col)
    for (int k = 1; k <= N; k++)
        for (int j = 0; j < N; j++) {
            vector<int> vars;
            for (int i = 0; i < N; i++) vars.push_back(var(i, j, k));
            amo(vars);
        }
}

void generate8() {
    // each value appears at most once per box (AMO box)
    for (int k = 1; k <= N; k++)
        for (int bRow = 0; bRow < n_blk; bRow++)
            for (int bCol = 0; bCol < n_blk; bCol++) {
                vector<int> vars;
                for (int i = 0; i < n_blk; i++)
                    for (int j = 0; j < n_blk; j++)
                        vars.push_back(var(bRow * n_blk + i, bCol * n_blk + j, k));
                amo(vars);
            }
}

void generateClauses() {
    // optimal ordering for fastest SAT propagation:
    generate9();        // 1. unit clues -> solver fixes all given cells immediately (no branching)
    generateGreedy();   // 2. greedy eliminations -> free BCP from every clue
    generate2();        // 3. AMO cell  (binary/ternary ladder clauses)
    generate6();        // 4. AMO row   (binary/ternary ladder clauses)
    generate7();        // 5. AMO col   (binary/ternary ladder clauses)
    generate8();        // 6. AMO box   (binary/ternary ladder clauses)
    generate1();        // 7. ALO cell  (long clauses of size N)
    generate3();        // 8. ALO row   (long clauses of size N)
    generate4();        // 9. ALO col   (long clauses of size N)
    generate5();        // 10. ALO box  (long clauses of size N)
}

int main() {
    ifstream file("sudoku.txt");
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }
    string line;
    while (getline(file, line)) {
        vector<int> row;
        stringstream ss(line);
        int num;
        while (ss >> num) row.push_back(num);
        if (!row.empty()) board.push_back(row);
    }
    file.close();

    if (board.empty()) {
        cerr << "Input sudoku.txt is empty." << endl;
        return 1;
    }
    N = (int)board.size();
    for (const auto& row : board) {
        if ((int)row.size() != N) {
            cerr << "Invalid board: must be square (N rows and N columns)." << endl;
            return 1;
        }
    }
    if (!isPerfectSquare(N)) {
        cerr << "Invalid board size: N must be a perfect square (e.g., 4, 9, 16, 25, 36)." << endl;
        return 1;
    }
    n_blk = (int)sqrt(N);

    // append puzzle view to board_view.txt (no ANSI codes in file)
    {
        ofstream fout("board_view.txt", ios::app);
        fout << "\n=== Encoder: Puzzle Being Solved  (. = empty) ===\n";
        printBoard(fout, board, false, false);
    }

    // aux variables for ladder AMO start right after N^3 base variables
    nextAux = N * N * N + 1;
    numClauses = 0;

    // total variable count:
    //   base: N^3
    //   ladder aux (N>9): 4 AMO groups x N^2 calls x (N-1) aux vars each
    long numVars = (long)N * N * N;
    if (N > 9) numVars += 4LL * N * N * (N - 1);

    outFile.open("clauses.cnf");
    if (!outFile.is_open()) {
        cerr << "Failed to open clauses.cnf for writing." << endl;
        return 1;
    }

    // write N as a comment so decoder.cpp can read it back
    // (needed when ladder encoding adds aux vars and cbrt(numVars) != N)
    outFile << "c SUDOKU_N=" << N << "\n";

    // write a fixed-length placeholder header (always 38 bytes: "p cnf " + 15 + " " + 15 + "\n")
    // we seek back here at the end to fill in the real clause count
    headerPos = (long)outFile.tellp();
    outFile << "p cnf " << setw(15) << 0LL << " " << setw(15) << 0LL << "\n";

    clauseBuf.reserve(BATCH_SIZE + 100);
    generateClauses();
    flush();  // flush any remaining clauses in the buffer

    // seek back to the placeholder and overwrite with real values
    outFile.seekp(headerPos);
    outFile << "p cnf " << setw(15) << numVars << " " << setw(15) << numClauses << "\n";

    outFile.close();
    cout << "Wrote " << numClauses << " clauses to clauses.cnf (" << numVars << " variables)" << endl;
    cout << "Now run: kissat clauses.cnf" << endl;
    return 0;
}