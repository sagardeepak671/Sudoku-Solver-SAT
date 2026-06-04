run: 
	g++ -std=c++17 -O2 -o generate generate.cpp
	./generate
	g++ -std=c++17 -O2 -o encoder encoder.cpp
	./encoder
	-kissat clauses.cnf > sat_output.txt
	g++ -std=c++17 -O2 -o decoder decoder.cpp
	./decoder
	g++ -std=c++17 -O2 -o checker checker.cpp
	./checker

clean:
	rm -f generate encoder decoder checker clauses.cnf sat_output.txt sudoku.txt solved_sudoku.txt