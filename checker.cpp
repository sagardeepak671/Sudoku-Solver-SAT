#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>
using namespace std;

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

// Usage: ./checker [solved_sudoku.txt]
// Default file: solved_sudoku.txt

int main(int argc, char** argv) {
	string path = (argc > 1) ? argv[1] : "solved_sudoku.txt";
	ifstream in(path);
	if (!in) {
		cerr << "Failed to open " << path << "\n";
		return 1;
	}
	vector<vector<int>> board;
	string line;
	while (getline(in, line)) {
		if (line.empty()) continue;
		stringstream ss(line);
		vector<int> row;
		int x;
		while (ss >> x) row.push_back(x);
		if (!row.empty()) board.push_back(row);
	}
	in.close();
	if (board.empty()) {
		cout << "Invalid: empty board\n";
		return 1;
	}
	int N = (int)board.size();
	for (auto &r : board) if ((int)r.size() != N) { cout << "Invalid: board not square\n"; return 1; }
	int n = (int)round(sqrt(N));
	if (n * n != N) { cout << "Invalid: board size not a perfect square\n"; return 1; }

	// append verified board to board_view.txt (no ANSI codes in file)
	{
		ofstream fout("board_view.txt", ios::app);
		fout << "\n=== Checker: Verified Solution ===\n";
		printBoard(fout, board, true, false);
	}

	vector<int> seen(N+1);
	// check rows
	for (int i = 0; i < N; ++i) {
		fill(seen.begin(), seen.end(), 0);
		for (int j = 0; j < N; ++j) {
			int v = board[i][j];
			if (v < 1 || v > N) { cout << "Invalid: value out of range at ("<<i<<","<<j<<")\n"; return 1; }
			if (seen[v]) { cout << "Invalid: duplicate in row "<<i<<"\n"; return 1; }
			seen[v] = 1;
		}
	}
	// check cols
	for (int j = 0; j < N; ++j) {
		fill(seen.begin(), seen.end(), 0);
		for (int i = 0; i < N; ++i) {
			int v = board[i][j];
			if (seen[v]) { cout << "Invalid: duplicate in column "<<j<<"\n"; return 1; }
			seen[v] = 1;
		}
	}
	// check boxes
	for (int br = 0; br < n; ++br) {
		for (int bc = 0; bc < n; ++bc) {
			fill(seen.begin(), seen.end(), 0);
			for (int i = 0; i < n; ++i) {
				for (int j = 0; j < n; ++j) {
					int r = br*n + i;
					int c = bc*n + j;
					int v = board[r][c];
					if (seen[v]) { cout << "Invalid: duplicate in block "<<br<<","<<bc<<"\n"; return 1; }
					seen[v] = 1;
				}
			}
		}
	}
	cout << "✅ Valid: solved Sudoku" << "\n";
	return 0;
}