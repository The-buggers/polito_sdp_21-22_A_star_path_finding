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
    // int num_threads;
    // int V;
    int *common_pos;
    Graph G;
    // Position pos_dest;
    pthread_spinlock_t *m;
    // int source;
    // int dest;
    PQ open_list;
    int *parentVertex;
    double *gvalues;
    double *otherGvalues;
    double *hvalues;
    double *otherHvalues;
    double *fvalues;
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
    int a, b;
    double f_extracted_node, g_b, f_b, a_b_wt;
    link t;

    while (*(args->found) == 0)  // while OPEN list not empty
    {
        // Take from OPEN list the node with min f(n) (min priority)
        f_extracted_node =
            args->fvalues[a = PQextractMin(args->open_list, args->fvalues)];
#if COLLECT_STAT
        args->expanded_nodes[a]++;
#endif

        if (args->M[a] == 1) {
            // For each successor 'b' of node 'a':
            if ((args->fvalues[a] < *(args->L)) &&
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
                        if (g_b < args->gvalues[b]) {
                            args->parentVertex[b] = a;
                            args->costToCome[b] = a_b_wt;
                            args->gvalues[b] = g_b;
                            args->fvalues[b] = f_b;
                            if (PQsearch(args->open_list, b) == -1) {
                                PQinsert(args->open_list, args->fvalues, b);
                            } else
                                PQchange(args->open_list, args->fvalues, b);

                            if ((args->gvalues[b] < maxWT &&
                                 args->otherGvalues[b] < maxWT) &&
                                (*(args->L) >
                                 (args->gvalues[b] + args->otherGvalues[b]))) {
                                if ((args->gvalues[b] < maxWT &&
                                     args->otherGvalues[b] < maxWT) &&
                                    (*(args->L) > (args->gvalues[b] +
                                                   args->otherGvalues[b]))) {
                                    pthread_spin_lock(args->m);
                                    *(args->common_pos) = b;
                                    *(args->L) = (args->gvalues[b] +
                                                  args->otherGvalues[b]);
                                    pthread_spin_unlock(args->m);
                                }
                            }
                        }
                    }
                }
            }
            pthread_spin_lock(args->m);
            args->M[a] = 0;
            pthread_spin_unlock(args->m);
        }
        if (!PQempty(args->open_list)) {
            *(args->F) = args->fvalues[PQshowMin(args->open_list)];
        } else {
            pthread_spin_lock(args->m);
            *(args->found) = 1;
            pthread_spin_unlock(args->m);
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
    double *costToComeG, *costToComeR, *gvaluesG, *gvaluesR, *fvaluesG,
        *fvaluesR, *hvaluesG, *hvaluesR, FG, FR, L = maxWT;
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position pG, pR;
    pthread_t *threads;
    pthread_spinlock_t m;
    struct arg_t *args;
    PQ open_listG, open_listR;

    open_listG = PQinit(V);
    open_listR = PQinit(V);

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
    fvaluesG = (double *)malloc(V * sizeof(double));
    fvaluesR = (double *)malloc(V * sizeof(double));
    M = (int *)malloc(V * sizeof(int));
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
#endif
    if ((parentVertexG == NULL) || (parentVertexR == NULL) ||
        (costToComeG == NULL) || (costToComeR == NULL) || (gvaluesG == NULL) ||
        (gvaluesR == NULL) || (hvaluesG == NULL) || (hvaluesR == NULL) ||
        (fvaluesG == NULL) || (fvaluesR == NULL) || (M == NULL))
        return;

    // SETUP
    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT
    for (i = 0; i < V; i++) {
        parentVertexG[i] = -1;
        parentVertexR[i] = -1;
        M[i] = 1;
        gvaluesG[i] = maxWT;
        gvaluesR[i] = maxWT;
        pG = GRAPHget_node_position(G, i);
        pR = GRAPHget_node_position(R, i);
        hvaluesG[i] = heuristic(pG, pos_dest, heuristic_type);
        fvaluesG[i] = maxWT;
        hvaluesR[i] = heuristic(pR, pos_source, heuristic_type);
        fvaluesR[i] = maxWT;
    }
    // For G
    fvaluesG[source] =
        compute_f(hvaluesG[source], 0);  // g(n) = 0 for n == source
    gvaluesG[source] = 0;
    PQinsert(open_listG, fvaluesG, source);
    FG = hvaluesG[source];
    // For R
    fvaluesR[dest] = compute_f(hvaluesR[dest], 0);  // g(n) = 0 for n == source
    gvaluesR[dest] = 0;
    PQinsert(open_listR, fvaluesR, dest);
    FR = hvaluesR[dest];

    pthread_spin_init(&m, 0);

    for (i = 0; i < N; i++) {
        if (i == 0) {
            args[i].G = G;
            args[i].open_list = open_listG;
            args[i].parentVertex = parentVertexG;
            args[i].gvalues = gvaluesG;
            args[i].otherGvalues = gvaluesR;
            args[i].hvalues = hvaluesG;
            args[i].otherHvalues = hvaluesR;
            args[i].fvalues = fvaluesG;
            args[i].F = &FG;
            args[i].otherF = &FR;
            args[i].costToCome = costToComeG;
        } else {
            args[i].G = R;
            args[i].open_list = open_listR;
            args[i].parentVertex = parentVertexR;
            args[i].gvalues = gvaluesR;
            args[i].otherGvalues = gvaluesG;
            args[i].hvalues = hvaluesR;
            args[i].otherHvalues = hvaluesG;
            args[i].fvalues = fvaluesR;
            args[i].F = &FR;
            args[i].otherF = &FG;
            args[i].costToCome = costToComeR;
        }
        args[i].index = i;
        args[i].m = &m;
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