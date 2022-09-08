CC=gcc
FLAGCS=-Wall -g
SRC=main.c
OBJFILES= stack.o main.o
TARGET=stacktest


target: main.o Graph.o ST.o PQ.o Position.o Astar_utility.o Astar_seq.o Astar_sas_b.o Astar_sas_sf.o Astar_sas_sf_v2.o Dijkstra_seq.o  Astar_ab_ba.o Astar_mp.o
	gcc -Wall -g -o ./build/graphtest ./build/main.o ./build/Graph.o ./build/ST.o ./build/PQ.o ./build/Position.o ./build/Astar_utility.o ./build/Astar_seq.o ./build/Astar_ab_ba.o ./build/Astar_sas_b.o ./build/Astar_sas_sf.o ./build/Astar_sas_sf_v2.o ./build/Astar_mp.o ./build/Dijkstra_seq.o -lm -lpthread

main.o: main.c ./Graph/Graph.h
	gcc -Wall -g -c main.c -o ./build/main.o

Graph.o: ./Graph/Graph.c ./Graph/Graph.h ./Graph/ST.h
	gcc -Wall -g -c ./Graph/Graph.c -o ./build/Graph.o

ST.o: ./Graph/ST.c ./Graph/ST.h ./Graph/Position.h
	gcc -Wall -g -c ./Graph/ST.c -o ./build/ST.o 

PQ.o: ./Graph/PQ.c ./Graph/PQ.h
	gcc -Wall -g -c ./Graph/PQ.c -o ./build/PQ.o 

Position.o: ./Graph/Position.c ./Graph/Position.h
	gcc -Wall -g -c ./Graph/Position.c -o ./build/Position.o -lm

Astar_utility.o: ./AstarEngine/Astar_utility.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_utility.c -o ./build/Astar_utility.o

Astar_seq.o: ./AstarEngine/Astar_seq.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_seq.c -o ./build/Astar_seq.o

Astar_sas_b.o: ./AstarEngine/Astar_sas_b.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_sas_b.c -o ./build/Astar_sas_b.o -lpthread

Astar_sas_sf.o: ./AstarEngine/Astar_sas_sf.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_sas_sf.c -o ./build/Astar_sas_sf.o -lpthread

Astar_sas_sf_v2.o: ./AstarEngine/Astar_sas_sf_v2.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_sas_sf_v2.c -o ./build/Astar_sas_sf_v2.o -lpthread

Astar_ab_ba.o: ./AstarEngine/Astar_ab_ba.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_ab_ba.c -o ./build/Astar_ab_ba.o -lpthread

Astar_mp.o: ./AstarEngine/Astar_mp.c ./AstarEngine/Astar.h 
	gcc -Wall -g -c ./AstarEngine/Astar_mp.c -o ./build/Astar_mp.o -lpthread

Dijkstra_seq.o: ./DijkstraEngine/Dijkstra_seq.c ./DijkstraEngine/Dijkstra.h 
	gcc -Wall -g -c ./DijkstraEngine/Dijkstra_seq.c -o ./build/Dijkstra_seq.o


clean:
	rm -rf ./build/*
