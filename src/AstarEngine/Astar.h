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
void ASTARshortest_path_hda_mp_sm(Graph G, int source, int dest,
                                  char heuristic_type, int num_threads);
void ASTARshortest_path_sas_sf(Graph G, int source, int dest,
                               char heuristic_type, int num_threads);
void ASTARshortest_path_nps(Graph G, int source, int dest, char heuristic_type);
void ASTARshortest_path_phs(Graph G, int source, int dest, char heuristic_type);
void ASTARshortest_path_ab_ba(Graph G, Graph R, int source, int dest,
                              char heuristic_type);
void ASTARshortest_path_hda_mp_mq(Graph G, int source, int dest,
                                  char heuristic_type, int num_threads);
// Utility functions
double heuristic_haversine(Position source, Position dest);
double heuristic(Position p1, Position p2, char heuristic_type);
double compute_f(double h, double g);
int hash_function(int index, int Nthread);
int hash_function2(int index, int Nthread, int V);
void reconstruct_path(int *parentVertex, int source, int dest,
                      double *costToCome);
void reconstruct_path_ab_ba(int *parentVertexG, int *parentVertexR, int source,
                            int comm, int dest, double *costToComeG,
                            double *costToComeR);
#endif
