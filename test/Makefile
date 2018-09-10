all: json_test

json_test: main.o
	gcc -o json_test ../main.c -L./ ../json_parser.dll

main.o: ../main.c ../json.h ../minunit.h
	gcc -std=c11 -pedantic -Wall -Wextra -c ../main.c