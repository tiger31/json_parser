all : json_parser

json_parser : json.o
	gcc -pedantic -Wall -Wextra -shared -o json_parser.dll json.o

json.o : ../json.c ../json.h
	gcc -std=c11 -pedantic -Wall -Wextra -c ../json.c

all: json_test

json_test: main.o
	gcc -o json_test ../test/main.c -L./ json_parser.dll

main.o: ../test/main.c ../test/json.h ../test/minunit.h
	gcc -std=c11 -pedantic -c ../test/main.c

clean :
	make clean
