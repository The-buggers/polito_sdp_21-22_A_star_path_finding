#include "Astar.h"
#include "PQ.h"
#include "Graph.h"
#include "Position.h"
#include <math.h>
#include <float.h>
#define maxWT DBL_MAX
static double heuristic_euclidean(Position source, Position dest);
static double compute_f(double h, double g);
void ASTARshortest_path(Graph G, int source, int dest)
{
    printf("A star algorithm on graph from %d to %d\n", 42, source, dest);
    int V = GRAPHget_num_nodes(G);
    int v;
    double f_extracted_node;
    int *previous;
    double *fvalues, *hvalues, *gvalues; // f(n) for each node n
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    link t;

    PQ pq = PQinit(V);
    previous = (int *)malloc(V * sizeof(int));
    fvalues = (double *)malloc(V * sizeof(double));
    hvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    if ((previous == NULL) || (fvalues == NULL) || (hvalues == NULL) || (gvalues == NULL))
        return;

    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT
    for (v = 0; v < V; v++)
    {
        previous[v] = -1;
        PQinsert(pq, fvalues, v);
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic_euclidean(p, pos_dest);
        fvalues[v] = maxWT;
        gvalues[v] = maxWT;
    }
#if DEBUG_ASTAR
    printf("Pre copmputed heuristic:\n");
    for (v = 0; v < V; v++)
    {
        printf("h(%d) = %.2lf\n", v, hvalues[v]);
    }
    printf("Starting f-values:\n");
    for (v = 0; v < V; v++)
    {
        printf("f(%d) = %.2lf\n", v, fvalues[v]);
    }
#endif

    fvalues[source] = compute_f(hvalues[source], 0); // g(n) = 0 for n == source
    PQchange(pq, fvalues, source);

    while (!PQempty(pq))
    {
        f_extracted_node = fvalues[v = PQextractMin(pq, fvalues)];
        if (f_extracted_node != maxWT)
        {
            // For each neighbor
            for (t = GRAPHget_list_node_head(G, v); t != GRAPHget_list_node_tail(G, v); t = LINKget_next(t)){
#if DEBUG_ASTAR
                printf("Node parent: %d - Neighbor: %d - Weight: %lf\n", v, LINKget_node(t), LINKget_wt(t));
#endif
                // Compute g(v) = g(parent) + dist(parent, v)
                // 
            }
        }
    }
}

double heuristic_euclidean(Position source, Position dest)
{
    return POSITIONcompute_euclidean_distance(source, dest);
}

double compute_f(double h, double g)
{
    return h + g;
}