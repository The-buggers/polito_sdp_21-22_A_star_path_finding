#include <float.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>

#include "./Astar.h"

#define maxWT DBL_MAX
#define SHMEM_SIZE 1024 * 1024  //* 1024
#define DEBUG_ASTAR 0

struct arg_t {
    int index;
    int num_threads;
    int V;
    Graph G;
    struct mess_t **data;
    int source;
    int dest;
    int *open_set_empty;
    char heuristic_type;
    double *best_dest_cost;
    double *hvalues;
    double *costToCome;
    int *parentVertex;
    sem_t *m;
    sem_t *r;
    sem_t *meR;
    int *nR;
    sem_t *w;
    sem_t *meW;
    int *nW;
#if COLLECT_STAT
    int *expanded_nodes;
#endif
};

struct mess_t {
    int n;
    double g;
    double a_b_wt;
    int prev;
};

static void *hda_mp_sm(void *arg);

static void *hda_mp_sm(void *arg) {
    struct arg_t *args = (struct arg_t *)arg;
    int i, a, b, n, k, count;
    double g_b, f_b, a_b_wt;
    link t;
    double *gvalues, *fvalues;
    PQ open_list;
    struct mess_t *data = *args->data;

    open_list = PQinit(args->V);
    fvalues = (double *)malloc(args->V * sizeof(double));
    gvalues = (double *)malloc(args->V * sizeof(double));

    if ((gvalues == NULL) || (fvalues == NULL)) pthread_exit(NULL);

    for (i = 0; i < args->V; i++) {
        gvalues[i] = maxWT;
        fvalues[i] = maxWT;
    }

    if (hash_function2(args->source, args->num_threads, args->V) ==
        args->index) {
        gvalues[args->source] = 0;
        fvalues[args->source] = compute_f(args->hvalues[args->source], 0);
        PQinsert(open_list, fvalues, args->source);
    }

    // Start HDA*
    while (1) {
        while (data != (*args->data)) {
            sem_wait(args->r);
            sem_wait(args->meR);
            (*args->nR)++;
            if (*(args->nR) == 1) sem_wait(args->w);
            sem_post(args->meR);
            sem_post(args->r);

            if (hash_function2(data->n, args->num_threads, args->V) ==
                args->index) {
                if (data->g < gvalues[data->n]) {
#if DEBUG_ASTAR
                    printf("T: %d Buff Extract: %d\n", args->index, data->n);
#endif
                    gvalues[data->n] = data->g;
                    fvalues[data->n] =
                        gvalues[data->n] + args->hvalues[data->n];
                    args->parentVertex[data->n] = data->prev;
                    args->costToCome[data->n] = data->a_b_wt;

                    PQinsert(open_list, fvalues, data->n);
#if DEBUG_ASTAR
                    printf("T: %d PQ Insert: %d\n", args->index, data->n);
#endif
                }
            }
            data++;

            sem_wait(args->meR);
            (*args->nR)--;
            if (*(args->nR) == 0) sem_post(args->w);
            sem_post(args->meR);
        }

        if (PQempty(open_list) && (*args->best_dest_cost) < maxWT) {
            args->open_set_empty[args->index] = 1;
            for (i = 0, count = 0; i < args->num_threads; i++)
                count += args->open_set_empty[i];
            if (count == args->num_threads) break;
        }

        if (!PQempty(open_list)) {
            args->open_set_empty[args->index] = 0;
            // POP the node with min f(n)
            a = PQextractMin(open_list, fvalues);
#if COLLECT_STAT
            args->expanded_nodes[a]++;
#endif
#if DEBUG_ASTAR
            printf("T: %d PQ Extract: %d\n", args->index, a);
#endif
            if (a == args->dest) {
                sem_wait(args->m);
                if (gvalues[a] < *(args->best_dest_cost))
                    *(args->best_dest_cost) = gvalues[a];
                sem_post(args->m);
            }

            // For each successor 'b' of node 'a':
            for (t = GRAPHget_list_node_head(args->G, a);
                 t != GRAPHget_list_node_tail(args->G, a);
                 t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);

                g_b = gvalues[a] + a_b_wt;

                if (g_b < gvalues[b]) {
                    k = hash_function2(b, args->num_threads, args->V);

                    if (k == args->index) {
                        gvalues[b] = g_b;
                        fvalues[b] = g_b + args->hvalues[b];
                        args->parentVertex[b] = a;
                        args->costToCome[b] = a_b_wt;

                        PQinsert(open_list, fvalues, b);
#if DEBUG_ASTAR
                        printf("T: %d PQ Insert: %d\n", args->index, b);
#endif
                    } else {
                        sem_wait(args->meW);
                        (*args->nW)++;
                        if (*(args->nW) == 1) sem_wait(args->r);
                        sem_post(args->meW);
                        sem_wait(args->w);

                        (*args->data)->n = b;
                        (*args->data)->g = g_b;
                        (*args->data)->a_b_wt = a_b_wt;
                        (*args->data)->prev = a;
                        (*args->data)++;

                        sem_post(args->w);
                        sem_wait(args->meW);
                        (*args->nW)--;
                        if (*(args->nW) == 0) sem_post(args->r);
                        sem_post(args->meW);
#if DEBUG_ASTAR
                        printf("T: %d Buff Insert: %d\n", args->index, b);
#endif
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
}

void ASTARshortest_path_hda_mp_sm(Graph G, int source, int dest,
                                  char heuristic_type, int num_threads) {
    printf("## HDA* MP-SM [heuristic: %c] from %d to %d ##\n", heuristic_type,
           source, dest);
    int V = GRAPHget_num_nodes(G);
    int i, j, shmid, *open_set_empty, nR = 0, nW = 0;
    pthread_t *threads;
    struct arg_t *args;
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    key_t key;
    struct mess_t *data;
    double *hvalues, *costToCome, best_dest_cost = maxWT;
    int *parentVertex;
    sem_t m;
    sem_t meW, meR;
    sem_t w, r;

    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    args = (struct arg_t *)malloc(num_threads * sizeof(struct arg_t));
    open_set_empty = (int *)malloc(num_threads * sizeof(int));
    parentVertex = (int *)malloc(V * sizeof(int));
    hvalues = (double *)malloc(V * sizeof(double));
    costToCome = (double *)malloc(V * sizeof(double));

    if ((parentVertex == NULL) || (hvalues == NULL) || (costToCome == NULL))
        return;
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
    if (expanded_nodes == NULL) {
        return NULL;
    }
#endif
    // Setup shared memory
    if ((key = ftok(".", 65)) == -1) return;
    if ((shmid = shmget(key, SHMEM_SIZE, 0644 | IPC_CREAT)) == -1) return;
    data = (struct mess_t *)shmat(shmid, NULL, 0);

    sem_init(&m, 0, 1);
    sem_init(&meR, 0, 1);
    sem_init(&r, 0, 1);
    sem_init(&w, 0, 1);
    sem_init(&meW, 0, 1);

    for (i = 0; i < V; i++) {
        parentVertex[i] = -1;
        p = GRAPHget_node_position(G, i);
        hvalues[i] = heuristic(p, pos_dest, heuristic_type);
    }
    for (i = 0; i < num_threads; i++) open_set_empty[i] = 0;

    for (i = 0; i < num_threads; i++) {
        args[i].index = i;
        args[i].num_threads = num_threads;
        args[i].V = V;
        args[i].G = G;
        args[i].m = &m;
        args[i].r = &r;
        args[i].w = &w;
        args[i].meR = &meR;
        args[i].meW = &meW;
        args[i].nR = &nR;
        args[i].nW = &nW;
        args[i].hvalues = hvalues;
        args[i].costToCome = costToCome;
        args[i].parentVertex = parentVertex;
        args[i].source = source;
        args[i].dest = dest;
        args[i].heuristic_type = heuristic_type;
        args[i].data = &data;
        args[i].best_dest_cost = &best_dest_cost;
        args[i].open_set_empty = open_set_empty;
#if COLLECT_STAT
        args[i].expanded_nodes = expanded_nodes;
#endif
        pthread_create(&threads[i], NULL, hda_mp_sm, (void *)&args[i]);
    }
    for (i = 0; i < num_threads; i++) pthread_join(threads[i], NULL);

    if (best_dest_cost < maxWT)
        reconstruct_path(parentVertex, source, dest, costToCome);
    else
        printf("Path not found\n");
#if COLLECT_STAT
    int n = 0;
    int tot = 0;
    FILE *fp = fopen("./stats/stat_astar_hda_mp_sm.txt", "w+");
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
    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
    free(threads);
    free(args);

    return;
}