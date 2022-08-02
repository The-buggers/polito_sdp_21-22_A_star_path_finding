#include "Dijkstra.h"
#include "float.h"
#define maxWT DBL_MAX
#define STAT 1
void DIJKSTRA_shortest_path_sequential(Graph G, int source, int dest) {
    int v;
    link t;
    int V = GRAPHget_num_nodes(G);
    PQ pq = PQinit(V);
    int *parentVertex;
    double *mindist, *costToCome;
    int a, b, found = 0;
    double a_b_wt;
    parentVertex = (int *)malloc(V * sizeof(int));
    mindist = (double *)malloc(V * sizeof(double));
    costToCome = (double *)malloc(V * sizeof(double));
    if ((parentVertex == NULL) || (mindist == NULL)) return;
    for (v = 0; v < V; v++) {
        parentVertex[v] = -1;
        mindist[v] = maxWT;
    }
#if STAT
    int *visited_nodes = (int *)calloc(V, sizeof(int));
#endif
    mindist[source] = 0;
    parentVertex[source] = -1;
    PQinsert(pq, mindist, source);
    while (!PQempty(pq)) {
        if (mindist[a = PQextractMin(pq, mindist)] != maxWT) {
#if STAT
            visited_nodes[a] = 1;
#endif
            if (a == dest) {
                found = 1;
                reconstruct_path(parentVertex, source, dest, costToCome);
                break;
            }
            for (t = GRAPHget_list_node_head(G, a);
                 t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);
                if (mindist[a] + a_b_wt < mindist[b]) {
                    mindist[b] = mindist[a] + a_b_wt;
                    parentVertex[b] = a;
                    costToCome[b] = a_b_wt;
                    if (PQsearch(pq, b) == -1)
                        PQinsert(pq, mindist, b);
                    else
                        PQchange(pq, mindist, b);
                }
            }
        }
    }
    if (found == 0) {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
    }
#if STAT
    FILE *fp = fopen("./stats/stat_dijkstra.txt", "w+");
    for (v = 0; v < V; v++) {
        if (visited_nodes[v] == 1) fprintf(fp, "%d\n", v);
    }
    fclose(fp);
    free(visited_nodes);
#endif
    free(mindist);
    free(parentVertex);
    free(costToCome);
    PQfree(pq);
}
// ^(\s)*$\n