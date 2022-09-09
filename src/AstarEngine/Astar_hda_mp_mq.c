#include <errno.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "Astar.h"

#define DEBUG_ASTAR 0
#define maxWT DBL_MAX
#define L 50
#define MSG_SIZE (L * sizeof(char))
extern int errno;
// MP utility functions
static key_t get_key();
static int get_msgqid(key_t key);
static void *hda_mp_mq(void *arg);
struct msg_s {
    long msg_type;
    char msg_payload[L];
};
struct node_info_s {
    int node;
    double f_node;
    double g_node;
    int parent_node;
};
struct reconstruct_info_s {
    int node;
};
static void prepare_msg_payload(struct node_info_s *i, int node, double f_node,
                                double g_node, int parent_node);
static int send_reconstruct_path_msg(int node, int msgqid, int msgtype);
static int send_msg(struct node_info_s info, int msgqid, long msgtype);
static int receive_msg(struct node_info_s *i, int msgqid, long msgtype);
static int receive_reconstruct_path_msg(int *node, int msgqid, int msgtype);
static long thread_to_msgtype(int id);
static int msgtype_to_thread(long type);
static void get_path(int *path_to_dest, int source, int dest);
static void enqueue_node(int node, int parent_node, int f_node, int g_node,
                         int *fvalues, int *gvalues, int *parentVertex, PQ pq);

static void enqueue_node(int node, int parent_node, int f_node, int g_node,
                         int *fvalues, int *gvalues, int *parentVertex, PQ pq) {
    if (g_node < gvalues[node]) {
        fvalues[node] = f_node;
        gvalues[node] = g_node;
        parentVertex[node] = parent_node;
    }
#if DEBUG_ASTAR
    printf("t[] PUSH node %d with parent %d in open set\n", node, parent_node);
#endif
    PQinsert(pq, fvalues, node);
}
static void check_and_update(int a, int dest, double g_a,
                             double *best_dest_cost, pthread_mutex_t *m);
static key_t get_key() { return ftok(".", 65); }
static int get_msgqid(key_t key) { return msgget(key, IPC_CREAT | 0666); }
static void prepare_msg_payload(struct node_info_s *i, int node, double f_node,
                                double g_node, int parent_node) {
    i->node = node;
    i->f_node = f_node;
    i->g_node = g_node;
    i->parent_node = parent_node;
}
static int send_reconstruct_path_msg(int node, int msgqid, int msgtype) {
    struct msg_s msg;
    int ret;

    sprintf(msg.msg_payload, "%d", node);
    msg.msg_type = msgtype;
    ret = msgsnd(msgqid, &msg, MSG_SIZE, IPC_NOWAIT);
    return ret;
}
static int send_msg(struct node_info_s info, int msgqid, long msgtype) {
    struct msg_s msg;
    int ret;

    sprintf(msg.msg_payload, "%d %lf %lf %d", info.node, info.f_node,
            info.g_node, info.parent_node);
    msg.msg_type = msgtype;
    ret = msgsnd(msgqid, &msg, MSG_SIZE, IPC_NOWAIT);
    return ret;
}
static int receive_msg(struct node_info_s *i, int msgqid, long msgtype) {
    struct msg_s msg;
    struct node_info_s info;
    int ret;

    ret = msgrcv(msgqid, &msg, MSG_SIZE, msgtype, IPC_NOWAIT);
    sscanf(msg.msg_payload, "%d %lf %lf %d", &i->node, &i->f_node, &i->g_node,
           &i->parent_node);
    return ret;
}
static int receive_reconstruct_path_msg(int *node, int msgqid, int msgtype) {
    struct msg_s msg;
    int ret;

    ret = msgrcv(msgqid, &msg, MSG_SIZE, msgtype, IPC_NOWAIT);
    sscanf(msg.msg_payload, "%d", node);
    return ret;
}
static long thread_to_msgtype(int id) { return id + 1; }
static int msgtype_to_thread(long type) { return type - 1; }
typedef struct barrier_s {
    sem_t sem1, sem2;
    pthread_mutex_t mutex;
    int count;
} barrier_t;
struct thread_arg_s {
    int id;
    int V;
    int dest;
    int source;
    int num_threads;
    double *hvalues;
    double *best_dest_cost;
    barrier_t *barrier;
    int *msg_queue_empty;
    Graph G;
    int *path_to_dest;
    pthread_mutex_t *m;
    int *finalParentVertex;
#if COLLECT_STAT
    int *expanded_nodes;
#endif
};
void ASTARshortest_path_hda_mp_mq(Graph G, int source, int dest,
                                  char heuristic_type, int num_threads) {
    printf("## HDA* MP-MQ [heuristic: %c] from %d to %d ##\n", heuristic_type,
           source, dest);

    pthread_t *threads;
    struct thread_arg_s *threads_arg;
    key_t key;
    int msgqid, i, v;
    int thread_source_owner;
    struct node_info_s source_info;
    int V = GRAPHget_num_nodes(G);
    Position pos_source = GRAPHget_node_position(G, source);
    Position pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    double *hvalues;
    double best_cost = maxWT;
    int *msg_queue_empty;
    struct msqid_ds stat_msgq;
    int *path_to_dest;
    pthread_mutex_t *m;
    int finalParentVertex = 0;

    // Allocate mutex
    m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m, NULL);

    // Allocate barrier
    barrier_t *barrier = (barrier_t *)malloc(1 * sizeof(barrier_t));
    sem_init(&barrier->sem1, 0, 0);
    sem_init(&barrier->sem2, 0, 0);
    pthread_mutex_init(&barrier->mutex, NULL);
    barrier->count = 0;

    // Prepare message queue
    system("ipcrm --all=msg;");
    msgqid = get_msgqid((key = get_key()));
    if (key == -1 || msgqid == -1) {
        printf("ERROR = %d\n", errno);
        return;
    }
    msgctl(msgqid, IPC_STAT, &stat_msgq);
    printf("[INFO] Message Queue - max bytes = %d\n", stat_msgq.msg_qbytes);

    // Send message with source node to the source's owner thread
    thread_source_owner = hash_function(source, num_threads);
    prepare_msg_payload(
        &source_info, source,
        compute_f(heuristic(pos_source, pos_dest, heuristic_type), 0), 0, -1);
    send_msg(source_info, msgqid, thread_to_msgtype(thread_source_owner));

    // Dynamic allocate data structures
    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    threads_arg = (struct thread_arg_s *)malloc(num_threads *
                                                sizeof(struct thread_arg_s));
    hvalues = (double *)malloc(V * sizeof(double));
    msg_queue_empty = (int *)malloc(num_threads * sizeof(int));
    path_to_dest = (int *)malloc(V * sizeof(int));
#if COLLECT_STAT
    int *expanded_nodes = (int *)calloc(V, sizeof(int));
    if (expanded_nodes == NULL) {
        return NULL;
    }
#endif
    if (threads == NULL || threads_arg == NULL || hvalues == NULL ||
        m == NULL || path_to_dest == NULL) {
        return;
    }
    // Initialize heuristic of each node
    for (v = 0; v < V; v++) {
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic(p, pos_dest, heuristic_type);
    }

    // Launch num_threads threads
    for (i = 0; i < num_threads; i++) {
        threads_arg[i].id = i;
        threads_arg[i].V = V;
        threads_arg[i].dest = dest;
        threads_arg[i].source = source;
        threads_arg[i].G = G;
        threads_arg[i].num_threads = num_threads;
        threads_arg[i].hvalues = hvalues;
        threads_arg[i].best_dest_cost = &best_cost;
        threads_arg[i].barrier = barrier;
        threads_arg[i].msg_queue_empty = msg_queue_empty;
        threads_arg[i].path_to_dest = path_to_dest;
        threads_arg[i].m = m;
        threads_arg[i].finalParentVertex = &finalParentVertex;
#if COLLECT_STAT
        threads_arg[i].expanded_nodes = expanded_nodes;
#endif
        pthread_create(&threads[i], NULL, hda_mp_mq, (void *)&threads_arg[i]);
    }
    // Join threads
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("+-----------------------------------+\n");
    if (best_cost < maxWT) {
        printf("Path from %d to %d: [ ", source, dest);
        get_path(path_to_dest, source, dest);
        printf("]");

        printf("\nCost: %.2lf\n", best_cost);
    } else {
        printf("Path from %d to %d not found.\n", source, dest);
    }
    printf("+-----------------------------------+\n");
#if COLLECT_STAT
    int n = 0;
    int tot = 0;
    FILE *fp = fopen("./stats/stat_astar_hda_mp_mq.txt", "w+");
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
    free(threads_arg);
    free(hvalues);
    free(barrier);
    free(msg_queue_empty);
    free(path_to_dest);
    free(m);
    msgctl(msgqid, IPC_RMID, NULL);
    return;
}

static void *hda_mp_mq(void *arg) {
    struct thread_arg_s *targ = (struct thread_arg_s *)arg;
    key_t key;
    int msgqid;
    int id = targ->id;
    int V = targ->V;
    int dest = targ->dest;
    int source = targ->source;
    Graph G = targ->G;
    int num_threads = targ->num_threads;
    double *best_dest_cost = targ->best_dest_cost;
    double *hvalues = targ->hvalues;
    barrier_t *barrier = targ->barrier;
    int *msg_queue_empty = targ->msg_queue_empty;
    int *finalParentVertex = targ->finalParentVertex;
    pthread_mutex_t *m = targ->m;
    char msg[L];
    struct node_info_s message;
    int res;
    double *fvalues, *gvalues, *costToCome;
    double f_extracted_node, a_b_wt, g_b, f_b;
    int i, a, b, k;
    int *parentVertex;
    int terminate = 0;
    link t;
    int rec_node;

    // Initialize the open set and the closed set
    PQ open_set = PQinit(V);
    double *closed_set = (double *)malloc(V * sizeof(double));

    // Allocate arrays and initialize data structures
    fvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    costToCome = (double *)malloc(V * sizeof(double));
    parentVertex = (int *)malloc(V * sizeof(int));
    if (gvalues == NULL || gvalues == NULL || costToCome == NULL ||
        parentVertex == NULL)
        pthread_exit(NULL);
    for (i = 0; i < V; i++) {
        fvalues[i] = maxWT;
        gvalues[i] = maxWT;
        closed_set[i] = (double)-1;
        costToCome[i] = maxWT;
        parentVertex[i] = -1;
    }

    // Prepare message queue
    msgqid = get_msgqid((key = get_key("keyfile")));
    if (msgqid == -1) {
        printf("Message queue error\n");
        pthread_exit(NULL);
    }
#if DEBUG_ASTAR
    printf("Start t[%d]\n", id);
#endif
    while (1) {
        // Try polling message queue --> improvement: one condition variable for
        // each thread that is incremented by 1 after each msgsnd()
        do {
            res = receive_msg(&message, msgqid, thread_to_msgtype(id));
            // If message queue is not empty add each message to the OPEN LIST
            if (res != -1) {
#if DEBUG_ASTAR
                printf("t[%d] received a message about node %d\n", id,
                       message.node);
#endif
                if (message.g_node < gvalues[message.node]) {
                    fvalues[message.node] = message.f_node;
                    gvalues[message.node] = message.g_node;
                    parentVertex[message.node] = message.parent_node;
                    costToCome[message.node] = message.g_node;
                    PQinsert(open_set, fvalues, message.node);
                    break;
                }
            }
        } while (res != -1);
        // If the OPEN SET is empty and path < DBL_MAX we can terminate -->
        if (PQempty(open_set) && *best_dest_cost < maxWT) {
#if DEBUG_ASTAR
            printf("--t[%d] enters the barrier--\n", id);
#endif
            pthread_mutex_lock(&barrier->mutex);
            barrier->count++;
            if (barrier->count == num_threads) {
                for (i = 0; i < num_threads; i++) sem_post(&barrier->sem1);
            }
            pthread_mutex_unlock(&barrier->mutex);
            sem_wait(&barrier->sem1);
            // Here if: all threads hit the barrier
#if DEBUG_ASTAR
            printf("t[%d] says: all threads hit the barrier--\n", id);
#endif
            // Now chek if message queue is still empty or not

            res = receive_msg(&message, msgqid, thread_to_msgtype(id));
            if (res == -1) {
                // Empty: write 1 in shared array (i am ready to stop)
                msg_queue_empty[id] = 1;
            } else {
                // Not empty: push the node inside open list and go on
                if (message.g_node < gvalues[message.node]) {
                    fvalues[message.node] = message.f_node;
                    gvalues[message.node] = message.g_node;
                    parentVertex[message.node] = message.parent_node;
                    costToCome[message.node] = message.g_node;
                    PQinsert(open_set, fvalues, message.node);
                }
                msg_queue_empty[id] = 0;
            }

#if DEBUG_ASTAR
            if (res == -1) {
                printf("t[%d] after barrier: message queue empty\n", id);
            } else {
                printf("t[%d] after barrier: message queue NOT empty\n", id);
                printf("t[%d] after barrier: PUSH node %d in open set\n", id,
                       message.node);
            }
#endif
            pthread_mutex_lock(&barrier->mutex);
            barrier->count--;
            if (barrier->count == 0) {
                for (i = 0; i < num_threads; i++) sem_post(&barrier->sem2);
            }
            pthread_mutex_unlock(&barrier->mutex);
            sem_wait(&barrier->sem2);

            // If all the threads have the message queue empty terminate
            int tot = 0;
            for (i = 0; i < num_threads; i++) tot += msg_queue_empty[i];
            if (tot == num_threads) {
#if DEBUG_ASTAR
                printf(
                    "Exit t[%d]: best path to %d with cost %.4lf and parent: "
                    "%d\n",
                    id, dest, *best_dest_cost, parentVertex[dest]);
#endif
                break;
            }
        }

        while (!PQempty(open_set)) {
            // Pop node with min(f(n))
            f_extracted_node = fvalues[a = PQextractMin(open_set, fvalues)];
#if COLLECT_STAT
            targ->expanded_nodes[a]++;
#endif
            if (a == dest) {
                pthread_mutex_lock(m);
                if (gvalues[a] < *best_dest_cost) *best_dest_cost = gvalues[a];
                pthread_mutex_unlock(m);
            }

            // Expand the node a
            for (t = GRAPHget_list_node_head(G, a);
                 t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);

                // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
                g_b = gvalues[a] + a_b_wt;
                f_b = g_b + hvalues[b];

                if (g_b > gvalues[b]) continue;

                // Send a message to b's owner thread
                k = hash_function(b, num_threads);

                // If i am the owner of the node push it in the open set
                if (k == id) {
                    fvalues[b] = f_b;
                    gvalues[b] = g_b;
                    parentVertex[b] = a;
                    costToCome[b] = g_b;
                    PQinsert(open_set, fvalues, b);
                } else {
                    prepare_msg_payload(&message, b, f_b, g_b, a);
                    if (send_msg(message, msgqid, thread_to_msgtype(k)) == -1) {
                        printf("# !! Message queue full !! ##\n");
                        pthread_exit(NULL);
                    }
                }
            }
        }
    }

    // Here when the thread has finished its work: now path reconstruction
    if (*best_dest_cost < maxWT) {
        // If i am the the owner of the destination node
        if (id == hash_function(dest, num_threads)) {
            targ->path_to_dest[dest] = parentVertex[dest];
            // Compute the owner of parentVertex[dest]
            k = hash_function(parentVertex[dest], num_threads);
            // Ask: who is the parent of parentVertex[dest]?
            send_reconstruct_path_msg(parentVertex[dest], msgqid,
                                      thread_to_msgtype(k));
        }
        // Listen for messages
        while (*finalParentVertex == 0) {
            res = receive_reconstruct_path_msg(&rec_node, msgqid,
                                               thread_to_msgtype(id));
            if (res != -1) {
                if (rec_node == source) {
                    targ->path_to_dest[rec_node] = -1;
                    *finalParentVertex = 1;
                    break;
                } else {
                    targ->path_to_dest[rec_node] = parentVertex[rec_node];
                    // Compute the owner of parentVertex[rec_node]
                    k = hash_function(parentVertex[rec_node], num_threads);
                    // Ask: who is the parent of parentVertex[rec_node]?
                    send_reconstruct_path_msg(parentVertex[rec_node], msgqid,
                                              thread_to_msgtype(k));
                }
            }
        }
    }

    free(fvalues);
    free(gvalues);
    free(parentVertex);
    free(costToCome);
    PQfree(open_set);
    pthread_exit(NULL);
}

static void check_and_update(int a, int dest, double g_a,
                             double *best_dest_cost, pthread_mutex_t *m) {
    pthread_mutex_lock(m);
    if (a == dest && g_a < *best_dest_cost) {
        *best_dest_cost = g_a;
        printf("Updated cost: %lf\n", *best_dest_cost);
    }
    pthread_mutex_unlock(m);
}
static void get_path(int *path_to_dest, int source, int dest) {
    if (source == dest) {
        printf("%d ", source);
        return;
    } else {
        get_path(path_to_dest, source, path_to_dest[dest]);
        printf("%d ", dest);
    }
}