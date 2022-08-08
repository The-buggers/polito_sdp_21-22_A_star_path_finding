#include "Dijkstra.h"
#include "float.h"
#define maxWT DBL_MAX
#define COLLECT_STAT 1
static void get_path(int *path_to_dest, int source, int dest);
void DIJKSTRA_shortest_path_sequential(Graph G, int source, int dest) {
    printf("Dijkstra algorithm from %d to %d\n", source, dest);
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
        // PQinsert(pq, mindist, v);
    }
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
#endif
    mindist[source] = 0;
    parentVertex[source] = -1;
    PQinsert(pq, mindist, source);

    while (!PQempty(pq)) {
        if (mindist[v = PQextractMin(pq, mindist)] != maxWT) {
#if COLLECT_STAT
            expanded_nodes[v]++;
#endif
            if (v == dest) {
                found = 1;
                break;
            }
            for (t = GRAPHget_list_node_head(G, v);
                 t != GRAPHget_list_node_tail(G, v); t = LINKget_next(t))

                if (mindist[v] + LINKget_wt(t) < mindist[LINKget_node(t)]) {
                    mindist[LINKget_node(t)] = mindist[v] + LINKget_wt(t);
                    PQinsert(pq, mindist, LINKget_node(t));
                    parentVertex[LINKget_node(t)] = v;
                }
        }
    }
    printf("+-----------------------------------+\n");
    if (found == 1) {
        printf("Path from %d to %d: [ ", source, dest);
        get_path(parentVertex, source, dest);
        printf("]");

        printf("\nCost: %.2lf\n", mindist[dest]);
    } else {
        printf("Path from %d to %d not found.\n", source, dest);
    }
    printf("+-----------------------------------+\n");
#if COLLECT_STAT
    int n = 0, tot = 0;
    FILE *fp = fopen("./stats/stat_dijkstra.txt", "w+");
    for (v = 0; v < V; v++) {
        if (expanded_nodes[v] != 0) {
            n++;
            tot += expanded_nodes[v];
            fprintf(fp, "%d\n", v);
        }
    }
    printf("Distict expanded nodes: %d [of %d]\nTotal expanded nodes: %d\n", n,
           V, tot);
    fclose(fp);
    free(expanded_nodes);
#endif
    free(mindist);
    // free(parentVertex);
    // free(costToCome);
    PQfree(pq);
}

static void get_path(int *path_to_dest, int source, int dest) {
    if (source == dest) {
        printf("%d ", source);
        return;
    } else {
        get_path(path_to_dest, source, path_to_dest[dest]);
        printf("%d ", dest);
    }
}
