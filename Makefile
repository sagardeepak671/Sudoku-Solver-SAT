all: generate encoder decoder checker

generate: generate.cpp
	g++ -std=c++17 -O2 -o generate generate.cpp

encoder: encoder.cpp
	g++ -std=c++17 -O2 -o encoder encoder.cpp

decoder: decoder.cpp
	g++ -std=c++17 -O2 -o decoder decoder.cpp

checker: checker.cpp
	g++ -std=c++17 -O2 -o checker checker.cpp

sudoku.txt: generate
	./generate

clauses.cnf: sudoku.txt encoder
	./encoder

sat_output.txt: clauses.cnf
	-kissat clauses.cnf > sat_output.txt

solved_sudoku.txt: sat_output.txt decoder
	./decoder

run: 
	./generate
	./encoder
	-kissat clauses.cnf > sat_output.txt
	./decoder
	./checker

clean:
	rm -f generate encoder decoder checker clauses.cnf sat_output.txt sudoku.txt solved_sudoku.txt