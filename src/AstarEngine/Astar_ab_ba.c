#include <float.h>

#include "../Graph/Graph.h"
#include "../Graph/PQ.h"
#include "../Graph/Position.h"
#include "Astar.h"
#include "pthread.h"

#define maxWT DBL_MAX

// Thread argument data structure
struct arg_t {
    int index;
    int num_threads;
    int *found;
    int V;
    Graph G;
    Position pos_source;
    Position pos_dest;
    pthread_mutex_t *m;
    int source;
    int dest;
    int *parentVertex;
    double *costToCome;
    char heuristic_type;
#if COLLECT_STAT
    int *expanded_nodes;
#endif
};

static void *nba(void *arg);

static void *nba(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int i, v, a, b;
    double f_extracted_node, g_b, f_b, a_b_wt;
    double *fvalues, *hvalues, *gvalues;
    Position p;
    link t;
    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT

    PQ open_list = PQinit(args->V);
    fvalues = (double *)malloc(args->V * sizeof(double));
    hvalues = (double *)malloc(args->V * sizeof(double));
    gvalues = (double *)malloc(args->V * sizeof(double));
    if ((fvalues == NULL) || (hvalues == NULL) || (gvalues == NULL)) return;
    for (v = 0; v < args->V; v++) {
        args->parentVertex[v] = -1;
        p = GRAPHget_node_position(args->G, v);
        hvalues[v] = heuristic(p, args->pos_dest, args->heuristic_type);
        fvalues[v] = maxWT;
        gvalues[v] = maxWT;
    }
    fvalues[args->source] =
        compute_f(hvalues[args->source], 0);  // g(n) = 0 for n == source
    gvalues[args->source] = 0;
    PQinsert(open_list, fvalues, args->source);

    while (!PQempty(open_list))  // while OPEN list not empty
    {
        // Take from OPEN list the node with min f(n) (min priority)
        f_extracted_node = fvalues[a = PQextractMin(open_list, fvalues)];
#if COLLECT_STAT
        args->expanded_nodes[a]++;
#endif
        // If the extracted node is the destination stop: path found
        if (a == args->dest) {
            pthread_mutex_lock(args->m);
            args->found = 1;
            pthread_mutex_unlock(args->m);
            break;
        }

        // For each successor 'b' of node 'a':
        for (t = GRAPHget_list_node_head(args->G, a);
             t != GRAPHget_list_node_tail(args->G, a); t = LINKget_next(t)) {
            b = LINKget_node(t);
            a_b_wt = LINKget_wt(t);
            // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
            g_b = gvalues[a] + a_b_wt;
            f_b = g_b + hvalues[b];

            if (g_b < gvalues[b]) {
                args->parentVertex[b] = a;
                args->costToCome[b] = a_b_wt;
                gvalues[b] = g_b;
                fvalues[b] = f_b;
                if (PQsearch(open_list, b) == -1) {
                    PQinsert(open_list, fvalues, b);
                }
            }
        }
    }
    pthread_exit(NULL);
}

void ASTARshortest_path_ab_ba(Graph G, Graph R, int source, int dest,
                              char heuristic_type) {
    printf("## NBA* [heuristic: %c] from %d to %d ##\n", heuristic_type, source,
           dest);
    int V = GRAPHget_num_nodes(G);
    int v, i, found = 0, *parentVertexG, *parentVertexR;
    double *costToComeG, *costToComeR;
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    pthread_t *threads;
    pthread_mutex_t m;
    struct arg_t *args;

    threads = (pthread_t *)malloc(2 * sizeof(pthread_t));
    if (threads == NULL) return NULL;
    args = (struct arg_t *)malloc(2 * sizeof(struct arg_t));
    if (args == NULL) return NULL;

    parentVertexG = (int *)malloc(args->V * sizeof(int));
    costToComeG = (double *)malloc(args->V * sizeof(double));
    parentVertexR = (int *)malloc(args->V * sizeof(int));
    costToComeR = (double *)malloc(args->V * sizeof(double));
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(args->V, sizeof(int));
#endif

    pthread_mutex_init(&m, NULL);

    for (i = 0; i < 2; i++) {
        if (i == 0) {
            args[i].G = G;
            args[i].pos_source = pos_source;
            args[i].pos_dest = pos_dest;
            args[i].source = source;
            args[i].dest = dest;
            args[i].parentVertex = parentVertexG;
            args[i].costToCome = costToComeG;
        } else {
            args[i].G = R;
            args[i].pos_source = pos_dest;
            args[i].pos_dest = pos_source;
            args[i].source = dest;
            args[i].dest = source;
            args[i].parentVertex = parentVertexR;
            args[i].costToCome = costToComeR;
        }
        args[i].index = i;
        args[i].num_threads = 2;
        args[i].m = &m;
        args[i].heuristic_type = heuristic_type;
        args[i].V = V;
        args[i].found = &found;
#if COLLECT_STAT
        args[i].expanded_nodes = expanded_nodes;
#endif
        pthread_create(&threads[i], NULL, nba, (void *)&args[i]);
    }

    for (i = 0; i < 2; i++) pthread_join(threads[i], NULL);

    if (found) {
        reconstruct_path(parentVertexG, source, dest, costToComeG);
    } else {
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
    printf("Distict expanded nodes: %d [of %d]\nTotal expanded nodes: %d\n", n,
           V, tot);
    fclose(fp);
    free(expanded_nodes);
#endif
}