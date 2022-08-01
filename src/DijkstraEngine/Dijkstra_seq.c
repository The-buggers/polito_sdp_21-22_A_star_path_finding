#include "Dijkstra.h"
#include "float.h"
#define maxWT DBL_MAX
void DIJKSTRA_shortest_path_sequential(Graph G, int source, int dest){
    int v;
    link t;
    int V = GRAPHget_num_nodes(G);
    PQ pq = PQinit(V);
    int *st;
    double *mindist;
    int a, b;
    double a_b_wt;
    st = malloc(V * sizeof(int));
    mindist = malloc(V * sizeof(double));
    if ((st == NULL) || (mindist == NULL)) return;
    for (v = 0; v < V; v++) {
        st[v] = -1;
        mindist[v] = maxWT;
        PQinsert(pq, mindist, v);
    }
    mindist[source] = 0;
    st[source] = source;
    PQchange(pq, mindist, source);
    while (!PQempty(pq)) {
        if (mindist[a = PQextractMin(pq, mindist)] != maxWT) {
            for (t = GRAPHget_list_node_head(G, a);
                 t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);
                if (mindist[a] + a_b_wt < mindist[b]) {
                    mindist[b] = mindist[a] + a_b_wt;
                    PQchange(pq, mindist, b);
                    st[b] = a;
                }
            }
        }
        
        reconstruct_path(st, source, dest, mindist);
        PQfree(pq);
    }
}
// ^(\s)*$\n