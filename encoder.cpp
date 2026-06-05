// clauses_fomation.cpp
// Create DIMACS CNF clauses for a Sudoku board (for use with kissat)
//
// How to use:
// 1. Compile:
//    g++ -std=c++17 -O2 -o clauses_fomation clauses_fomation.cpp
// 2. Prepare a solved or partially-filled Sudoku file named `sudoku.txt` in the
//    current directory. Each row should be one line with N numbers separated
//    by spaces (use `0` for empty cells). Example for 9x9:
//      0 0 3 0 2 0 6 0 0
//      9 0 0 3 0 5 0 0 1
//      ...
// 3. Run the program to generate a DIMACS CNF file `clauses.cnf`:
//    ./clauses_fomation
// 4. Solve with kissat:
//    kissat clauses.cnf
//    - If satisfiable, kissat prints `s SATISFIABLE` and a satisfying assignment.
//    - To print the model to a file: `kissat -m model.txt clauses.cnf`.
// 5. Interpret model: positive integers correspond to true variables; each
//    variable encodes (row, column, value) as: var = row*N*N + col*N + val

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>
#include <cctype>

using namespace std;

vector<vector<int>> clauseList;
vector<vector<int>> board;

static bool parseSudokuRow(const string& line, vector<int>& row) {
    string digits;
    auto flushDigits = [&]() {
        if (!digits.empty()) {
            row.push_back(stoi(digits));
            digits.clear();
        }
    };
    for (char ch : line) {
        if (isdigit(static_cast<unsigned char>(ch))) {
            digits.push_back(ch);
        } else if (ch == '.') {
            flushDigits();
            row.push_back(0);
        } else {
            flushDigits();
        }
    }
    flushDigits();
    return !row.empty();
}

static bool isPerfectSquare(int x) {
    int r = static_cast<int>(sqrt(x));
    return r * r == x;
}

void writeClausesToFile(const string& filename) {
    // compute number of variables (N^3) and clauses
    if (board.empty()) {
        cerr << "Board is empty; cannot write clauses." << endl;
        return;
    }
    int N = board.size();
    long numVars = 1L * N * N * N;
    long numClauses = clauseList.size();
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open " << filename << " for writing." << endl;
        return;
    }
    // DIMACS header
    file << "p cnf " << numVars << " " << numClauses << "\n";
    for (const auto& clause : clauseList) {
        for (const auto& literal : clause) {
            file << literal << " ";
        }
        file << "0\n"; // End of clause
    }
    file.close();
    cout << "Wrote " << numClauses << " clauses to " << filename << " (" << numVars << " variables)\n";
}

void generate1(){
    // For each cell (i, j), at least one number must be placed
    int N = board.size(); 
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            vector<int> clause;
            for (int k = 1; k <= N; k++){
                clause.push_back(i * N * N + j * N + k);
            }
            clauseList.push_back(clause);
        }
    }
}

void generate2(){
    // For each cell (i, j), at most one number can be placed
    int N = board.size(); 
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            for (int k1 = 1; k1 <= N; k1++){
                for (int k2 = k1 + 1; k2 <= N; k2++){
                    clauseList.push_back({-(i * N * N + j * N + k1), -(i * N * N + j * N + k2)});
                }
            }
        }
    }
}
 

void generate3(){
    // For each number k, at least one occurrence in each row
    int N = board.size(); 
    for (int k = 1; k <= N; k++){
        for (int i = 0; i < N; i++){
            vector<int> clause;
            for (int j = 0; j < N; j++){
                clause.push_back(i * N * N + j * N + k);
            }
            clauseList.push_back(clause);
        }
    }
}

void generate4(){
    // For each number k, at least one occurrence in each column
    int N = board.size(); 
    for (int k = 1; k <= N; k++){
        for (int j = 0; j < N; j++){
            vector<int> clause;
            for (int i = 0; i < N; i++){
                clause.push_back(i * N * N + j * N + k);
            }
            clauseList.push_back(clause);
        }
    }
}

void generate5(){
    // For each number k, at least one occurrence in each n*n subgrid
    int N = board.size(); 
    int n = sqrt(N);
    for (int k = 1; k <= N; k++){
        for (int blockRow = 0; blockRow < n; blockRow++){
            for (int blockCol = 0; blockCol < n; blockCol++){
                vector<int> clause;
                for (int i = 0; i < n; i++){
                    for (int j = 0; j < n; j++){
                        int row = blockRow * n + i;
                        int col = blockCol * n + j;
                        clause.push_back(row * N * N + col * N + k);
                    }
                }
                clauseList.push_back(clause);
            }
        }
    }
}

void generate6(){
    // for each number k, it must appear at most once in each row
    int N = board.size(); 
    for (int k = 1; k <= N; k++){
        for (int i = 0; i < N; i++){
            for (int j1 = 0; j1 < N; j1++){
                for (int j2 = j1 + 1; j2 < N; j2++){
                    clauseList.push_back({-(i * N * N + j1 * N + k), -(i * N * N + j2 * N + k)});
                }
            }
        }
    }
}

void generate7(){
    // for each number k, it must appear at most once in each column
    int N = board.size(); 
    for (int k = 1; k <= N; k++){
        for (int j = 0; j < N; j++){
            for (int i1 = 0; i1 < N; i1++){
                for (int i2 = i1 + 1; i2 < N; i2++){
                    clauseList.push_back({-(i1 * N * N + j * N + k), -(i2 * N * N + j * N + k)});
                }
            }
        }
    }
}

void generate8(){
    // for each number k, it must appear at most once in each n* n subgrid
    int N = board.size(); 
    int n = sqrt(N);
    for (int k = 1; k <= N; k++){
        for (int blockRow = 0; blockRow < n; blockRow++){
            for (int blockCol = 0; blockCol < n; blockCol++){
                vector<pair<int, int>> cells;
                for (int i = 0; i < n; i++){
                    for (int j = 0; j < n; j++){
                        int row = blockRow * n + i;
                        int col = blockCol * n + j;
                        cells.emplace_back(row, col);
                    }
                }
                for (size_t idx1 = 0; idx1 < cells.size(); idx1++){
                    for (size_t idx2 = idx1 + 1; idx2 < cells.size(); idx2++){
                        int row1 = cells[idx1].first;
                        int col1 = cells[idx1].second;
                        int row2 = cells[idx2].first;
                        int col2 = cells[idx2].second;
                        clauseList.push_back({-(row1 * N * N + col1 * N + k), -(row2 * N * N + col2 * N + k)});
                    }
                }
            }
        }
    }
}

void generate9(){
    // for each pre-filled cell (i, j) with number k, add a clause that states that this cell must be k
    int N = board.size(); 
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            int num = board[i][j];
            if (num != 0){
                clauseList.push_back({i * N * N + j * N + num});
            }
        }
    }
}

void generateClauses(){
    generate9();  // Pre-filled cells
    generate1();  // At least one digit per cell
    generate2();  // At most one digit per cell
    generate3();  // At least one occurrence of digit k in row
    generate4();  // At least one occurrence of digit k in column
    generate5();  // At least one occurrence of digit k in box
    generate6();  // At most one occurrence of digit k in row
    generate7();  // At most one occurrence of digit k in column
    generate8();  // At most one occurrence of digit k in box
}

int main(){
    // read sudoku.txt
    ifstream file("sudoku.txt");
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }
    string line;
    while (getline(file, line)) {
        vector<int> row;
        parseSudokuRow(line, row);
        if (!row.empty()) board.push_back(row);
    }
    file.close();

    if (board.empty()) {
        cerr << "Input sudoku.txt is empty." << endl;
        return 1;
    }
    int N = static_cast<int>(board.size());
    for (const auto& row : board) {
        if (static_cast<int>(row.size()) != N) {
            cerr << "Invalid board: must be square (N rows and N columns)." << endl;
            return 1;
        }
    }
    if (!isPerfectSquare(N)) {
        cerr << "Invalid board size: N must be a perfect square (e.g., 4, 9, 16)." << endl;
        return 1;
    }

    // generate clauses and write DIMACS file
    generateClauses();
    writeClausesToFile("clauses.cnf");
    cout << "Now run: kissat clauses.cnf" << endl;
    return 0;
}