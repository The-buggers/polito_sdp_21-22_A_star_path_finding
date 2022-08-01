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
    double **t_fvalues;
    pthread_mutex_t *mut_nodes;
};
static void *hda(void *arg);

static void *hda(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int i, v, a, b, k_a, k_b;
    Position p;
    double f_extracted_node, g_b, f_b, a_b_wt, tot_cost = 0;
    link t;
    barrier_t *bar = args->bar;
    int *open_set_empty = args->open_set_empty;

    // -- Notation --
    // MY OPEN SET: open_lists[index], t_fvalues[index]
    // t[n] OPEN SET: open_lists[n], t_fvalues[n]

    // If i am the owner of the source node: set g(source) = 0, f(source) =
    // g(source) + h(source) = h(source)
    if (hash_function(args->source, args->num_threads) == args->index) {
        args->t_fvalues[args->index][args->source] =
            compute_f(args->hvalues[args->source], 0);
        args->gvalues[args->source] = 0;
        PQinsert(args->open_lists[args->index], args->t_fvalues[args->index],
                 args->source);
    }
    while (1) {
        while (!PQempty(args->open_lists[args->index])) {
            // POP the node with min f(n)
            pthread_mutex_lock(&(args->mut_threads[args->index]));
            f_extracted_node =
                args->t_fvalues[args->index]
                               [a = PQextractMin(args->open_lists[args->index],
                                                 args->t_fvalues[args->index])];
            pthread_mutex_unlock(&(args->mut_threads[args->index]));

            // For each successor 'b' of node 'a':
            for (t = GRAPHget_list_node_head(args->G, a);
                 t != GRAPHget_list_node_tail(args->G, a);
                 t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);

                // Compute the owner of a (k_a == index) and b (k_b)
                k_a = hash_function(a, args->num_threads);
                k_b = hash_function(b, args->num_threads);

                // Acquire the lock for vertex a and take g(a)
                pthread_mutex_lock(&(args->mut_nodes[a]));
                g_b = args->gvalues[a] + a_b_wt;
                pthread_mutex_unlock(&(args->mut_nodes[a]));

                f_b = g_b + args->hvalues[b];

                // Update gvalues, fvalues, parentVertex, costToCome
                if (g_b < args->gvalues[b]) {
                    // Acuire the lock for vertex b and modify shared data
                    // structures
                    pthread_mutex_lock(&(args->mut_nodes[b]));
                    args->parentVertex[b] = a;
                    args->costToCome[b] = a_b_wt;
                    args->gvalues[b] = g_b;
                    args->t_fvalues[k_b][b] = f_b;
                    pthread_mutex_lock(&(args->mut_threads[k_b]));
                    // note: with PQchange the algorithm doesn't find best path
                    PQinsert(args->open_lists[k_b], args->t_fvalues[k_b], b);
                    pthread_mutex_unlock(&(args->mut_threads[k_b]));
                    pthread_mutex_unlock(&(args->mut_nodes[b]));
                }
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
#if DEBUG_ASTAR
            printf("t[%d] says: all threads hit the barrier--\n", args->index);
#endif
            // Now check if all the open set is still empty or not
            // pthread_mutex_lock(&open_set_empty_lk);
            if (PQempty(args->open_lists[args->index])) {
                open_set_empty[args->index] = 1;
            } else {
                open_set_empty[args->index] = 0;
            }
            // pthread_mutex_unlock(&open_set_empty_lk);

#if DEBUG_ASTAR
            if (PQempty(args->open_lists[args->index])) {
                printf("t[%d] after barrier: message queue empty\n",
                       args->index);
            } else {
                printf("t[%d] after barrier: message queue NOT empty\n",
                       args->index);
            }
#endif
            pthread_mutex_lock(&bar->mutex);
            bar->count--;
            if (bar->count == 0) {
                for (i = 0; i < args->num_threads; i++) sem_post(&bar->sem2);
            }
            pthread_mutex_unlock(&bar->mutex);
            sem_wait(&bar->sem2);

            // If all the threads had the message queue empty terminate
            // pthread_mutex_lock(&open_set_empty_lk);
            int count = 0;
            for (i = 0; i < args->num_threads; i++) {
                count += open_set_empty[i];
            }
            // pthread_mutex_unlock(&open_set_empty_lk);
            if (count == args->num_threads) {
#if DEBUG_ASTAR
                printf(
                    "Exit t[%d]: best path to %d with costToCome %.4lf and "
                    "parent: "
                    "%d\n",
                    args->index, args->dest, args->gvalues[args->dest],
                    args->parentVertex[args->dest]);
#endif
                break;
            }
        }
    }
    pthread_exit(NULL);
}
void ASTARshortest_path_sas_b(Graph G, int source, int dest, int num_threads) {
    printf("A star algorithm (SAS-2 version) on graph from %d to %d\n", source,
           dest);
    int V = GRAPHget_num_nodes(G);
    int i, j, v, num_threads_nodes, *parentVertex;
    pthread_mutex_t *mut_threads;
    pthread_mutex_t *mut_nodes;
    double *hvalues, *gvalues, *costToCome;
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
    if ((parentVertex == NULL) || (mut_threads == NULL) || (hvalues == NULL) ||
        (gvalues == NULL) || (costToCome == NULL) || (threads == NULL) ||
        (args == NULL) || (open_set_empty == NULL))
        return;

    for (v = 0; v < V; v++) {
        parentVertex[v] = -1;
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic_euclidean(p, pos_dest);
        gvalues[v] = maxWT;
        pthread_mutex_init(&mut_nodes[v], NULL);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_mutex_init(&mut_threads[i], NULL);
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