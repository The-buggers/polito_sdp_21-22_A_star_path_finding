#include "Astar.h"
#include "PQ.h"
#include "Graph.h"
#include "Position.h"
#include <math.h>
#include <float.h>
#define maxWT DBL_MAX
static double heuristic_euclidean(Position source, Position dest);
static double compute_f(double h, double g);
static void reconstruct_path(int* previous, int source, int dest, double *cost, double* tot_cost);
static void reconstruct_path_r(int* previous, int j, double* cost, double* tot_cost);
void ASTARshortest_path(Graph G, int source, int dest)
{
    printf("A star algorithm on graph from %d to %d\n", source, dest);
    int V = GRAPHget_num_nodes(G);
    int v, a, b;
    double f_extracted_node, g_b, f_b, a_b_wt;
    int *previous;
    double *fvalues, *hvalues, *gvalues, *cost; // f(n) for each node n
    double tot_cost = 0;
    int *closed_list;
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    link t;

    PQ open_list = PQinit(V);
    previous = (int *)malloc(V * sizeof(int));
    fvalues = (double *)malloc(V * sizeof(double));
    hvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    closed_list = (int *)malloc(V * sizeof(int));
    cost = (double *)malloc(V * sizeof(double));
    if ((previous == NULL) ||
        (fvalues == NULL) ||
        (hvalues == NULL) ||
        (gvalues == NULL) ||
        (closed_list == NULL) ||
        (cost == NULL))
        return;

    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT
    for (v = 0; v < V; v++)
    {
        previous[v] = -1;
        closed_list[v] = -1;
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic_euclidean(p, pos_dest);
        fvalues[v] = maxWT;
        gvalues[v] = maxWT;
        // TRY PQinsert(open_list, fvalues, v);
    }

    fvalues[source] = compute_f(hvalues[source], 0); // g(n) = 0 for n == source
    gvalues[source] = 0;
    // TRY PQchange(open_list, fvalues, source);
    PQinsert(open_list, fvalues, source);

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

    int found = 0;
    while (!PQempty(open_list)) // while OPEN list not empty
    {
        // Take from OPEN list the node with min f(n) (min priority)
        f_extracted_node = fvalues[a = PQextractMin(open_list, fvalues)];

        // If the extracted node is the destination stop: path found
        if (a == dest)
        {   
            found = 1;
            reconstruct_path(previous, source, dest, cost, &tot_cost);
            break;
        }

        // For each successor 'b' of node 'a':
        for (t = GRAPHget_list_node_head(G, a); t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t))
        {
            b = LINKget_node(t);
            a_b_wt = LINKget_wt(t);
#if DEBUG_ASTAR
            printf("a: %d [f(a)=%.2lf, g(a)=%.2lf] - b: %d [f(b)=%.2lf, g(b)=%.2lf] - weight: %.2f\n", a, fvalues[a], gvalues[a], b, fvalues[b], gvalues[b], a_b_wt);
#endif

            // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
            g_b = gvalues[a] + a_b_wt;
            f_b = g_b + hvalues[b];

            if (g_b < gvalues[b])
            {
                previous[b] = a;
                cost[b] = a_b_wt;
                gvalues[b] = g_b;
                fvalues[b] = f_b;
                if(PQsearch(open_list, b) == -1){
                    PQinsert(open_list, fvalues, b);
                }else{
                    PQchange(open_list, fvalues, b);
                }
            }
        }
    }
    if(!found){
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
        return;
    }
#if DEBUG_ASTAR
    printf("Previuos array:\n");
    for (v = 0; v < V; v++)
    {
        printf("Previous[%d] = %d\n", v, previous[v]);
    }
    printf("Cost array:\n");
    for (v = 0; v < V; v++)
    {
        printf("Cost[%d] = %lf\n", v, cost[v]);
    }
#endif
}

static double heuristic_euclidean(Position source, Position dest)
{
    return POSITIONcompute_euclidean_distance(source, dest);
}

static double compute_f(double h, double g)
{
    return h + g;
}
static void reconstruct_path(int* previous, int source, int dest, double *cost, double* tot_cost){
    int i;
    printf("+-----------------------------------+");
    printf("\nPath found\n");
    printf("Path from %d to %d: [ ", source, dest);
    reconstruct_path_r(previous, dest, cost, tot_cost);
    printf("]");

    printf("\nCost: %.2lf\n", *tot_cost);
    printf("+-----------------------------------+\n\n");
}

static void reconstruct_path_r(int* previous, int j, double* cost, double* tot_cost){
    if(previous[j] == -1){
        return;
    }else{
        reconstruct_path_r(previous, previous[j], cost, tot_cost);
        if(previous[previous[j]] == -1){
            printf("%d ", previous[j]);
            *tot_cost = *tot_cost + cost[previous[j]];
        }
        printf("%d ", j);
        *tot_cost = *tot_cost + cost[j];
    }
}