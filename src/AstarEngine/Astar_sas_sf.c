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
    int *wait_flags;
    int source;
    int dest;
    int *previous;
    double *fvalues;
    double *hvalues;
    double *gvalues;
    double *cost;
};

int stop_flag = 0;
static void *hda(void *arg);

static void *hda(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int i, v, a, b, owner_a, owner_b;
    double f_extracted_node, g_b, f_b, a_b_wt;
    link t;
    // - Insert all the nodes in the priority queue
    // - compute h(n) for each node
    // - initialize f(n) to maxWT
    // - initialize g(n) to maxWT

#if DEBUG_ASTAR_SAS
    printf("Node %d belogs to T %d\n", v, hash_function(v, args->num_threads));
#endif

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

void ASTARshortest_path_sas_sf(Graph G, int source, int dest, int num_threads) {
    printf("A star algorithm (SAS version) on graph from %d to %d\n", source,
           dest);
    int V = GRAPHget_num_nodes(G);
    int i, num_threads_nodes, *previous, *wait_flags, stop_flag = 0, found = 0;
    pthread_cond_t *cond;
    pthread_mutex_t *mut;
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
    if ((fvalues == NULL) || (previous == NULL) || (cond == NULL) ||
        (mut == NULL) || (hvalues == NULL) || (wait_flags == NULL) ||
        (gvalues == NULL) || (cost == NULL) || (threads == NULL) ||
        (args == NULL))
        return;

    // SETUP
    for (i = 0; i < V; i++) {
        previous[i] = -1;
        hvalues[i] =
            heuristic_euclidean(GRAPHget_node_position(G, i), pos_dest);
        fvalues[i] = maxWT;
        gvalues[i] = maxWT;
    }
    fvalues[source] =
        compute_f(hvalues[source], 0);  // g(n) = 0 for n == source
    gvalues[source] = 0;
    PQinsert(open_lists[hash_function(source, num_threads)], fvalues, source);

    for (i = 0; i < num_threads; i++) {
        wait_flags[i] = 0;
        pthread_cond_init(&cond[i], NULL);
        pthread_mutex_init(&mut[i], NULL);
        args[i].index = i;
        args[i].num_threads = num_threads;
        args[i].open_lists = open_lists;
        args[i].cond = cond;
        args[i].mut = mut;
        args[i].wait_flags = wait_flags;
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
        pthread_create(&threads[i], NULL, hda, (void *)&args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        pthread_cond_destroy(&cond[i]);
        pthread_mutex_destroy(&mut[i]);
        free(open_lists[i]);
    }

    if (args->gvalues[args->dest] < maxWT) {
        reconstruct_path(args->previous, args->source, args->dest, args->cost);
    } else {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
    }

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
