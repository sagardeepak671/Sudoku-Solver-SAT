#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <cmath>
#include <sstream>
#include <string>
#include <cctype>
#include <iomanip>
using namespace std;

// decoder.cpp
// Usage: ./decoder [clauses.cnf] [sat_output.txt] [solved_sudoku.txt]
// Defaults: clauses.cnf, sat_output.txt, solved_sudoku.txt

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

int main(int argc, char** argv) {
	string cnf_file = (argc > 1) ? argv[1] : "clauses.cnf";
	string sat_file = (argc > 2) ? argv[2] : "sat_output.txt";
	string out_file = (argc > 3) ? argv[3] : "solved_sudoku.txt";

	ifstream cnf(cnf_file);
	if (!cnf) {
		cerr << "Failed to open " << cnf_file << "\n";
		return 1;
	}
	int num_vars = 0;
	int sudoku_N = 0;   // read from "c SUDOKU_N=" comment written by encoder
	string line;
	while (getline(cnf, line)) {
		// check for SUDOKU_N comment (written by encoder when ladder AMO is used)
		if (line.size() >= 11 && line.substr(0, 11) == "c SUDOKU_N=") {
			sudoku_N = stoi(line.substr(11));
		}
		if (line.size() && line[0] == 'p') {
			// p cnf <vars> <clauses>
			stringstream ss(line);
			string p, cnfword;
			ss >> p >> cnfword >> num_vars;
			break;
		}
	}
	cnf.close();
	if (num_vars <= 0) {
		cerr << "Could not detect number of variables in " << cnf_file << "\n";
		return 1;
	}

	// compute N: prefer the SUDOKU_N comment (reliable when ladder AMO adds aux vars)
	// fall back to cbrt for old CNF files that don't have the comment
	int N = sudoku_N;
	if (N <= 0) {
		N = (int)round(cbrt((double)num_vars));
		if ((long)N * N * N != (long)num_vars) {
			cerr << "Unexpected number of variables: " << num_vars
			     << " (not a perfect cube; regenerate with the updated encoder)\n";
			return 1;
		}
	}

	// read sat output and collect positive literals
	ifstream sat(sat_file);
	if (!sat) {
		cerr << "Failed to open " << sat_file << "\n";
		return 1;
	}
	vector<int> positives;
	while (getline(sat, line)) {
		if (line.empty()) continue;
		if (line[0] == 'v' || isspace((unsigned char)line[0]) || line[0]=='+' || line[0]=='-' || isdigit((unsigned char)line[0])) {
			stringstream ss(line);
			string token;
			// skip optional leading 'v'
			if (line[0] == 'v') ss >> token;
			while (ss >> token) {
				try {
					int lit = stoi(token);
					if (lit > 0) positives.push_back(lit);
				} catch(...) { }
			}
		}
	}
	sat.close();

	if (positives.empty()) {
		cerr << "No positive literals found in " << sat_file << "\n";
		return 1;
	}

	// build board
	vector<vector<int>> grid(N, vector<int>(N, 0));
	for (int v : positives) {
		int idx = v - 1;
		int i = idx / (N * N);
		int rem = idx % (N * N);
		int j = rem / N;
		int k = rem % N + 1;
		if (i >= 0 && i < N && j >= 0 && j < N) grid[i][j] = k;
	}

	// append solved board to board_view.txt (no ANSI codes in file)
	{
		ofstream fout("board_view.txt", ios::app);
		fout << "\n=== Decoder: Solved Sudoku ===\n";
		printBoard(fout, grid, true, false);
	}

	// write output
	ofstream out(out_file);
	if (!out) {
		cerr << "Failed to open " << out_file << " for writing\n";
		return 1;
	}
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			out << grid[i][j] << (j+1==N? '\n':' ');
		}
	}
	out.close();
	cout << "Wrote solved board to " << out_file << "\n";
	return 0;
}