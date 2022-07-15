#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#include "Astar.h"
#define MAXC 11

int main(int argc, char* argv[]) {
  Graph G;
  FILE *fin;

  fin = fopen(argv[1], "r");
  if (fin == NULL)
    exit(-1);
  G = GRAPHload(fin);
  GRAPHstore(G, stdout);
  // GRAPHspD(G, 0);
  // ASTARshortest_path(G, 0, 1);
  ASTARshortest_path(G, 2, 0);
  // TODO: add check on input nodes
  GRAPHfree(G);
  return 0;
}