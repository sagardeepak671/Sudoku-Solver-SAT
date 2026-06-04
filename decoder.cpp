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
using namespace std;

// decoder.cpp
// Usage: ./decoder [clauses.cnf] [sat_output.txt] [solved_sudoku.txt]
// Defaults: clauses.cnf, sat_output.txt, solved_sudoku.txt

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
	string line;
	while (getline(cnf, line)) {
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

	// compute N from num_vars = N^3
	int N = round(cbrt((double)num_vars));
	if (N * N * N != num_vars) {
		cerr << "Unexpected number of variables: " << num_vars << " (not a perfect cube)\n";
		return 1;
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