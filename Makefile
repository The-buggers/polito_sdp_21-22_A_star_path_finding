CC=gcc
FLAGCS=-Wall -g
SRC=main.c
OBJFILES= stack.o main.o
TARGET=stacktest

target: main.o Graph.o ST.o PQ.o Position.o Astar.o
	gcc -Wall -g -o ./build/graphtest ./build/main.o ./build/Graph.o ./build/ST.o ./build/PQ.o ./build/Position.o ./build/Astar.o -lm -lpthread

main.o: main.c Graph.h
	gcc -Wall -g -c main.c -o ./build/main.o

Graph.o: Graph.c Graph.h ST.h
	gcc -Wall -g -c Graph.c -o ./build/Graph.o

ST.o: ST.c ST.h Position.h
	gcc -Wall -g -c ST.c -o ./build/ST.o 

PQ.o: PQ.c PQ.h
	gcc -Wall -g -c PQ.c -o ./build/PQ.o 

Position.o: Position.c Position.h
	gcc -Wall -g -c Position.c -o ./build/Position.o -lm

Astar.o: Astar.c Astar.h Position.h Graph.h PQ.h
	gcc -Wall -g -c Astar.c -o ./build/Astar.o

clean:
	rm -rf ./build/*
