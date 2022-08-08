#include <float.h>
#include <math.h>
#include <pthread.h>

#include "Astar.h"

#define maxWT DBL_MAX

// Thread argument data structure
struct arg_t {
    int index;
    int num_threads;
    PQ *open_lists;
    int V;
    Graph G;
    Position pos_source;
    Position pos_dest;
    pthread_cond_t *cond;
    pthread_mutex_t *mut;
    pthread_barrier_t *barrier;
    int *wait_flags;
    int source;
    int dest;
    int *previous;
    double *fvalues;
    double *hvalues;
    double *gvalues;
    double *cost;
    char heuristic_type;
#if COLLECT_STAT
    int *expanded_nodes;
#endif
};

int stop_flag = 0;
static void *hda(void *arg);

static void *hda(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int i, v, a, b, owner_a, owner_b;
    Position p;
    double f_extracted_node, g_b, f_b, a_b_wt;
    link t;
    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT
    for (v = 0; v < args->V; v++) {
#if DEBUG_ASTAR_SAS
        printf("Node %d belogs to T %d\n", v,
               hash_function(v, args->num_threads));
#endif
        if (hash_function(v, args->num_threads) == args->index) {
            args->previous[v] = -1;
            p = GRAPHget_node_position(args->G, v);
            args->hvalues[v] =
                heuristic(p, args->pos_dest, args->heuristic_type);
            args->fvalues[v] = maxWT;
            args->gvalues[v] = maxWT;
        }
    }
    if (hash_function(args->source, args->num_threads) == args->index) {
        args->fvalues[args->source] = compute_f(args->hvalues[args->source],
                                                0);  // g(n) = 0 for n == source
        args->gvalues[args->source] = 0;
        PQinsert(args->open_lists[args->index], args->fvalues, args->source);
    }
    // Waiting for all threads are ready
    pthread_barrier_wait(args->barrier);

    // START
    while (1)  // while OPEN list not empty
    {
        // Condition variable
        pthread_mutex_lock(&(args->mut[args->index]));
        while (PQempty(args->open_lists[args->index])) {
            args->wait_flags[args->index] = 1;
            stop_flag++;
#if DEBUG_ASTAR_SAS
            printf("T %d signal end\n", args->index);
#endif
            if (stop_flag == args->num_threads) {
                for (i = 0; i < args->num_threads; i++)
                    pthread_cond_signal(&(args->cond[i]));
                pthread_mutex_unlock(&(args->mut[args->index]));
                pthread_exit(NULL);
            }
            pthread_cond_wait(&(args->cond[args->index]),
                              &(args->mut[args->index]));
            if (stop_flag == args->num_threads) {
                pthread_mutex_unlock(&(args->mut[args->index]));
                pthread_exit(NULL);
            };
        }
        pthread_mutex_unlock(&(args->mut[args->index]));

        // Take from OPEN list the node with min f(n) (min priority)
        pthread_mutex_lock(&(args->mut[args->index]));
        f_extracted_node =
            args->fvalues[a = PQextractMin(args->open_lists[args->index],
                                           args->fvalues)];
#if COLLECT_STAT
        args->expanded_nodes[a]++;
#endif
        pthread_mutex_unlock(&(args->mut[args->index]));
#if DEBUG_ASTAR_SAS
        printf("T %d extracts node %d\n", args->index, a);
#endif

        // For each successor 'b' of node 'a':
        for (t = GRAPHget_list_node_head(args->G, a);
             t != GRAPHget_list_node_tail(args->G, a); t = LINKget_next(t)) {
            b = LINKget_node(t);
            a_b_wt = LINKget_wt(t);
            // Compute owers
            owner_a = hash_function(a, args->num_threads);
            owner_b = hash_function(b, args->num_threads);
            // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
            pthread_mutex_lock(&(args->mut[owner_a]));
            g_b = args->gvalues[a] + a_b_wt;
            pthread_mutex_unlock(&(args->mut[owner_a]));
            f_b = g_b + args->hvalues[b];

            pthread_mutex_lock(&(args->mut[owner_b]));
            if (g_b < args->gvalues[b] && f_b < args->fvalues[b]) {
#if DEBUG_ASTAR_SAS
                printf("T %d extract %d from %d\n", args->index, b, a);
#endif
                args->previous[b] = a;
                args->cost[b] = a_b_wt;
                args->gvalues[b] = g_b;
                args->fvalues[b] = f_b;

                PQinsert(args->open_lists[owner_b], args->fvalues, b);

#if DEBUG_ASTAR_SAS
                printf("T %d signals %d\n", args->index, owner_b);
#endif
                pthread_cond_signal(&(args->cond[owner_b]));

                if (args->wait_flags[owner_b] == 1) stop_flag--;
                args->wait_flags[owner_b] = 0;
            }
            pthread_mutex_unlock(&(args->mut[owner_b]));
        }
    }
}
void ASTARshortest_path_sas_sf(Graph G, int source, int dest,
                               char heuristic_type, int num_threads) {
    printf("## SAS-SF A* [heuristic: %c] from %d to %d ##\n", heuristic_type,
           source, dest);
    int V = GRAPHget_num_nodes(G);
    int i, num_threads_nodes, *previous, *wait_flags, stop_flag = 0, found = 0;
    pthread_cond_t *cond;
    pthread_mutex_t *mut;
    pthread_barrier_t barrier;
    double *fvalues, *hvalues, *gvalues, *cost,
        tot_cost = 0;  // f(n) for each node n
    pthread_t *threads;
    struct arg_t *args;
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    num_threads_nodes = (int)ceil(((float)V) / num_threads);

    // Matrix of priority queues
    PQ *open_lists = (PQ *)malloc(num_threads * sizeof(PQ));
    for (i = 0; i < num_threads; i++) {
        open_lists[i] = (PQ)malloc(
            num_threads_nodes * (sizeof(int *) + sizeof(int *) + sizeof(int)));
        open_lists[i] = PQinit(V);
    }

    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    args = (struct arg_t *)malloc(num_threads * sizeof(struct arg_t));
    cond = (pthread_cond_t *)malloc(num_threads * sizeof(pthread_cond_t));
    mut = (pthread_mutex_t *)malloc(num_threads * sizeof(pthread_mutex_t));
    wait_flags = (int *)malloc(num_threads * sizeof(int));
    previous = (int *)malloc(V * sizeof(int));
    fvalues = (double *)malloc(V * sizeof(double));
    hvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    cost = (double *)malloc(V * sizeof(double));
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
    if (expanded_nodes == NULL) {
        return NULL;
    }
#endif
    if ((fvalues == NULL) || (previous == NULL) || (cond == NULL) ||
        (mut == NULL) || (hvalues == NULL) || (wait_flags == NULL) ||
        (gvalues == NULL) || (cost == NULL) || (threads == NULL) ||
        (args == NULL))
        return;

    for (i = 0; i < num_threads; i++) wait_flags[i] = 0;

    pthread_barrier_init(&barrier, NULL, num_threads);
    for (i = 0; i < num_threads; i++) {
        pthread_cond_init(&cond[i], NULL);
        pthread_mutex_init(&mut[i], NULL);
        args[i].index = i;
        args[i].num_threads = num_threads;
        args[i].open_lists = open_lists;
        args[i].cond = cond;
        args[i].mut = mut;
        args[i].wait_flags = wait_flags;
        args[i].barrier = &barrier;
        args[i].V = V;
        args[i].G = G;
        args[i].pos_source = pos_source;
        args[i].pos_dest = pos_dest;
        args[i].source = source;
        args[i].dest = dest;
        args[i].previous = previous;
        args[i].fvalues = fvalues;
        args[i].hvalues = hvalues;
        args[i].gvalues = gvalues;
        args[i].cost = cost;
        args[i].heuristic_type = heuristic_type;
#if COLLECT_STAT
        args[i].expanded_nodes = expanded_nodes;
#endif
        pthread_create(&threads[i], NULL, hda, (void *)&args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        pthread_cond_destroy(&cond[i]);
        pthread_mutex_destroy(&mut[i]);
        free(open_lists[i]);
    }

    pthread_barrier_destroy(&barrier);

    if (args->gvalues[args->dest] < maxWT) {
        reconstruct_path(args->previous, args->source, args->dest, args->cost);
    } else {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
    }
#if COLLECT_STAT
    int n = 0;
    int tot = 0;
    FILE *fp = fopen("./stats/stat_astar_sas_sf.txt", "w+");
    for (int v = 0; v < V; v++) {
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
    free(threads);
    free(args);
    free(cond);
    free(mut);
    free(open_lists);
    free(previous);
    free(fvalues);
    free(hvalues);
    free(gvalues);
    free(cost);

    return;
}
