#include <float.h>
#include <math.h>

#include "../Graph/Graph.h"
#include "../Graph/PQ.h"
#include "../Graph/Position.h"
#include "Astar.h"
#define maxWT DBL_MAX
void ASTARshortest_path_sequential(Graph G, int source, int dest, char heuristic_type) {
    printf("## Sequential A* [heuristic: %c] from %d to %d ##\n", heuristic_type, source, dest);
    int V = GRAPHget_num_nodes(G);
    int v, a, b;
    double f_extracted_node, g_b, f_b, a_b_wt;
    int *parentVertex;
    double *fvalues, *hvalues, *gvalues, *costToCome;  // f(n) for each node n
    double tot_cost = 0;
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    link t;

    PQ open_list = PQinit(V);
    parentVertex = (int *)malloc(V * sizeof(int));
    fvalues = (double *)malloc(V * sizeof(double));
    hvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    costToCome = (double *)malloc(V * sizeof(double));
    if ((parentVertex == NULL) || (fvalues == NULL) || (hvalues == NULL) ||
        (gvalues == NULL) || (costToCome == NULL))
        return;
    for (v = 0; v < V; v++) {
        parentVertex[v] = -1;
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic(p, pos_dest, heuristic_type);
        fvalues[v] = maxWT;
        gvalues[v] = maxWT;
    }
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
#endif
    fvalues[source] =
        compute_f(hvalues[source], 0);  // g(n) = 0 for n == source
    gvalues[source] = 0;
    PQinsert(open_list, fvalues, source);

    int found = 0;
    while (!PQempty(open_list))  // while OPEN list not empty
    {
        // Take from OPEN list the node with min f(n) (min priority)
        f_extracted_node = fvalues[a = PQextractMin(open_list, fvalues)];
#if COLLECT_STAT
        expanded_nodes[a]++;
#endif
        // If the extracted node is the destination stop: path found
        if (a == dest) {
            found = 1;
            reconstruct_path(parentVertex, source, dest, costToCome);
            break;
        }

        // For each successor 'b' of node 'a':
        for (t = GRAPHget_list_node_head(G, a);
             t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
            b = LINKget_node(t);
            a_b_wt = LINKget_wt(t);
            // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
            g_b = gvalues[a] + a_b_wt;
            f_b = g_b + hvalues[b];

            if (g_b < gvalues[b]) {
                parentVertex[b] = a;
                costToCome[b] = a_b_wt;
                gvalues[b] = g_b;
                fvalues[b] = f_b;
                if (PQsearch(open_list, b) == -1) {
                    PQinsert(open_list, fvalues, b);
                } else {
                    PQchange(open_list, fvalues, b);
                }
            }
        }
    }
    if (!found) {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
        return;
    }
#if COLLECT_STAT
    int n = 0, tot = 0;
    FILE *fp = fopen("./stats/stat_astar_seq.txt", "w+");
        for (v = 0; v < V; v++) {
        if (expanded_nodes[v] != 0) {
            n++;
            tot += expanded_nodes[v];
            fprintf(fp, "%d\n", v);
        }
    }
    printf("Distict expanded nodes: %d [of %d]\nTotal expanded nodes: %d\n", n, V, tot);
    fclose(fp);
    free(expanded_nodes);
#endif
    free(parentVertex);
    free(costToCome);
    free(hvalues);
    free(gvalues);
    free(fvalues);
    PQfree(open_list);
}