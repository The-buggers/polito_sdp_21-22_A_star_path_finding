#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Graph.h"
#include "Astar.h"
#define MAXC 11

int main(int argc, char* argv[]) {
  Graph G;
  FILE *fin;
  struct timespec begin, end; 
  clock_gettime(CLOCK_REALTIME, &begin);
  fin = fopen(argv[1], "r");
  if (fin == NULL)
    exit(-1);
  G = GRAPHload(fin);
  GRAPHstore(G, stdout);
  // GRAPHspD(G, 0);
  // ASTARshortest_path(G, 0, 1);
  ASTARshortest_path(G, 9, 0);
  clock_gettime(CLOCK_REALTIME, &end);
  long seconds = end.tv_sec - begin.tv_sec;
  long nanoseconds = end.tv_nsec - begin.tv_nsec;
  double elapsed = seconds + nanoseconds*1e-9;
  printf("Time measured: %.3f seconds.\n", elapsed);
  // TODO: add check on input nodes
  GRAPHfree(G);
  return 0;
}
