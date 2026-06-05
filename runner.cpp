// runner.cpp
// Runs the full Sudoku pipeline and prints a timing report table.
// Usage: ./runner          (prompts for n)
//        ./runner 3        (n=3 → 9x9, non-interactive)

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <sys/wait.h>
using namespace std;
using namespace chrono;

int main(int argc, char** argv) {
    int n;
    if (argc > 1) {
        n = atoi(argv[1]);
    } else {
        cout << "Enter the size of the Sudoku (n for n*n blocks, e.g. 3 = 9x9): ";
        cin >> n;
    }
    if (n <= 0) { cerr << "Invalid n\n"; return 1; }

    cout << "\n=== Sudoku Pipeline  (N = " << n*n << "x" << n*n << ") ===\n\n";

    struct Step {
        string name;
        string cmd;
        long long ms = 0;
        string status;
    };

    vector<Step> steps = {
        {"Generate",       "./generate " + to_string(n), 0, ""},
        {"Encode",         "./encoder",                   0, ""},
        {"Solve (kissat)", "kissat --sat --walkinitially=true --tier1=2 --tier2=5 clauses.cnf > sat_output.txt", 0, ""},
        {"Decode",         "./decoder",                   0, ""},
        {"Check",          "./checker",                   0, ""},

    };

    long long total_ms = 0;

    for (auto& step : steps) {
        cout << "--- " << step.name << " ---\n";
        auto t1 = steady_clock::now();
        int ret = system(step.cmd.c_str());
        auto t2 = steady_clock::now();
        step.ms = duration_cast<milliseconds>(t2 - t1).count();
        total_ms += step.ms;

        int code = WEXITSTATUS(ret);
        if (step.name.find("kissat") != string::npos) {
            if (code == 10)      step.status = "SATISFIABLE";
            else if (code == 20) step.status = "UNSATISFIABLE";
            else                 step.status = "ERROR (" + to_string(code) + ")";
        } else {
            step.status = (code == 0) ? "OK" : "ERROR (" + to_string(code) + ")";
        }
        cout << "\n";
    }

    // format milliseconds nicely (show ms for <10s, seconds for >=10s)
    auto fmt = [](long long ms) -> string {
        ostringstream oss;
        if (ms < 10000) oss << ms << " ms";
        else            oss << fixed << setprecision(2) << ms / 1000.0 << " s";
        return oss.str();
    };

    // column inner widths (ASCII content only, so setw works correctly)
    const int WN = 18;   // step name
    const int WT =  9;   // time (right-justified, then 1 space)
    const int WS = 14;   // status

    // border helper (each ═ prints as 1 visual char in UTF-8 terminal)
    string bN(WN + 2, '\0');   // "  " + WN chars = WN+2 visual slots
    string bT(WT + 2, '\0');
    string bS(WS + 2, '\0');
    // build border segments as strings of ═ repeated
    string segN, segT, segS;
    for (int i = 0; i < WN + 2; i++) segN += "═";
    for (int i = 0; i < WT + 2; i++) segT += "═";
    for (int i = 0; i < WS + 2; i++) segS += "═";

    auto top_border = [&](const string& l, const string& mid, const string& r) {
        cout << l << segN << mid << segT << mid << segS << r << "\n";
    };

    cout << "\n";
    top_border("╔", "╦", "╗");
    // header
    cout << "║ " << left  << setw(WN) << "Step"
         << " ║ " << right << setw(WT) << "Time"
         << " ║ " << left  << setw(WS) << "Status" << " ║\n";
    top_border("╠", "╬", "╣");

    for (const auto& s : steps) {
        cout << "║ " << left  << setw(WN) << s.name
             << " ║ " << right << setw(WT) << fmt(s.ms)
             << " ║ " << left  << setw(WS) << s.status << " ║\n";
    }

    top_border("╠", "╬", "╣");
    cout << "║ " << left  << setw(WN) << "TOTAL"
         << " ║ " << right << setw(WT) << fmt(total_ms)
         << " ║ " << left  << setw(WS) << "✅" << "  ║\n";
    top_border("╚", "╩", "╝");

    cout << "\nBoard views (puzzle + solution) -> board_view.txt\n";
    return 0;
}
