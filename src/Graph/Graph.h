#ifndef GRAPH_H
#define GRAPH_H
#define DEBUGPRINT 0
#define DEBUGPARALLELREAD 0
#include <stdio.h>

#include "ST.h"
typedef struct edge {
    int v;
    int w;
    double wt;
} Edge;
typedef struct graph *Graph;
typedef struct node *link;
Graph GRAPHinit(int V);
void GRAPHfree(Graph G);
void GRAPHstore(Graph G, FILE *fin);
int GRAPHgetIndex(Graph G, char *label);
void GRAPHinsertE(Graph G, int id1, int id2, double wt);
void GRAPHremoveE(Graph G, int id1, int id2);
void GRAPHedges(Graph G, Edge *a);
Position GRAPHget_node_position(Graph G, int id);
int GRAPHget_num_nodes(Graph G);
link GRAPHget_list_node_head(Graph G, int v);
link GRAPHget_list_node_tail(Graph G, int v);
link LINKget_next(link t);
double LINKget_wt(link t);
int LINKget_node(link t);
// Parallel read
Graph GRAPHload_sequential(char *filepath);
Graph GRAPHload_parallel3(char *filepath, int num_partitions_nodes,
                          int num_partitions_edges, int num_threads_nodes,
                          int num_threads_edges);
Graph GRAPHload_parallel1(char *filepath, int num_threads);
void GRAPHload_parallel2(char *filepath, int num_threads, Graph *G, Graph *R);
#endif
