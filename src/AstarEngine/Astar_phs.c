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

static void *phs(void *arg) {
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
void ASTARshortest_path_phs(Graph G, int source, int dest,
                            char heuristic_type) {
    printf(
        "## Parallel-Hierarchical-Search A* [heuristic: %c] from %d to %d ##\n",
        heuristic_type, source, dest);
    int V = GRAPHget_num_nodes(G);
    int i, num_threads = 3;
    pthread_t *threads;
    struct arg_t *args;
    struct path_t **paths;
    double best_cost_path = DBL_MAX;
    double *first_edge_cost;
    int c_found = -1, d_found = -1, v;
    Position p, p_nearest;
    Position a = GRAPHget_node_position(G, source);
    Position b = GRAPHget_node_position(G, dest);
    double ab = POSITIONcompute_haversine_distance(a, b);
    double p_lat, p_lon,
        a_lat = POSITIONget_latitude(a), b_lat = POSITIONget_latitude(b),
        a_lon = POSITIONget_longitude(a), b_lon = POSITIONget_longitude(b),
        dist;
    link t;

    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    args = (struct arg_t *)malloc(num_threads * sizeof(struct arg_t));
    paths = (struct path_t **)malloc(num_threads * sizeof(struct path_t *));

    if (threads == NULL || args == NULL) {
        return;
    }

    // Look for 2 intermediate nodes between source and dest
    v = 0;
    // p_nearest = a_lat < b_lat ? a : b;
    while (c_found == -1 || d_found == -1) {
        p = GRAPHget_node_position(G, v);
        p_lat = POSITIONget_latitude(p);
        p_lon = POSITIONget_longitude(p);
        if ((p_lat <= fmax(a_lat, b_lat) && p_lat >= fmin(a_lat, b_lat)) &&
            (p_lon <= fmax(a_lon, b_lon) && p_lon >= fmin(a_lon, b_lon)) &&
            (v != source && v != dest)) {
            dist = POSITIONcompute_haversine_distance(p, a);
            if (c_found == -1 && v != d_found) {
                if (dist >= ab / 3 && dist < ab / 2) {
                    c_found = v;
                    printf("Found node c: %d - ", v);
                    POSITIONprint(p, stdout);
                }
            }
            if (d_found == -1 && v != c_found) {
                if (dist >= 2 * ab / 3 && dist < ab) {
                    d_found = v;
                    printf("Found node d: %d - ", v);
                    POSITIONprint(p, stdout);
                }
            }
        }
        v++;
    }

    // Launch one thread for each sub-path to be found
    for (i = 0; i < 3; i++) {
        args[i].index = i;
        args[i].heuristic_type = heuristic_type;
        args[i].G = G;
        args[i].V = V;
        paths[i] = (struct path_t *)malloc(sizeof(struct path_t));
        paths[i]->costToCome = (double *)malloc(V * sizeof(double));
        paths[i]->parentVertex = (int *)malloc(V * sizeof(int));
        args[i].result = paths[i];
        if (i == 0) {
            args[i].source = source;
            args[i].dest = c_found;
        } else if (i == 1) {
            args[i].source = c_found;
            args[i].dest = d_found;
        } else {
            args[i].source = d_found;
            args[i].dest = dest;
        }
        pthread_create(&threads[i], NULL, phs, (void *)&args[i]);
    }
    double tot_cost;
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        tot_cost += paths[i]->cost;
        /*
        reconstruct_path(paths[i]->parentVertex, paths[i]->source,
                         paths[i]->dest, paths[i]->costToCome);*/
    }
    printf("Total cost computed: %lf\n", tot_cost);
    free(threads);
    free(args);

    return;
}
