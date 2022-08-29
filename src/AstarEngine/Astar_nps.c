#include <float.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#include "./Astar.h"

#define maxWT DBL_MAX
#define DEBUG_ASTAR 0

struct path_t {
    int source;
    int dest;
    int *parentVertex;
    double *costToCome;
    double cost;
};
struct arg_t {
    int index;
    int num_threads;
    int source;
    int dest;
    char heuristic_type;
    int V;
    Graph G;
    struct path_t *result;
};
static void *nps(void *arg);

static void *nps(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int source = args->source, dest = args->dest, V = args->V;
    struct path_t *result = args->result;
    char heuristic_type = args->heuristic_type;
    Graph G = args->G;

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

    printf("Thread [%d]: path from %d to %d\n", args->index, source, dest);
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

    fvalues[source] = compute_f(hvalues[source], 0);
    gvalues[source] = 0;
    PQinsert(open_list, fvalues, source);

    int found = 0;
    while (!PQempty(open_list)) {
        f_extracted_node = fvalues[a = PQextractMin(open_list, fvalues)];
        if (a == dest) {
            found = 1;
            // reconstruct_path(parentVertex, source, dest, costToCome);
            result->costToCome = costToCome;
            result->source = source;
            result->dest = dest;
            result->parentVertex = parentVertex;
            result->cost = gvalues[dest];
            break;
        }

        for (t = GRAPHget_list_node_head(G, a);
             t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
            b = LINKget_node(t);
            a_b_wt = LINKget_wt(t);

            g_b = gvalues[a] + a_b_wt;
            f_b = g_b + hvalues[b];

            if (g_b < gvalues[b]) {
                parentVertex[b] = a;
                costToCome[b] = a_b_wt;
                gvalues[b] = g_b;
                fvalues[b] = f_b;
                if (PQsearch(open_list, b) == -1) {
                    PQinsert(open_list, fvalues, b);
                }
            }
        }
    }
    if (!found) {
        result->cost = -1;
        return;
    }
    free(gvalues);
    free(fvalues);
    free(hvalues);
    PQfree(open_list);
    pthread_exit(NULL);
}
void ASTARshortest_path_nps(Graph G, int source, int dest,
                            char heuristic_type) {
    printf("## Neighbors-Parallel-Search A* [heuristic: %c] from %d to %d ##\n",
           heuristic_type, source, dest);
    int V = GRAPHget_num_nodes(G);
    int i, num_threads = 0;
    pthread_t *threads;
    struct arg_t *args;
    struct path_t **paths;
    double best_cost_path = DBL_MAX;
    double *first_edge_cost;
    int thread_best_path;
    link t;

    // Get all the neighbors of node start
    for (t = GRAPHget_list_node_head(G, source);
         t != GRAPHget_list_node_tail(G, source); t = LINKget_next(t)) {
        num_threads++;
    }
    printf("Node [%d] has %d neighbors\n", source, num_threads);

    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    args = (struct arg_t *)malloc(num_threads * sizeof(struct arg_t));
    paths = (struct path_t **)malloc(num_threads * sizeof(struct path_t *));
    first_edge_cost = (double *)malloc(num_threads * sizeof(double));

    if (threads == NULL || args == NULL || paths == NULL) {
        return;
    }

    // Launch one thread for each neighbor of source
    for (t = GRAPHget_list_node_head(G, source), i = 0;
         t != GRAPHget_list_node_tail(G, source); t = LINKget_next(t), i++) {
        first_edge_cost[i] = LINKget_wt(t);
        args[i].index = i;
        args[i].num_threads = num_threads;
        args[i].source = LINKget_node(t);
        args[i].dest = dest;
        args[i].heuristic_type = heuristic_type;
        args[i].V = V;
        args[i].G = G;
        paths[i] = (struct path_t *)malloc(sizeof(struct path_t));
        paths[i]->costToCome = (double *)malloc(V * sizeof(double));
        paths[i]->parentVertex = (int *)malloc(V * sizeof(int));
        args[i].result = paths[i];
        pthread_create(&threads[i], NULL, nps, (void *)&args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        if (paths[i]->cost < best_cost_path) {
            best_cost_path = paths[i]->cost;
            thread_best_path = i;
        } else {
            free(paths[i]->costToCome);
            free(paths[i]->parentVertex);
            free(paths[i]);
        }
    }

    reconstruct_path(paths[thread_best_path]->parentVertex,
                     paths[thread_best_path]->source, dest,
                     paths[thread_best_path]->costToCome);
    free(paths[thread_best_path]->costToCome);
    free(paths[thread_best_path]->parentVertex);
    free(paths[thread_best_path]);
    free(paths);
    free(threads);
    free(args);
    free(first_edge_cost);
    return;
}
