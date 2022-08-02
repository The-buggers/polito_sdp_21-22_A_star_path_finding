#include "Dijkstra.h"
#include "float.h"
#define maxWT DBL_MAX
#define STAT 1
void DIJKSTRA_shortest_path_sequential(Graph G, int source, int dest) {
    printf("Dijkstra algorithm on graph from %d to %d\n", source, dest);
    int v;
    link t;
    int V = GRAPHget_num_nodes(G);
    PQ pq = PQinit(V);
    int *parentVertex;
    int found = 0;
    double *mindist;
    parentVertex = malloc(V * sizeof(int));
    mindist = malloc(V * sizeof(double));

    if ((parentVertex == NULL) || (mindist == NULL)) return;

    for (v = 0; v < V; v++) {
        parentVertex[v] = -1;
        mindist[v] = maxWT;
        PQinsert(pq, mindist, v);
    }
#if STAT
    int *visited_nodes = (int *)calloc(V, sizeof(int));
    int n_visited = 0;
#endif
    mindist[source] = 0;
    parentVertex[source] = source;
    PQchange(pq, mindist, source);

    while (!PQempty(pq)) {
        if (mindist[v = PQextractMin(pq, mindist)] != maxWT) {
#if STAT
            n_visited++;
            visited_nodes[v] = 1;
#endif
            for (t = GRAPHget_list_node_head(G, v);
                 t != GRAPHget_list_node_tail(G, v); t = LINKget_next(t))

                if (mindist[v] + LINKget_wt(t) < mindist[LINKget_node(t)]) {
                    mindist[LINKget_node(t)] = mindist[v] + LINKget_wt(t);
                    PQchange(pq, mindist, LINKget_node(t));
                    parentVertex[LINKget_node(t)] = v;
                }
        }
    }
    if (parentVertex[dest] == -1) {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
    }
#if STAT
    FILE *fp = fopen("./stats/stat_dijkstra.txt", "w+");
    for (v = 0; v < V; v++) {
        if (visited_nodes[v] == 1) {
            fprintf(fp, "%d\n", v);
        }
    }
    printf("Visited nodes: %d\n", n_visited);
    fclose(fp);
    free(visited_nodes);
#endif
    free(mindist);
    // free(parentVertex);
    // free(costToCome);
    PQfree(pq);
}
// ^(\s)*$\n