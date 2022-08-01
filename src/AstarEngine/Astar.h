#ifndef ASTAR_H
#define ASTAR_H
#define DEBUG_ASTAR 0
#include "../Graph/Graph.h"
#include "../Graph/Position.h"
#include "../Graph/PQ.h"
void ASTARshortest_path_sequential(Graph G, int source, int dest);
void ASTARshortest_path_sas_b(Graph G, int source, int dest, int num_threads);
void ASTARshortest_path_sas_sf(Graph G, int source, int dest, int num_threads);
void ASTARshortest_path_fa(Graph G, int source, int dest, int num_threads);
void ASTARshortest_path_mp(Graph G, int source, int dest,
                                    int num_threads);
// Utility functions
double heuristic_euclidean(Position source, Position dest);
double compute_f(double h, double g);
int hash_function(int index, int Nthread);
#endif
