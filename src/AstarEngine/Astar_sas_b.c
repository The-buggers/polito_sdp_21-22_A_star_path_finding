#include <float.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#include "./Astar.h"

#define maxWT DBL_MAX
#define DEBUG_ASTAR 0

typedef struct barrier_s {
    sem_t sem1, sem2;
    pthread_mutex_t mutex;
    int count;
} barrier_t;
struct arg_t {
    int index;
    int num_threads;
    PQ *open_lists;
    int V;
    Graph G;
    pthread_mutex_t *mut_threads;
    int *wait_flags;
    int source;
    int dest;
    int *parentVertex;
    double *hvalues;
    double *gvalues;
    double *costToCome;
    barrier_t *bar;
    int *open_set_empty;
    double *closed_set;
    double **t_fvalues;
    pthread_mutex_t *mut_nodes;
#if COLLECT_STAT
    int *expanded_nodes;
#endif
};
static void *hda(void *arg);

static void *hda(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int i, v, a, b, k_a, k_b;
    Position p;
    double g_b, f_b, a_b_wt, tot_cost = 0;
    link t;
    barrier_t *bar = args->bar;
    int *open_set_empty = args->open_set_empty;

    // -- Notation --
    // MY OPEN SET: open_lists[index], t_fvalues[index]
    // t[n] OPEN SET: open_lists[n], t_fvalues[n]

    // If i am the owner of the source node: set g(source) = 0, f(source) =
    // g(source) + h(source) = h(source)

    // Start HDA*
    while (1) {
        while (!PQempty(args->open_lists[args->index])) {
            // POP the node with min f(n)
            pthread_mutex_lock(&(args->mut_threads[args->index]));
            a = PQextractMin(args->open_lists[args->index],
                             args->t_fvalues[args->index]);
            pthread_mutex_unlock(&(args->mut_threads[args->index]));
#if COLLECT_STAT
            args->expanded_nodes[a]++;
#endif

            // NEW: duplicate check
            pthread_mutex_lock(&(args->mut_nodes[a]));
            if (args->closed_set[a] != -1 &&
                args->closed_set[a] <= args->gvalues[a]) {
                pthread_mutex_unlock(&(args->mut_nodes[a]));
                continue;
            }
            args->closed_set[a] = args->gvalues[a];
            pthread_mutex_unlock(&(args->mut_nodes[a]));

            // For each successor 'b' of node 'a':
            for (t = GRAPHget_list_node_head(args->G, a);
                 t != GRAPHget_list_node_tail(args->G, a);
                 t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);

                // Compute the owner of a (k_a == index) and b (k_b)
                k_a = hash_function2(a, args->num_threads, args->V);
                k_b = hash_function2(b, args->num_threads, args->V);

                // Acquire the lock for vertex a and take g(a)
                pthread_mutex_lock(&(args->mut_nodes[a]));
                g_b = args->gvalues[a] + a_b_wt;
                pthread_mutex_unlock(&(args->mut_nodes[a]));

                f_b = g_b + args->hvalues[b];

                // NEW: duplicate check
                pthread_mutex_lock(&(args->mut_nodes[b]));
                if (args->closed_set[b] != -1 && args->closed_set[b] <= g_b) {
                    pthread_mutex_unlock(&(args->mut_nodes[b]));
                    continue;
                }
                pthread_mutex_unlock(&(args->mut_nodes[b]));

                // Update gvalues, fvalues, parentVertex, costToCome
                pthread_mutex_lock(&(args->mut_nodes[b]));
                if (g_b < args->gvalues[b]) {
                    // Modify shared data structures
                    args->parentVertex[b] = a;
                    args->costToCome[b] = a_b_wt;
                    args->gvalues[b] = g_b;
                    pthread_mutex_lock(&(args->mut_threads[k_b]));
                    args->t_fvalues[k_b][b] = f_b;
                    PQinsert(args->open_lists[k_b], args->t_fvalues[k_b], b);
                    pthread_mutex_unlock(&(args->mut_threads[k_b]));
                }
                pthread_mutex_unlock(&(args->mut_nodes[b]));
            }
        }

        // Check if open set is empty and global costToCome to dest is < maxWT
        // -> hit the barrier
        if (PQempty(args->open_lists[args->index]) &&
            args->parentVertex[args->dest] != -1) {
            pthread_mutex_lock(&bar->mutex);
            bar->count++;
            if (bar->count == args->num_threads) {
                for (i = 0; i < args->num_threads; i++) sem_post(&bar->sem1);
            }
            pthread_mutex_unlock(&bar->mutex);
            sem_wait(&bar->sem1);
            // Here if: all threads hit the barrier

            // Now check if all the open set is still empty or not
            if (PQempty(args->open_lists[args->index])) {
                open_set_empty[args->index] = 1;
            } else {
                open_set_empty[args->index] = 0;
            }

            pthread_mutex_lock(&bar->mutex);
            bar->count--;
            if (bar->count == 0) {
                for (i = 0; i < args->num_threads; i++) sem_post(&bar->sem2);
            }
            pthread_mutex_unlock(&bar->mutex);
            sem_wait(&bar->sem2);

            // If all the threads have the message queue empty terminate
            int count = 0;
            for (i = 0; i < args->num_threads; i++) {
                count += open_set_empty[i];
            }
            if (count == args->num_threads) {
                break;
            }
        }
    }
    pthread_exit(NULL);
}
void ASTARshortest_path_sas_b(Graph G, int source, int dest,
                              char heuristic_type, int num_threads) {
    printf("## SAS-B A* [heuristic: %c] from %d to %d ##\n", heuristic_type,
           source, dest);
    int V = GRAPHget_num_nodes(G);
    int i, j, v, num_threads_nodes, *parentVertex;
    pthread_mutex_t *mut_threads;
    pthread_mutex_t *mut_nodes;
    double *hvalues, *gvalues, *costToCome, *closed_set;
    double tot_cost;
    int *open_set_empty;
    pthread_t *threads;
    struct arg_t *args;
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    num_threads_nodes = (int)ceil(((float)V) / num_threads);

    // Matrix of priority queues
    PQ *open_lists = (PQ *)malloc(num_threads * sizeof(PQ));
    for (i = 0; i < num_threads; i++) {
        open_lists[i] = (PQ)malloc(
            num_threads_nodes * (sizeof(int *) + sizeof(int *) + sizeof(int)));
        open_lists[i] = PQinit(V);
    }

    // Matrix of priorities(fvalues) for the PQs
    double **t_fvalues = (double **)malloc(num_threads * sizeof(double *));
    for (i = 0; i < num_threads; i++) {
        t_fvalues[i] = (double *)malloc(V * sizeof(double));
        for (j = 0; j < V; j++) {
            t_fvalues[i][j] = maxWT;
        }
    }

    // Allocate the barrier
    barrier_t *bar = (barrier_t *)malloc(1 * sizeof(barrier_t));
    sem_init(&bar->sem1, 0, 0);
    sem_init(&bar->sem2, 0, 0);
    pthread_mutex_init(&bar->mutex, NULL);
    bar->count = 0;

    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    args = (struct arg_t *)malloc(num_threads * sizeof(struct arg_t));
    mut_threads =
        (pthread_mutex_t *)malloc(num_threads * sizeof(pthread_mutex_t));
    mut_nodes = (pthread_mutex_t *)malloc(V * sizeof(pthread_mutex_t));
    parentVertex = (int *)malloc(V * sizeof(int));
    hvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    costToCome = (double *)malloc(V * sizeof(double));
    open_set_empty = (int *)malloc(num_threads * sizeof(int));
    closed_set = (double *)malloc(V * sizeof(double));
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
    if (expanded_nodes == NULL) {
        return NULL;
    }
#endif
    if ((parentVertex == NULL) || (mut_threads == NULL) || (hvalues == NULL) ||
        (gvalues == NULL) || (costToCome == NULL) || (threads == NULL) ||
        (args == NULL) || (open_set_empty == NULL) || (closed_set == NULL))
        return;

    for (v = 0; v < V; v++) {
        parentVertex[v] = -1;
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic(p, pos_dest, heuristic_type);
        gvalues[v] = maxWT;
        closed_set[v] = -1.0;
        pthread_mutex_init(&mut_nodes[v], NULL);
    }
    gvalues[source] = 0;
    t_fvalues[hash_function2(source, num_threads, V)][source] =
        compute_f(hvalues[source], 0);
    PQinsert(open_lists[hash_function2(source, num_threads, V)],
             t_fvalues[hash_function2(source, num_threads, V)], source);

    for (i = 0; i < num_threads; i++) pthread_mutex_init(&mut_threads[i], NULL);

    for (i = 0; i < num_threads; i++) {
        args[i].index = i;
        args[i].num_threads = num_threads;
        args[i].open_lists = open_lists;
        args[i].mut_threads = mut_threads;
        args[i].V = V;
        args[i].G = G;
        args[i].source = source;
        args[i].dest = dest;
        args[i].parentVertex = parentVertex;
        args[i].hvalues = hvalues;
        args[i].gvalues = gvalues;
        args[i].costToCome = costToCome;
        args[i].bar = bar;
        args[i].open_set_empty = open_set_empty;
        args[i].t_fvalues = t_fvalues;
        args[i].mut_nodes = mut_nodes;
        args[i].closed_set = closed_set;
#if COLLECT_STAT
        args[i].expanded_nodes = expanded_nodes;
#endif
        pthread_create(&threads[i], NULL, hda, (void *)&args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        free(open_lists[i]);
    }
    if (gvalues[dest] < maxWT)
        reconstruct_path(parentVertex, source, dest, costToCome);
    else
        printf("Path not found\n");
#if COLLECT_STAT
    int n = 0;
    int tot = 0;
    FILE *fp = fopen("./stats/stat_astar_sas_b.txt", "w+");
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
    free(threads);
    free(args);
    free(mut_threads);
    free(open_lists);
    free(parentVertex);
    free(hvalues);
    free(gvalues);
    free(costToCome);
    free(bar);

    return;
}
