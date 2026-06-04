#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>
#include <cctype>
using namespace std;

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
		parseSudokuRow(line, row);
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
	cout << "Valid: solved Sudoku" << "\n";
	return 0;
}