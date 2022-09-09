#include <float.h>

#include "../Graph/Graph.h"
#include "../Graph/PQ.h"
#include "../Graph/Position.h"
#include "Astar.h"
#include "pthread.h"

#define maxWT DBL_MAX
#define N 2

// Thread argument data structure
struct arg_t {
    int index;
    int V;
    int *common_pos;
    Graph G;
    Position pos_dest;
    pthread_spinlock_t *m;
    pthread_barrier_t *barr;
    int source;
    int *parentVertex;
    double *gvalues;
    double *otherGvalues;
    double *hvalues;
    double *otherHvalues;
    double *F;
    double *otherF;
    int *M;
    int *found;
    double *L;
    double *costToCome;
    char heuristic_type;
#if COLLECT_STAT
    int *expanded_nodes;
#endif
};

static void *nba(void *arg);

static void *nba(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int v, a, b;
    double f_extracted_node, g_b, f_b, a_b_wt, *fvalues;
    PQ open_list;
    Position p;
    link t;

    // SETUP
    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT
    open_list = PQinit(args->V);
    fvalues = (double *)malloc(args->V * sizeof(double));

    for (v = 0; v < args->V; v++) {
        args->parentVertex[v] = -1;
        if (args->index == 0) args->M[v] = 1;
        p = GRAPHget_node_position(args->G, v);
        args->hvalues[v] = heuristic(p, args->pos_dest, args->heuristic_type);
        args->gvalues[v] = maxWT;
        fvalues[v] = maxWT;
    }
    fvalues[args->source] =
        compute_f(args->hvalues[args->source], 0);  // g(n) = 0 for n == source
    args->gvalues[args->source] = 0;
    PQinsert(open_list, fvalues, args->source);
    *(args->F) = args->hvalues[args->source];

    pthread_barrier_wait(args->barr);

    while (*(args->found) == 0)  // while not finished
    {
        // Take from OPEN list the node with min f(n) (min priority)
        f_extracted_node = fvalues[a = PQextractMin(open_list, fvalues)];
#if COLLECT_STAT
        args->expanded_nodes[a]++;
#endif
        if (args->M[a] == 1) {
            //  For each successor 'b' of node 'a':
            if ((fvalues[a] < *(args->L)) &&
                (args->gvalues[a] + *(args->otherF) - args->otherHvalues[a] <
                 *(args->L))) {
                for (t = GRAPHget_list_node_head(args->G, a);
                     t != GRAPHget_list_node_tail(args->G, a);
                     t = LINKget_next(t)) {
                    b = LINKget_node(t);
                    a_b_wt = LINKget_wt(t);
                    // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
                    g_b = args->gvalues[a] + a_b_wt;
                    f_b = g_b + args->hvalues[b];

                    if (args->M[b] == 1 &&
                        args->gvalues[b] > args->gvalues[a] + a_b_wt) {
                        args->parentVertex[b] = a;
                        args->costToCome[b] = a_b_wt;
                        args->gvalues[b] = g_b;
                        fvalues[b] = f_b;
                        if (PQsearch(open_list, b) == -1)
                            PQinsert(open_list, fvalues, b);
                        else
                            PQchange(open_list, fvalues, b);

                        if (*(args->L) >
                            (args->gvalues[b] + args->otherGvalues[b])) {
                            pthread_spin_lock(args->m);
                            if (*(args->L) >
                                (args->gvalues[b] + args->otherGvalues[b])) {
                                *(args->common_pos) = b;
                                *(args->L) =
                                    (args->gvalues[b] + args->otherGvalues[b]);
                            }
                            pthread_spin_unlock(args->m);
                        }
                    }
                }
            }
            args->M[a] = 0;
        }
        if (!PQempty(open_list)) {
            *(args->F) = fvalues[PQshowMin(open_list)];
        } else {
            *(args->found) = *(args->found) + 1;
        }
    }
    pthread_exit(NULL);
}

void ASTARshortest_path_ab_ba(Graph G, Graph R, int source, int dest,
                              char heuristic_type) {
    printf("## NBA* [heuristic: %c] from %d to %d ##\n", heuristic_type, source,
           dest);
    int V = GRAPHget_num_nodes(G);
    int i, *parentVertexG, *parentVertexR, *M, common_pos = -1, found = 0;
    double *costToComeG, *costToComeR, *gvaluesG, *gvaluesR, *hvaluesG,
        *hvaluesR, FG, FR, L = maxWT;
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    pthread_t *threads;
    pthread_spinlock_t m;
    pthread_barrier_t barr;
    struct arg_t *args;

    threads = (pthread_t *)malloc(N * sizeof(pthread_t));
    if (threads == NULL) return NULL;
    args = (struct arg_t *)malloc(N * sizeof(struct arg_t));
    if (args == NULL) return NULL;

    parentVertexG = (int *)malloc(V * sizeof(int));
    costToComeG = (double *)malloc(V * sizeof(double));
    parentVertexR = (int *)malloc(V * sizeof(int));
    costToComeR = (double *)malloc(V * sizeof(double));
    gvaluesG = (double *)malloc(V * sizeof(double));
    gvaluesR = (double *)malloc(V * sizeof(double));
    hvaluesG = (double *)malloc(V * sizeof(double));
    hvaluesR = (double *)malloc(V * sizeof(double));
    M = (int *)malloc(V * sizeof(int));
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
    if (expanded_nodes == NULL) {
        return NULL;
    }
#endif
    if ((parentVertexG == NULL) || (parentVertexR == NULL) ||
        (costToComeG == NULL) || (costToComeR == NULL) || (gvaluesG == NULL) ||
        (gvaluesR == NULL) || (hvaluesG == NULL) || (hvaluesR == NULL) ||
        (M == NULL))
        return;

    pthread_spin_init(&m, PTHREAD_PROCESS_PRIVATE);
    pthread_barrier_init(&barr, NULL, 2);

    for (i = 0; i < N; i++) {
        if (i == 0) {
            args[i].G = G;
            args[i].parentVertex = parentVertexG;
            args[i].gvalues = gvaluesG;
            args[i].otherGvalues = gvaluesR;
            args[i].hvalues = hvaluesG;
            args[i].otherHvalues = hvaluesR;
            args[i].source = source;
            args[i].pos_dest = pos_dest;
            args[i].F = &FG;
            args[i].otherF = &FR;
            args[i].costToCome = costToComeG;
        } else {
            args[i].G = R;
            args[i].parentVertex = parentVertexR;
            args[i].gvalues = gvaluesR;
            args[i].otherGvalues = gvaluesG;
            args[i].hvalues = hvaluesR;
            args[i].otherHvalues = hvaluesG;
            args[i].source = dest;
            args[i].pos_dest = pos_source;
            args[i].F = &FR;
            args[i].otherF = &FG;
            args[i].costToCome = costToComeR;
        }
        args[i].index = i;
        args[i].V = V;
        args[i].m = &m;
        args[i].barr = &barr;
        args[i].common_pos = &common_pos;
        args[i].heuristic_type = heuristic_type;
        args[i].found = &found;
        args[i].M = M;
        args[i].L = &L;
#if COLLECT_STAT
        args[i].expanded_nodes = expanded_nodes;
#endif
        pthread_create(&threads[i], NULL, nba, (void *)&args[i]);
    }

    for (i = 0; i < N; i++) pthread_join(threads[i], NULL);

    if (common_pos != -1) {
        printf("L: %lf\n", L);
        printf("Common node: %d\n", common_pos);
        reconstruct_path_ab_ba(parentVertexG, parentVertexR, source, common_pos,
                               dest, costToComeG, costToComeR);
    } else {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
        return;
    }
#if COLLECT_STAT
    int n = 0, tot = 0;
    FILE *fp = fopen("./stats/stat_astar_ab_ba.txt", "w+");
    for (i = 0; i < V; i++) {
        if (expanded_nodes[i] != 0) {
            n++;
            tot += expanded_nodes[i];
            fprintf(fp, "%d\n", i);
        }
    }
    printf("Distict expanded nodes: %d [of %d]\nTotal expanded nodes: %d\n", n,
           V, tot);
    fclose(fp);
    free(expanded_nodes);
#endif
}