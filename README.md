Sudoku-SAT
===========

A small, practical toolset to convert Sudoku puzzles into DIMACS CNF clauses, solve them with a SAT solver (kissat), and interpret the result.

This repository contains a compact encoder and a few helper programs used during research and experimentation. It aims to be simple and direct — nothing fancy, just useful.

Quick overview
--------------

- encoder.cpp  : Create DIMACS CNF clauses from a Sudoku board (sudoku.txt).
- decoder.cpp  : Helpers to interpret a SAT model back into a Sudoku grid.
- generate.cpp : (Optional) utilities for generating puzzles.
- runner.cpp   : (Optional) convenience runner for piping solver output.

Prerequisites
-------------

- A C++17-compatible compiler (tested with g++).  
- kissat or another DIMACS-compatible SAT solver to solve the generated CNF.

How to build
------------

Build the encoder and decoder with optimizations for solving larger puzzles:

```sh
g++ -std=c++17 -O3 -march=native -funroll-loops -fomit-frame-pointer -DNDEBUG -o encoder encoder.cpp
g++ -std=c++17 -O3 -march=native -funroll-loops -fomit-frame-pointer -DNDEBUG -o decoder decoder.cpp
```

How to use
----------

1. Prepare an input file named `sudoku.txt` in the repository root. Each row is one line with N numbers separated by spaces; use 0 for empty cells. N must be a perfect square (for example: 9 for a 9x9 Sudoku).

   Example (9x9):

   ```txt
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

2. Run the encoder to generate a DIMACS CNF file `clauses.cnf`:

```sh
./encoder
```

The encoder writes a short comment header indicating the puzzle size and produces `clauses.cnf` in the current directory.

3. Solve with a DIMACS SAT solver such as kissat:

```sh
kissat clauses.cnf > model.txt
```

4. Interpret the solver output using `decoder` (see `decoder.cpp` for details). Depending on the decoder/runner in this repo, you may need to pass the model file or pipe the solver output. A typical flow is:

```sh
./decoder model.txt > solved.txt
```

Notes and tips
--------------

- Input requirements: the board must be square and its size N must be a perfect square (4, 9, 16, 25, ...). The encoder validates this and will exit with an error if the input is malformed.
- Clause ordering: the encoder emits unit clauses for given clues first so the solver can do a lot of propagation before branching.
- AMO encodings: the encoder uses different at-most-one strategies depending on size (pairwise, Sinz ladder, or commander groups) to keep CNFs reasonably compact and solver-friendly.
- Large puzzles: solving scales quickly in clause count; use sufficient memory and consider solver options when working with big boards (16x16+).

Contributing
-------------

If you want to add features or improve the toolchain, please fork and submit a pull request. Useful improvements include clearer decoder output formats, a small test harness, or a script that runs the entire encode→solve→decode pipeline.

License
-------

This project is provided as-is. There is no formal license included; add one if you plan to distribute the code.

Acknowledgements
-----------------

This encoder implements a few common CNF encodings and pragmatic choices tuned for modern CDCL solvers. It was written to be readable and easy to modify.
