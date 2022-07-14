#ifndef GRAPH_H
#define GRAPH_H
#define DEBUGPRINT 0
#include "ST.h"
#include <stdio.h>
typedef struct edge
{
    int v;
    int w;
    double wt;
} Edge;
typedef struct graph *Graph;
typedef struct node *link;
Graph GRAPHinit(int V);
void GRAPHfree(Graph G);
Graph GRAPHload(FILE *fin);
void GRAPHstore(Graph G, FILE *fin);
int GRAPHgetIndex(Graph G, char *label);
void GRAPHinsertE(Graph G, int id1, int id2, double wt);
void GRAPHremoveE(Graph G, int id1, int id2);
void GRAPHedges(Graph G, Edge *a);
void GRAPHspD(Graph G, int id);
void GRAPHshortest_path_astar(Graph G, int source, int dest);
Position GRAPHget_node_position(Graph G, int id);
int GRAPHget_num_nodes(Graph G);
link GRAPHget_list_node_head(Graph G, int v);
link GRAPHget_list_node_tail(Graph G, int v);
link LINKget_next(link t);
double LINKget_wt(link t);
int LINKget_node(link t);
#endif
