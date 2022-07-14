#include "Astar.h"
#include "PQ.h"
#include "Graph.h"
#include "Position.h"
#include <math.h>
#include <limits.h>
#define maxWT INT_MAX
static double heuristic_euclidean(Position source, Position dest);
static double compute_f(double h, double g);
void ASTARshortest_path(Graph G, int source, int dest){
    printf("A star algorithm on graph from %d to %d\n", 42, source, dest);
    int V = GRAPHget_num_nodes(G);
    int v, extracted_node;
    int *previous;
    double *fvalues, *hvalues, *gvalues; // f(n) for each node n
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;

    PQ pq = PQinit(V);
    previous = (int*)malloc(V * sizeof(int));
    fvalues = (double*)malloc(V * sizeof(double));
    hvalues = (double*)malloc(V * sizeof(double));
    gvalues = (double*)malloc(V * sizeof(double));
    if((previous == NULL) || (fvalues == NULL) || (hvalues == NULL) || (gvalues == NULL))
        return;

    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT
    for(v=0; v<V; v++){
        previous[v] = -1;
        PQinsert(pq, fvalues[v]);
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic_euclidean(p, pos_dest);
        fvalues[v] = maxWT;
        gvalues[v] = maxWT;
    }
#if DEBUG_ASTAR
    printf("Pre copmputed heuristic:\n");
    for(v=0; v<V; v++){
        printf("h(%d) = %.2lf\n", v, hvalues[v]);
    }
#endif

    fvalues[source] = compute_f(hvalues[source], 0); // g(n) = 0 for n == source
    PQchange(pq, fvalues, source);

    while(!PQempty(pq)){
        extracted_node = PQextractMin(pq, fvalues);
        if(extracted_node == dest){
            // Solution found
            break;
        }
        if(fvalues[extracted_node] != maxWT){

        }
    }
}

double heuristic_euclidean(Position source, Position dest){
    return POSITIONcompute_euclidean_distance(source, dest);
}

double compute_f(double h, double g){
    return h + g;
}