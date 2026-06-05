/* HOW TO COMPILE AND RUN 
1) g++ -std=c++17 -O3 -march=native -funroll-loops -fomit-frame-pointer -DNDEBUG -o generate generate.cpp
2) ./generate          (interactive)
   ./generate 3        (CLI: n=3 → 9×9 board, used by runner)
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <set>
#include <iomanip>
#include <string>
#include <cmath>
using namespace std;

// write a sudoku board with unicode box-drawing to any ostream
// useColor=true adds ANSI codes (for terminal), false = plain text (for files)
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

class Generator {
public:
    Generator(int len);

    void generate();
    void writeToFile(const string& filename);

private:
    int n;
    int N;
    vector<vector<int>> board;
    mt19937 rng;

    void fillBoard();
    void removeNumbers();
};
Generator::Generator(int len) {
    this->n = len;
    this->N = n * n;
    board.resize(N, vector<int>(N, 0));
    random_device rd;
    rng.seed(rd());
}

void Generator::generate() {
    fillBoard();
    removeNumbers();

    // write both views to board_view.txt (no ANSI in file)
    ofstream fout("board_view.txt");  // create / overwrite for each run
    fout << "=== Generated Full Board ===\n";
    // show full board: we need to temporarily restore removed numbers
    // instead, just show the puzzle and note it in the file header
    fout << "(puzzle view — run decoder to append solved board)\n";
    fout << "\n=== Puzzle to Solve  (. = empty) ===\n";
    printBoard(fout, board, false, false);
    fout.close();

    cout << "Puzzle written to sudoku.txt  |  Board view -> board_view.txt\n";
}

void Generator::writeToFile(const string& filename) {
    ofstream file(filename);
    for (const auto& row : board) {
        for (const auto& num : row) {
            file << num << " ";
        }
        file << '\n';
    }
}
void Generator::fillBoard()
{
    // Step 1: Generate base valid Sudoku
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            board[i][j] = (i * n + i / n + j) % N + 1;
        }
    }

    // Step 2: Randomly relabel digits
    vector<int> mapping(N);
    for (int i = 0; i < N; i++){
        mapping[i] = i + 1;
    }

    shuffle(mapping.begin(), mapping.end(), rng);

    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            board[i][j] = mapping[board[i][j] - 1];
        }
    }

    // Step 3: Apply many random valid transformations
    uniform_int_distribution<int> opDist(0, 4);
    uniform_int_distribution<int> idxDist(0, n - 1);

    for (int iter = 0; iter < 1000; iter++){
        int op = opDist(rng);
        // Swap rows within a band
        if (op == 0){
            int band = idxDist(rng);
            int r1 = band * n + idxDist(rng);
            int r2 = band * n + idxDist(rng);
            swap(board[r1], board[r2]);
        }

        // Swap columns within a stack
        else if (op == 1){
            int stack = idxDist(rng);
            int c1 = stack * n + idxDist(rng);
            int c2 = stack * n + idxDist(rng);
            for (int row = 0; row < N; row++)
                swap(board[row][c1], board[row][c2]);
        }

        // Swap entire row bands
        else if (op == 2){
            int b1 = idxDist(rng);
            int b2 = idxDist(rng);
            if (b1 == b2) continue;
            for (int k = 0; k < n; k++){
                swap(board[b1 * n + k],board[b2 * n + k]);
            }
        }

        // Swap entire column stacks
        else if (op == 3)
        {
            int s1 = idxDist(rng);
            int s2 = idxDist(rng);
            if (s1 == s2) continue;
            for (int row = 0; row < N; row++){
                for (int k = 0; k < n; k++){
                    swap(board[row][s1 * n + k],board[row][s2 * n + k]);
                }
            }
        }

        // Transpose board
        else
        {
            for (int i = 0; i < N; i++){
                for (int j = i + 1; j < N; j++){
                    swap(board[i][j], board[j][i]);
                }
            }
        }
    }
}
void Generator::removeNumbers() {
    
    int totalCells = N*N;
    int percentageToRemove;
    // 30% of times it will be from 0% to 40% OR 60% to 100% of the cells will be removed
    // for 70% of times it will be from 40% to 60% of the cells will be removed 
    uniform_int_distribution<int> dist100(0, 99);
    int randVal = dist100(rng);
    if (randVal < 30) {
        uniform_int_distribution<int> dist41(0, 40);
        uniform_int_distribution<int> dist61(60, 100);
        percentageToRemove = (randVal < 15) ? dist41(rng) : dist61(rng);
    } else {
        uniform_int_distribution<int> dist21(40, 60);
        percentageToRemove = dist21(rng);
    }
    

    int cellsToRemove = (percentageToRemove * totalCells) / 100;
    vector<pair<int, int>> cells;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            cells.emplace_back(i, j);
        }
    }
    shuffle(cells.begin(), cells.end(), rng);
    for (int i = 0; i < cellsToRemove; ++i) {
        int row = cells[i].first;
        int col = cells[i].second;
        board[row][col] = 0; // remove the number   
    }
} 

int main(int argc, char** argv){
    int input;
    if (argc > 1) {
        // called from runner with size as CLI argument
        input = atoi(argv[1]);
    } else {
        cout << "Enter the size of the Sudoku (n for n x n blocks): ";
        cin >> input;
    }
    Generator gen(input);
    gen.generate();
    gen.writeToFile("sudoku.txt");
    return 0;
}
