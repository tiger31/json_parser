all : json_parser

json_parser : json.o
	gcc -pedantic -Wall -Wextra -shared -o json_parser.so json.o

json.o : ../json.c ../json.h
	gcc -std=c11 -pedantic -Wall -Wextra -c ../json.c

clean : json.o