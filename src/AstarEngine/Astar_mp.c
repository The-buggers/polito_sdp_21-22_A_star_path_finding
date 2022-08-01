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
static void *astar_thread(void *arg);
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
static void prepare_msg_payload(struct node_info_s *i, int node, double f_node,
                                double g_node, int parent_node);
static int send_msg(struct node_info_s info, int msgqid, long msgtype);
static int receive_msg(struct node_info_s *i, int msgqid, long msgtype);
static long thread_to_msgtype(int id);
static int msgtype_to_thread(long type);
static int atomic_read(int *empty, int num_threads, pthread_mutex_t *m);
static void atomic_write(int *empty, int id, int val, pthread_mutex_t *m);

static key_t get_key() { return ftok(".", 65); }
static int get_msgqid(key_t key) { return msgget(key, IPC_CREAT | 0666); }
static void prepare_msg_payload(struct node_info_s *i, int node, double f_node,
                                double g_node, int parent_node) {
    i->node = node;
    i->f_node = f_node;
    i->g_node = g_node;
    i->parent_node = parent_node;
}
static int send_msg(struct node_info_s info, int msgqid, long msgtype) {
    struct msg_s msg;
    int ret;

    sprintf(msg.msg_payload, "%d %lf %lf %d", info.node, info.f_node,
            info.g_node, info.parent_node);
    msg.msg_type = msgtype;
    ret = msgsnd(msgqid, &msg, MSG_SIZE, IPC_NOWAIT);
#if DEBUG_MSG
    if (ret != -1)
        printf("Message [%s] sent from t[%d] to t[%d]!\n", msg.msg_payload,
               info.parent_node, msgtype_to_thread(msg.msg_type));
#endif
    return 1;
}
static int receive_msg(struct node_info_s *i, int msgqid, long msgtype) {
    struct msg_s msg;
    struct node_info_s info;
    int ret;

    ret = msgrcv(msgqid, &msg, MSG_SIZE, msgtype, IPC_NOWAIT);
    sscanf(msg.msg_payload, "%d %lf %lf %d", &i->node, &i->f_node, &i->g_node,
           &i->parent_node);
#if DEBUG_MSG
    if (ret != -1)
        printf("Message [%s] sent by t[%d] received by t[%d]!\n",
               msg.msg_payload, i->parent_node,
               msgtype_to_thread(msg.msg_type));
#endif
    return ret;
}
static long thread_to_msgtype(int id) { return id + 1; }
static int msgtype_to_thread(long type) { return type - 1; }
static int atomic_read(int *empty, int num_threads, pthread_mutex_t *m) {
    int tot = 0;
    int i;
    pthread_mutex_lock(m);
    for (i = 0; i < num_threads; i++) tot += empty[i];
    pthread_mutex_unlock(m);
    return tot;
}
static void atomic_write(int *empty, int id, int val, pthread_mutex_t *m) {
    pthread_mutex_lock(m);
    empty[id] = val;
    pthread_mutex_unlock(m);
}
typedef struct barrier_s {
    sem_t sem1, sem2;
    pthread_mutex_t mutex;
    int count;
} barrier_t;
struct thread_arg_s {
    int id;
    int V;
    int dest;
    int num_threads;
    double *hvalues;
    double *best_dest_cost;
    barrier_t *barrier;
    int *msg_queue_empty;
    pthread_mutex_t *lock_msg_queue_empty;
    Graph G;
};
void ASTARshortest_path_mp(Graph G, int source, int dest,
                                    int num_threads) {
    printf(
        "A star algorithm - parallel MP %d threads- on graph from %d to %d\n",
        num_threads, source, dest);

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

    // Allocate mutex for message queue
    pthread_mutex_t *m_msg_queue =
        (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m_msg_queue, NULL);

    // Allocate barrier
    barrier_t *barrier = (barrier_t *)malloc(1 * sizeof(barrier_t));
    sem_init(&barrier->sem1, 0, 0);
    sem_init(&barrier->sem2, 0, 0);
    pthread_mutex_init(&barrier->mutex, NULL);
    barrier->count = 0;

    // Prepare message queue
    msgqid = get_msgqid((key = get_key()));
    if (key == -1 || msgqid == -1) {
        printf("ERROR = %d\n", errno);
        return;
    }
    msgctl(msgqid, IPC_STAT, &stat_msgq);
    printf("[BEFORE] Message queue info - max bytes = %d\n",
           stat_msgq.msg_qbytes);

    // Send message with source node to the source's owner thread
    thread_source_owner = hash_function(source, num_threads);
    prepare_msg_payload(&source_info, source,
                        compute_f(heuristic_euclidean(pos_source, pos_dest), 0),
                        0, -1);
    send_msg(source_info, msgqid, thread_to_msgtype(thread_source_owner));

    // Dynamic allocate data structures
    threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    threads_arg = (struct thread_arg_s *)malloc(num_threads *
                                                sizeof(struct thread_arg_s));
    hvalues = (double *)malloc(V * sizeof(double));
    msg_queue_empty = (int *)malloc(num_threads * sizeof(int));
    if (threads == NULL || threads_arg == NULL || hvalues == NULL ||
        msg_queue_empty == NULL) {
        return;
    }
    // Initialize heuristic of each node
    for (v = 0; v < V; v++) {
        p = GRAPHget_node_position(G, v);
        hvalues[v] = heuristic_euclidean(p, pos_dest);
    }

    // Launch num_threads threads
    for (i = 0; i < num_threads; i++) {
        threads_arg[i].id = i;
        threads_arg[i].V = V;
        threads_arg[i].dest = dest;
        threads_arg[i].G = G;
        threads_arg[i].num_threads = num_threads;
        threads_arg[i].hvalues = hvalues;
        threads_arg[i].best_dest_cost = &best_cost;
        threads_arg[i].barrier = barrier;
        threads_arg[i].msg_queue_empty = msg_queue_empty;
        threads_arg[i].lock_msg_queue_empty = m_msg_queue;
        pthread_create(&threads[i], NULL, astar_thread,
                       (void *)&threads_arg[i]);
    }
    // Join threads
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Cost of the path found: %.4lf\n", best_cost);
    free(threads);
    free(threads_arg);
    free(hvalues);
    free(barrier);
    free(msg_queue_empty);
    free(m_msg_queue);
    msgctl(msgqid, IPC_RMID, NULL);
    return;
}

static void *astar_thread(void *arg) {
    struct thread_arg_s *targ = (struct thread_arg_s *)arg;
    key_t key;
    int msgqid;
    int id = targ->id;
    int V = targ->V;
    int dest = targ->dest;
    Graph G = targ->G;
    int num_threads = targ->num_threads;
    double *best_dest_cost = targ->best_dest_cost;
    double *hvalues = targ->hvalues;
    barrier_t *barrier = targ->barrier;
    int *msg_queue_empty = targ->msg_queue_empty;
    pthread_mutex_t *lock_msg_queue_empty = targ->lock_msg_queue_empty;
    char msg[L];
    struct node_info_s message;
    int res;
    double *fvalues, *gvalues, *costToCome;
    double f_extracted_node, a_b_wt, g_b, f_b;
    int i, a, b, k;
    int *parentVertex;
    int terminate = 0;
    link t;

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
    if (msgqid == -1) pthread_exit(NULL);

    while (1) {
        // Try polling message queue --> improvement: one condition variable for
        // each thread that is incremented by 1 after each msgsnd()
        do {
            res = receive_msg(&message, msgqid, thread_to_msgtype(id));
            // If message queue is not empty add each message to the OPEN LIST
            if (res != -1) {
                fvalues[message.node] = message.f_node;
                gvalues[message.node] = message.g_node;
                parentVertex[message.node] = message.parent_node;
#if DEBUG_ASTAR
                printf("t[%d] PUSH node %d in open set\n", id, message.node);
#endif
                PQinsert(open_set, fvalues, message.node);
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
                atomic_write(msg_queue_empty, id, 1, lock_msg_queue_empty);
            } else {
                // Not empty: push the node inside open list and go on
                fvalues[message.node] = message.f_node;
                gvalues[message.node] = message.g_node;
                parentVertex[message.node] = message.parent_node;
                PQinsert(open_set, fvalues, message.node);
                atomic_write(msg_queue_empty, id, 0, lock_msg_queue_empty);
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
            if (atomic_read(msg_queue_empty, num_threads,
                            lock_msg_queue_empty) == num_threads) {
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
#if DEBUG_ASTAR
            printf("t[%d] POP node %d from open set\n", id, a);
#endif
            // Duplicate check
            if (closed_set[a] != -1 && closed_set[a] <= gvalues[a]) continue;

            // [Destination check ? here ?]
            if (a == dest && gvalues[a] < costToCome[dest]) {
#if DEBUG_ASTAR
                printf("Pathfound by t[%d] - cost=%.4lf\n", id,
                       *best_dest_cost);
#endif
            }

            // Insert node in closed set
            closed_set[a] = gvalues[a];

            // Expand the node a
            for (t = GRAPHget_list_node_head(G, a);
                 t != GRAPHget_list_node_tail(G, a); t = LINKget_next(t)) {
                b = LINKget_node(t);
                a_b_wt = LINKget_wt(t);

                // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
                g_b = gvalues[a] + a_b_wt;
                f_b = g_b + hvalues[b];

                if (g_b > costToCome[b]) continue;

                // Update local tables
                costToCome[b] = g_b;
                parentVertex[b] = a;

                // Check if it is dest node
                if (b == dest && costToCome[b] < *best_dest_cost) {
                    printf("Found %lf\n", *best_dest_cost);
                    *best_dest_cost = costToCome[b];
                }

                // Send a message to b's owner thread
                k = hash_function(b, num_threads);

                // If i am the owner of the node push it in the open set
                if (k == id) {
                    fvalues[b] = f_b;
                    gvalues[b] = g_b;
#if DEBUG_ASTAR
                    printf("t[%d] owner PUSH node %d in open set\n", id, b);
#endif
                    PQinsert(open_set, fvalues, b);
                } else {
                    prepare_msg_payload(&message, b, f_b, g_b, a);
                    send_msg(message, msgqid, thread_to_msgtype(k));
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