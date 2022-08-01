#include "Dijkstra.h"
#include "float.h"
#define maxWT DBL_MAX
void DIJKSTRA_shortest_path_sequential(Graph G, int source, int dest) {
    int v;
    link t;
    int V = GRAPHget_num_nodes(G);
    PQ pq = PQinit(V);
    int *parentVertex;
    double *mindist;
    int a, b;
    double a_b_wt;
    parentVertex = malloc(V * sizeof(int));
    mindist = malloc(V * sizeof(double));
    if ((parentVertex == NULL) || (mindist == NULL)) return;
    for (v = 0; v < V; v++) {
        parentVertex[v] = -1;
        mindist[v] = maxWT;
    }

    mindist[source] = 0;
    parentVertex[source] = -1;
    PQinsert(pq, mindist, source);
    while (!PQempty(pq)) {
        if (mindist[a = PQextractMin(pq, mindist)] != maxWT) {
            for (t = GRAPHget_list_node_head(G, a);
                 t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);
                if (mindist[a] + a_b_wt < mindist[b]) {
                    mindist[b] = mindist[a] + a_b_wt;
                    if (PQsearch(pq, b) == -1)
                        PQinsert(pq, mindist, b);
                    else
                        PQchange(pq, mindist, b);
                    parentVertex[b] = a;
                }
            }
            // if (a == dest) break;
        }
    }
    reconstruct_path(parentVertex, source, dest, mindist);
    PQfree(pq);
}
// ^(\s)*$\n