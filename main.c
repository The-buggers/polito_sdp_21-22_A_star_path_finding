#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Astar.h"
#include "Graph.h"
#define MAXC 11

int main(int argc, char* argv[]) {
    Graph G;
    FILE* fin;
    struct timespec begin, end;
    //G = GRAPHload_sequential(argv[1]);
    //GRAPHstore(G, stdout);
    G = GRAPHload_parallel3(argv[1], 5, 3, 3);
    /*
    fin = fopen(argv[1], "r");
    if (fin == NULL)
      exit(-1);
    G = GRAPHload(fin);
    clock_gettime(CLOCK_REALTIME, &begin);
    ASTARshortest_path(G, 0, 23943);
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("Time measured: %.9f seconds.\n", elapsed);
    // TODO: add check on input nodes
    //GRAPHstore(G, stdout);
    GRAPHfree(G);
    */
    return 0;
}
