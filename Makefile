CXX      = g++
CXXFLAGS = -std=c++17 -O3 -march=native -funroll-loops -fomit-frame-pointer -DNDEBUG

# build all binaries — only recompiles what changed
all: generate encoder decoder checker runner

generate: generate.cpp
	$(CXX) $(CXXFLAGS) -o generate generate.cpp

encoder: encoder.cpp
	$(CXX) $(CXXFLAGS) -o encoder encoder.cpp

decoder: decoder.cpp
	$(CXX) $(CXXFLAGS) -o decoder decoder.cpp

checker: checker.cpp
	$(CXX) $(CXXFLAGS) -o checker checker.cpp

runner: runner.cpp
	$(CXX) $(CXXFLAGS) -o runner runner.cpp

# run full pipeline via runner (builds only what changed, then runs)
run: all
	./runner

clean:
	rm -f generate encoder decoder checker runner \
	      clauses.cnf sat_output.txt sudoku.txt solved_sudoku.txt board_view.txt