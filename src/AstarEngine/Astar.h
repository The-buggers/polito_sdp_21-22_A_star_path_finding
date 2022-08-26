#ifndef ASTAR_H
#define ASTAR_H
#define DEBUG_ASTAR 0
#define COLLECT_STAT 1
#include "../Graph/Graph.h"
#include "../Graph/PQ.h"
#include "../Graph/Position.h"
void ASTARshortest_path_sequential(Graph G, int source, int dest,
                                   char heuristic_type);
void ASTARshortest_path_sas_b(Graph G, int source, int dest,
                              char heuristic_type, int num_threads);
void ASTARshortest_path_sas_sf(Graph G, int source, int dest,
                               char heuristic_type, int num_threads);
void ASTARshortest_path_sas_sf_v2(Graph G, int source, int dest,
                                  char heuristic_type, int num_threads);
void ASTARshortest_path_nps(Graph G, int source, int dest, char heuristic_type);
void ASTARshortest_path_phs(Graph G, int source, int dest, char heuristic_type);
void ASTARshortest_path_fa(Graph G, int source, int dest, char heuristic_type,
                           int num_threads);
void ASTARshortest_path_mp(Graph G, int source, int dest, char heuristic_type,
                           int num_threads);
// Utility functions
double heuristic_haversine(Position source, Position dest);
double heuristic(Position p1, Position p2, char heuristic_type);
double compute_f(double h, double g);
int hash_function(int index, int Nthread);
void reconstruct_path(int *parentVertex, int source, int dest,
                      double *costToCome);
#endif
