#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "Graph.h"
#include "PQ.h"
#include "Position.h"
#define maxWT INT_MAX
#define MAXC 10
typedef struct node *link;
struct node
{
    int v;
    double wt;
    link next;
};
struct graph
{
    int V;
    int E;
    link *ladj;
    ST tab;
    link z;
};
static Edge EDGEcreate(int v, int w, double wt);
static link NEW(int v, double wt, link next);
static void insertE(Graph G, Edge e);
static void removeE(Graph G, Edge e);
static Edge EDGEcreate(int v, int w, double wt)
{
    Edge e;
    e.v = v;
    e.w = w;
    e.wt = wt;
    return e;
}
static link NEW(int v, double wt, link next)
{
    link x = malloc(sizeof *x);
    if (x == NULL)
        return NULL;
    x->v = v;
    x->wt = wt;
    x->next = next;
    return x;
}
Graph GRAPHinit(int V)
{
    int v;
    Graph G = malloc(sizeof *G);
    if (G == NULL)
        return NULL;
    G->V = V;
    G->E = 0;
    G->z = NEW(-1, 0, NULL);
    if (G->z == NULL)
        return NULL;
    G->ladj = malloc(G->V * sizeof(link));
    if (G->ladj == NULL)
        return NULL;
    for (v = 0; v < G->V; v++)
        G->ladj[v] = G->z;
    G->tab = STinit(V);
    if (G->tab == NULL)
        return NULL;
    return G;
}
void GRAPHfree(Graph G)
{
    int v;
    link t, next;
    for (v = 0; v < G->V; v++)
        for (t = G->ladj[v]; t != G->z; t = next)
        {
            next = t->next;
            free(t);
        }
    STfree(G->tab);
    free(G->ladj);
    free(G->z);
    free(G);
}
Graph GRAPHload(FILE *fin)
{
    int V, i, id1, id2;
    char label1[MAXC], label2[MAXC];
    int node_index, node_x, node_y;
    double wt;
    Graph G;
    Position p;

    // Read the first line: number of nodes
    fscanf(fin, "%d", &V);

    // Initialize the Graph ADT
    G = GRAPHinit(V);
    if (G == NULL)
        return NULL;

    // Read the nodes name(index 0-V) and the coordinates of each node
    for (i = 0; i < V; i++)
    {
        fscanf(fin, "%d %d %d", &node_index, &node_x, &node_y);

        p = POSITIONinit(node_x, node_y);
        STinsert(G->tab, p, node_index);
        POSITIONfree(p);
    }

#if DEBUGPRINT
    // Print nodes coordinates
    for (i = 0; i < G->V; i++)
    {
        POSITIONprint(STsearchByIndex(G->tab, i));
    }
#endif

    while (fscanf(fin, "%d %d %lf", &id1, &id2, &wt) == 3)
    {
        if (id1 >= 0 && id2 >= 0)
            GRAPHinsertE(G, id1, id2, wt);
    }
    return G;
}
void GRAPHedges(Graph G, Edge *a)
{
    int v, E = 0;
    link t;
    for (v = 0; v < G->V; v++)
        for (t = G->ladj[v]; t != G->z; t = t->next)
            a[E++] = EDGEcreate(v, t->v, t->wt);
}
void GRAPHstore(Graph G, FILE *fout)
{
    int i;
    Edge *a;
    a = malloc(G->E * sizeof(Edge));
    if (a == NULL)
        return;
    GRAPHedges(G, a);
    fprintf(fout, "Number of nodes: %d\n", G->V);
    for (i = 0; i < G->V; i++){
        fprintf(fout, "Node %d: ", i);
        POSITIONprint(STsearchByIndex(G->tab, i), fout);
    }
    for (i = 0; i < G->E; i++){
        fprintf(fout, "Edge %d -> %d - Weight: %.1lf\n", a[i].v, a[i].w, a[i].wt);
    }
}
int GRAPHgetIndex(Graph G, char *label)
{
    int id;
    id = STsearch(G->tab, label);
    if (id == -1)
    {
        id = STsize(G->tab);
        STinsert(G->tab, label, id);
    }
    return id;
}
void GRAPHinsertE(Graph G, int id1, int id2, double wt)
{
    insertE(G, EDGEcreate(id1, id2, wt));
}
void GRAPHremoveE(Graph G, int id1, int id2)
{
    removeE(G, EDGEcreate(id1, id2, 0));
}
static void insertE(Graph G, Edge e)
{
    int v = e.v, w = e.w;
    double wt = e.wt;
    G->ladj[v] = NEW(w, wt, G->ladj[v]);
    G->E++;
}
static void removeE(Graph G, Edge e)
{
    int v = e.v, w = e.w;
    link x;
    if (G->ladj[v]->v == w)
    {
        G->ladj[v] = G->ladj[v]->next;
        G->E--;
    }
    else
        for (x = G->ladj[v]; x != G->z; x = x->next)
            if (x->next->v == w)
            {
                x->next = x->next->next;
                G->E--;
            }
}

// Test: Djkstra algorithm
void GRAPHspD(Graph G, int id)
{
    int v;
    link t;
    PQ pq = PQinit(G->V);
    int *st, *mindist;
    st = malloc(G->V * sizeof(int));
    mindist = malloc(G->V * sizeof(int));
    if ((st == NULL) || (mindist == NULL))
        return;
    for (v = 0; v < G->V; v++)
    {
        st[v] = -1;
        mindist[v] = maxWT;
        PQinsert(pq, mindist, v);
    }
    mindist[id] = 0;
    st[id] = id;
    PQchange(pq, mindist, id);
    while (!PQempty(pq))
    {
        if (mindist[v = PQextractMin(pq, mindist)] != maxWT)
        {
            for (t = G->ladj[v]; t != G->z; t = t->next)
                if (mindist[v] + t->wt < mindist[t->v])
                {
                    mindist[t->v] = mindist[v] + t->wt;
                    PQchange(pq, mindist, t->v);
                    st[t->v] = v;
                }
        }
    }
    printf("\n Shortest path tree\n");
    for (v = 0; v < G->V; v++)
        printf("parent of %d is %d \n", v, st[v]);
    printf("\n Minimum distances from node %d\n", id);
    for (v = 0; v < G->V; v++)
        printf("mindist[%d] = %d \n", v, mindist[v]);
    PQfree(pq);
}
int GRAPHget_num_nodes(Graph G){
    return G->V;
}
Position GRAPHget_node_position(Graph G, int id){
    return STsearchByIndex(G->tab, id);
}
/*
void GRAPHshortest_path_astar(Graph G, int source, int dest){
    int v;
    link t;
    PQ pq = PQinit(G->V);
    int *st, *mindist;

    st = malloc(G->V * sizeof(int));
    mindist = malloc(G->V * sizeof(int));
    if ((st == NULL) || (mindist == NULL))
        return;
    
    // Compute Heuristic function for each node [Euclidean distance from node N to node dest]

    // Add all the nodes inside the priority queue
    for (v = 0; v < G->V; v++)
    {
        st[v] = -1;
        mindist[v] = maxWT;
        PQinsert(pq, mindist, v);
    }

    mindist[id] = 0;
    st[id] = id;
    PQchange(pq, mindist, id);
    while (!PQempty(pq))
    {
        if (mindist[v = PQextractMin(pq, mindist)] != maxWT)
        {
            for (t = G->ladj[v]; t != G->z; t = t->next)
                if (mindist[v] + t->wt < mindist[t->v])
                {
                    mindist[t->v] = mindist[v] + t->wt;
                    PQchange(pq, mindist, t->v);
                    st[t->v] = v;
                }
        }
    }
    printf("\n Shortest path tree\n");
    for (v = 0; v < G->V; v++)
        printf("parent of %d is %d \n", v, st[v]);
    printf("\n Minimum distances from node %d\n", id);
    for (v = 0; v < G->V; v++)
        printf("mindist[%d] = %d \n", v, mindist[v]);
    PQfree(pq);
}*/
// ^(\s)*$\n