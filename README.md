# Sudoku-SAT

Solve Sudoku by not writing a Sudoku solver.

This toolchain encodes an `N × N` Sudoku into **DIMACS CNF**, hands it to a modern CDCL SAT solver
([kissat](https://github.com/arminbiere/kissat)), and decodes the satisfying assignment back into a
filled grid. It scales well past 9×9 — 16×16 and 25×25 work — because the encoder picks a different
**at-most-one** encoding depending on the board size.

Five small C++17 programs, a `make run` that does everything, and a timing report at the end.

---

## Table of Contents

- [Pipeline](#pipeline)
- [Environment Setup](#environment-setup)
- [Build](#build)
- [Quick Start](#quick-start)
- [Running Step by Step](#running-step-by-step)
- [Input and Output Formats](#input-and-output-formats)
- [The CNF Encoding](#the-cnf-encoding)
- [Project Structure](#project-structure)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)

---

## Pipeline

```
  ./generate n  ──▶  sudoku.txt
                         │
                         ▼
                    ./encoder   ──▶  clauses.cnf   (DIMACS)
                                          │
                                          ▼
                                      kissat       ──▶  sat_output.txt
                                                              │
                                                              ▼
       board_view.txt  ◀──  ./checker  ◀── solved_sudoku.txt  ◀── ./decoder
```

`./runner` drives all five stages and prints a per-stage timing table.

---

## Environment Setup

| Requirement | Purpose |
|---|---|
| g++ / clang++ with C++17 | Builds the five tools |
| GNU Make | Build driver |
| **kissat** (or any DIMACS solver) | Solves the CNF |

<details open>
<summary><b>Linux (Debian / Ubuntu)</b></summary>

```bash
sudo apt update
sudo apt install -y build-essential git

# Build kissat from source
git clone https://github.com/arminbiere/kissat.git /tmp/kissat
cd /tmp/kissat && ./configure && make -j
sudo cp build/kissat /usr/local/bin/
```
</details>

<details>
<summary><b>macOS</b></summary>

```bash
xcode-select --install
brew install kissat        # or build from source as above
```

The build flags include `-march=native`. If your compiler rejects it:

```bash
make CXXFLAGS="-std=c++17 -O3 -DNDEBUG"
```
</details>

<details>
<summary><b>Windows</b></summary>

Use **WSL2** and follow the Linux instructions.
</details>

Any DIMACS-compatible solver works — MiniSat, CaDiCaL, Glucose. Substitute it in the
`kissat` invocation, or edit the command in `runner.cpp`.

Verify:

```bash
g++ --version && kissat --version
```

---

## Build

```bash
git clone https://github.com/<your-username>/sudoku-SAT.git
cd sudoku-SAT

make            # builds generate, encoder, decoder, checker, runner
```

Flags: `-std=c++17 -O3 -march=native -funroll-loops -fomit-frame-pointer -DNDEBUG`.

| Target | Effect |
|---|---|
| `make` | Build all five binaries |
| `make run` | Build, then run the complete pipeline via `./runner` |
| `make clean` | Remove binaries and all generated files |

---

## Quick Start

```bash
make run        # prompts for n; enter 3 for a 9×9 board
```

Or non-interactively:

```bash
./runner 3      # 9×9
./runner 4      # 16×16
./runner 5      # 25×25
```

The report looks like:

```
=== Sudoku Pipeline  (N = 9x9) ===

--- Generate ---        OK
--- Encode ---          OK
--- Solve (kissat) ---  SATISFIABLE
--- Decode ---          OK
--- Check ---           OK

  Step               Time        Status
  ────────────────────────────────────────
  Generate           12 ms       OK
  Encode              8 ms       OK
  Solve (kissat)     31 ms       SATISFIABLE
  Decode              3 ms       OK
  Check               2 ms       OK
```

`runner` interprets the solver's exit code correctly: **10 = SATISFIABLE**, **20 = UNSATISFIABLE**.

---

## Running Step by Step

Use this when you want to solve *your own* puzzle rather than a generated one.

```bash
# 1. Put your puzzle in sudoku.txt (see format below)

# 2. Encode
./encoder                       # writes clauses.cnf

# 3. Solve
kissat clauses.cnf > sat_output.txt

# 4. Decode
./decoder                       # clauses.cnf + sat_output.txt -> solved_sudoku.txt

# 5. Verify
./checker                       # validates solved_sudoku.txt
```

`decoder` and `checker` accept explicit paths:

```bash
./decoder clauses.cnf sat_output.txt solved_sudoku.txt
./checker solved_sudoku.txt
```

`runner` invokes kissat with tuned options —
`--sat --walkinitially=true --tier1=2 --tier2=5` — which help on satisfiable instances.

---

## Input and Output Formats

### `sudoku.txt` — the puzzle

One row per line, `N` space-separated integers, `0` for an empty cell. `N` must be a perfect square
(4, 9, 16, 25, …).

```
0 0 3 0 2 0 6 0 0
9 0 0 3 0 5 0 0 1
0 0 1 8 0 6 4 0 0
0 0 8 1 0 2 9 0 0
7 0 0 0 0 0 0 0 8
0 0 6 7 0 8 2 0 0
0 0 2 6 0 9 5 0 0
8 0 0 2 0 3 0 0 9
0 0 5 0 1 0 3 0 0
```

The encoder validates squareness and exits with an error on malformed input.

### `clauses.cnf` — the CNF

Standard DIMACS, with a comment header recording the board size so the decoder can recover it:

```
c SUDOKU_N=9
p cnf <vars> <clauses>
```

### `solved_sudoku.txt` and `board_view.txt`

The former is the plain grid; the latter is a Unicode box-drawing rendering with the clues
distinguishable from the solved cells.

---

## The CNF Encoding

One Boolean variable per (row, column, value) triple:

```
var(i, j, v) = i·N² + j·N + v          (v is 1-indexed)
```

Constraints:

| Constraint | Clauses |
|---|---|
| Every cell has **at least one** value | `(x_{i,j,1} ∨ … ∨ x_{i,j,N})` |
| Every cell has **at most one** value | AMO over the cell's values |
| Each value appears **exactly once** per row, column and box | at-least-one + AMO |
| Given clues | unit clauses, emitted **first** |

### Choosing the at-most-one encoding

AMO is where CNF size is won or lost. The encoder dispatches on `N`:

| Board size | Encoding | Clauses | Auxiliary vars |
|---|---|---|---|
| `N ≤ 9` | **Pairwise** | `O(N²)` | 0 |
| `9 < N ≤ 16` | **Sinz sequential (ladder)** | `O(N)` | `N − 1` |
| `N > 16` | **Commander** (Klieber & Kwon, 2007) | `O(N)` | `⌈N/g⌉`, recursive |

Pairwise is optimal when `N` is small — no auxiliary variables at all. The ladder encoding trades
`N−1` auxiliaries for a linear clause count. The commander encoding groups variables, applies
pairwise AMO within each group, introduces one commander per group, and recurses — giving
tree-depth `O(log N)` propagation, which matters at 25×25 and beyond.

### Two propagation-friendly choices

- **Clues first.** Unit clauses for given cells are written before everything else, so the solver
  fixes them by BCP before making a single branching decision.
- **Greedy elimination.** `generateGreedy` explicitly emits the negative unit clauses that each clue
  implies (this cell cannot hold any other value; its peers cannot hold this value). The solver
  would derive them anyway — writing them out lets propagation finish before search starts.

---

## Project Structure

```
.
├── generate.cpp   # 235 lines — random valid-puzzle generator, writes sudoku.txt
├── encoder.cpp    # 439 lines — CNF encoding, three AMO strategies, board rendering
├── decoder.cpp    # 160 lines — parses the model, reconstructs the grid
├── checker.cpp    # 121 lines — independent validity check on the solved grid
├── runner.cpp     # 115 lines — pipeline driver with a timing report
└── Makefile
```

Generated at runtime (all removed by `make clean`): `sudoku.txt`, `clauses.cnf`, `sat_output.txt`,
`solved_sudoku.txt`, `board_view.txt`.

**Reading order:** `encoder.cpp` — `var()`, then `amo()` and its three implementations, then
`generate9` / `generateGreedy`.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `kissat: command not found` | Build kissat from source and put it on `PATH`, or edit the solver command in `runner.cpp` |
| `runner` reports `ERROR (10)` | Not an error — 10 means SATISFIABLE. Only `runner` special-cases this |
| `error: unknown argument '-march=native'` | `make CXXFLAGS="-std=c++17 -O3 -DNDEBUG"` |
| Encoder exits with a size error | `N` must be a perfect square and every row must have exactly `N` entries |
| UNSATISFIABLE on a puzzle you believe is valid | The clues contradict each other — check for duplicates in a row, column or box |
| 25×25 exhausts memory | Expected; clause count grows quickly. Try a solver with a smaller footprint, or increase available RAM |

---

## Contributing

Contributions are welcome. Good first issues:

- Accept an input path argument instead of hard-coding `sudoku.txt`
- Add **exactly-one** encodings and benchmark them against the current at-least-one + AMO pairing
- Emit **UNSAT cores** to explain contradictory clue sets
- Add a test harness with known puzzles and expected solutions
- Support puzzle variants (X-Sudoku, hyper-Sudoku) as additional constraint generators
- Compare CNF size and solve time across the three AMO encodings at each board size

```bash
git checkout -b feat/your-change
make && ./runner 3
git commit -m "feat: describe your change"
```

If you touch the encoding, please include clause/variable counts and solve times before and after.

---

## References

- Klieber & Kwon, *Efficient CNF Encoding for Selecting 1 from N Objects* (2007) — commander encoding
- Sinz, *Towards an Optimal CNF Encoding of Boolean Cardinality Constraints* (2005) — ladder encoding
- [kissat](https://github.com/arminbiere/kissat) — Armin Biere

---

## License

Provided as-is, for educational use.
