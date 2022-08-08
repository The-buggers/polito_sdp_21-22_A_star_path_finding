/** @file Graph.c
 *  @brief Graph functions and utilities
 *  
 *  This file contains the definition of the I class ADT Graph and
 *  the related data structures to work with it. Among these functions
 *  the ones used for sequential and parallel reading of the input file
 *  are present
 * 
 *  @author Mattia Rosso
 *  @author Lorenzo Ippolito
 *  @author Fabio  Mirto
 */
#include "Graph.h"

#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "PQ.h"
#include "Position.h"
#define maxWT DBL_MAX
#define MAXC 10
/**
 * @brief Node(vertex) of the graph
 * 
 */
struct node {
    int v;
    double wt;
    link next;
};
/**
 * @brief Graph - I class ADT
 * 
 */
struct graph {
    int V;
    int E;
    link *ladj;
    ST tab;
    link z;
};
/**
 * @brief Data type compatible with the binary format of a NODE line 
 * 
 */
struct node_line_s {
    int index;
    double x;
    double y;
};
/**
 * @brief Data type compatible with the binary format of a EDGE line 
 * 
 */
struct edge_line_s {
    int id1;
    int id2;
    double wt;
};

// ################### PROTOTYPES ######################
static Edge EDGEcreate(int v, int w, double wt);
static link NEW(int v, double wt, link next);
static void insertE(Graph G, Edge e);
static void removeE(Graph G, Edge e);
static void reconstruct_path_r(int *parentVertex, int j, double *costToCome,
                               double *tot_cost);

// Static function prototypes for parallel read
static void parallelIo(void *arg);          // version 2
static void *thread_read(void *arg);        // version 3
static void *GRAPHloadParallel(void *arg);  // version 1
// #####################################################

// Utily functions for a_star implementation
int GRAPHget_num_nodes(Graph G) { return G->V; }
Position GRAPHget_node_position(Graph G, int id) {
    return STsearchByIndex(G->tab, id);
}
link GRAPHget_list_node_head(Graph G, int v) { return G->ladj[v]; }
link GRAPHget_list_node_tail(Graph G, int v) { return G->z; }
link LINKget_next(link t) { return t->next; }
double LINKget_wt(link t) { return t->wt; }
int LINKget_node(link t) { return t->v; }
int GRAPHgetIndex(Graph G, char *label) {
    int id;
    id = STsearch(G->tab, label);
    if (id == -1) {
        id = STsize(G->tab);
        STinsert(G->tab, label, id);
    }
    return id;
}
void GRAPHinsertE(Graph G, int id1, int id2, double wt) {
    insertE(G, EDGEcreate(id1, id2, wt));
}
void GRAPHremoveE(Graph G, int id1, int id2) {
    removeE(G, EDGEcreate(id1, id2, 0));
}
static void insertE(Graph G, Edge e) {
    int v = e.v, w = e.w;
    double wt = e.wt;
    G->ladj[v] = NEW(w, wt, G->ladj[v]);
    G->E++;
}
static void removeE(Graph G, Edge e) {
    int v = e.v, w = e.w;
    link x;
    if (G->ladj[v]->v == w) {
        G->ladj[v] = G->ladj[v]->next;
        G->E--;
    } else
        for (x = G->ladj[v]; x != G->z; x = x->next)
            if (x->next->v == w) {
                x->next = x->next->next;
                G->E--;
            }
}
void GRAPHedges(Graph G, Edge *a) {
    int v, E = 0;
    link t;
    for (v = 0; v < G->V; v++)
        for (t = G->ladj[v]; t != G->z; t = t->next)
            a[E++] = EDGEcreate(v, t->v, t->wt);
}
// Linked list utility functions
static Edge EDGEcreate(int v, int w, double wt) {
    Edge e;
    e.v = v;
    e.w = w;
    e.wt = wt;
    return e;
}
static link NEW(int v, double wt, link next) {
    link x = malloc(sizeof *x);
    if (x == NULL) return NULL;
    x->v = v;
    x->wt = wt;
    x->next = next;
    return x;
}

// Graph management functions
Graph GRAPHinit(int V) {
    int v;
    Graph G = malloc(sizeof *G);
    if (G == NULL) return NULL;
    G->V = V;
    G->E = 0;
    G->z = NEW(-1, 0, NULL);
    if (G->z == NULL) return NULL;
    G->ladj = malloc(G->V * sizeof(link));
    if (G->ladj == NULL) return NULL;
    for (v = 0; v < G->V; v++) G->ladj[v] = G->z;
    G->tab = STinit(V);
    if (G->tab == NULL) return NULL;
    return G;
}
void GRAPHfree(Graph G) {
    int v;
    link t, next;
    for (v = 0; v < G->V; v++)
        for (t = G->ladj[v]; t != G->z; t = next) {
            next = t->next;
            free(t);
        }
    STfree(G->tab);
    free(G->ladj);
    free(G->z);
    free(G);
}

void GRAPHstore(Graph G, FILE *fout) {
    int i;
    Edge *a;
    a = malloc(G->E * sizeof(Edge));
    if (a == NULL) return;
    GRAPHedges(G, a);
    fprintf(fout, "Number of nodes: %d\n", G->V);
    Position p = STsearchByIndex(G->tab, 0);
    for (i = 0; i < G->V; i++) {
        fprintf(fout, "Node %d: ", i);
        POSITIONprint(STsearchByIndex(G->tab, i), fout);
    }
    for (i = 0; i < G->E; i++) {
        fprintf(fout, "Edge %d -> %d - Weight: %.1lf\n", a[i].v, a[i].w,
                a[i].wt);
    }
}

// ########################################
// ### SEQUENTIAL READ FROM BINARY FILE ###
// ########################################
Graph GRAPHload_sequential(char *filepath) {
    int V, i, id1, id2, fd, nR;
    char label1[MAXC], label2[MAXC];
    int node_index, node_x, node_y;
    double wt;
    Graph G;
    Position p;
    struct node_line_s nl;
    struct edge_line_s el;

    fd = open(filepath, O_RDONLY);
    read(fd, &V, sizeof(V));
    G = GRAPHinit(V);
    if (G == NULL) return NULL;
    for (i = 0; i < V; i++) {
        read(fd, &nl, sizeof(struct node_line_s));
        p = POSITIONinit(nl.x, nl.y);
        STinsert(G->tab, p, nl.index);
        POSITIONfree(p);
    }
    while ((nR = read(fd, &el, sizeof(struct edge_line_s))) > 0) {
        if (el.id1 >= 0 && el.id2 >= 0) GRAPHinsertE(G, el.id1, el.id2, el.wt);
    }

    return G;
}
// #################################
// ### PARALLEL READ - VERSION 3 ###
// #################################

// Thread argument data structure
struct thread_arg_s {
    char filepath[50];
    int num_threads;
    int num_partitions;
    ssize_t filesize;
    ssize_t regionsize;
    int index;
    ssize_t linesize;
    off_t start_offset;
    char linetype;
    Graph *G;
    pthread_mutex_t *m_nodes;
    pthread_mutex_t *m_edges;
};

// Defines the partition(block) to be read from one thread
struct read_block_s {
    off_t start;
    off_t end;
    off_t cur;
    ssize_t size;
};
/**
 * @brief Thread function for reading file
 * 
 * @param arg thread argument
 * @return void* 
 */
static void *thread_read(void *arg) {
    struct thread_arg_s *targ = (struct thread_arg_s *)arg;
    int index = targ->index;
    int fd;
    ssize_t filesize = targ->filesize;
    ssize_t regionsize = targ->regionsize;
    ssize_t linesize = targ->linesize;
    off_t start_offset = targ->start_offset;
    int num_thread = targ->num_threads;
    struct read_block_s rb;
    off_t nodes_file_region_end;
    struct edge_line_s el;
    struct node_line_s nl;
    char linetype = targ->linetype;
    Position p;
    Graph G = *(targ->G);
    pthread_mutex_t *m_nodes = targ->m_nodes;
    pthread_mutex_t *m_edges = targ->m_edges;

    // Open the file (each thread works on its own file descriptor)
    fd = open(targ->filepath, O_RDONLY);

    // Compute (first) portion of file to read
    rb.size = targ->regionsize / targ->num_partitions;
    if (rb.size < linesize)
        rb.size = linesize;
    else
        rb.size = rb.size - (rb.size % linesize);

    rb.start = index * rb.size + start_offset;
    rb.cur = rb.start;
    rb.end = ((rb.start + rb.size) <= (regionsize + start_offset))
                 ? (rb.start + rb.size)
                 : (regionsize + start_offset);

    if (index >= targ->num_partitions) {
#if DEBUGPARALLELREAD
        printf("THREAD %d %c NO READ, EXIT\n", index, linetype);
#endif
        close(fd);
        pthread_exit(NULL);
    }

    // Read
    do {
#if DEBUGPARALLELREAD
        printf(
            "\n[T %c] %d reads: offset start: %ld, offset end: %ld, size: "
            "%ld\n",
            linetype, index, rb.start, rb.end, rb.size);
#endif
        // Move the cursor on offset_start
        lseek(fd, rb.start, SEEK_SET);

        // Read the assigned portion of the file
        do {
            if (linetype == 'n') {
                read(fd, &nl, linesize);
#if DEBUGPARALLELREAD
                printf("[T NODE %d] Node: %d - [%.lf; %.lf] - Cur: %ld\n",
                       index, nl.index, nl.x, nl.y, rb.cur);
#endif
                p = POSITIONinit(nl.x, nl.y);
                pthread_mutex_lock(m_nodes);
                STinsert(G->tab, p, nl.index);
                pthread_mutex_unlock(m_nodes);
                POSITIONfree(p);

            } else if (linetype == 'e') {
                read(fd, &el, linesize);
#if DEBUGPARALLELREAD
                printf("[T EDGE %d] Edge: %d --> %d - wt = %lf\n", index,
                       el.id1, el.id2, el.wt);
#endif
                pthread_mutex_lock(m_edges);
                GRAPHinsertE(G, el.id1, el.id2, el.wt);
                pthread_mutex_unlock(m_edges);
            }
            rb.cur += linesize;
        } while (rb.cur < rb.end);

        // Compute next region to read
        rb.start += rb.size * num_thread;
        rb.end = ((rb.start + rb.size) <= (regionsize + start_offset))
                     ? (rb.start + rb.size)
                     : (regionsize + start_offset);
        rb.cur = rb.start;

    } while (rb.start < start_offset + regionsize);

    close(fd);
    pthread_exit(NULL);
}
Graph GRAPHload_parallel3(char *filepath, int num_partitions_nodes,
                          int num_partitions_edges, int num_threads_nodes,
                          int num_threads_edges) {
    int fd;
    int n_nodes;
    int i, j;
    pthread_t *threads_nodes, *threads_edges;
    struct thread_arg_s *threads_arg;
    struct stat stat_buf;
    ssize_t filesize, filesize_nodes_region, filesize_edges_region;
    Graph G;
    struct node_line_s nl;

    // Allocate array of threads and global mutexes
    pthread_mutex_t *m_nodes = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *m_edges = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m_nodes, NULL);
    pthread_mutex_init(m_edges, NULL);
    threads_nodes = (pthread_t *)malloc(num_threads_nodes * sizeof(pthread_t));
    threads_edges = (pthread_t *)malloc(num_threads_nodes * sizeof(pthread_t));
    threads_arg = (struct thread_arg_s *)malloc(
        (num_threads_edges + num_threads_nodes) * sizeof(struct thread_arg_s));
    if (m_nodes == NULL || m_edges == NULL || threads_nodes == NULL ||
        threads_edges == NULL || threads_arg == NULL) {
        return NULL;
    }

    // Open the file
    fd = open(filepath, O_RDONLY);
    fstat(fd, &stat_buf);
    filesize = stat_buf.st_size;

    // Read first line of the file and get size information
    read(fd, &n_nodes, sizeof(int));
    filesize_nodes_region = n_nodes * sizeof(struct node_line_s);
    filesize_edges_region = filesize - filesize_nodes_region - sizeof(int);
    close(fd);
#if DEBUGPARALLELREAD
    printf(
        "[INFO SIZE: NODE LINE=%ld, EDGE LINE=%ld, NODES REGION=%ld, EDGES "
        "REGION=%ld]\n",
        sizeof(struct node_line_s), sizeof(struct edge_line_s),
        filesize_nodes_region, filesize_edges_region);
#endif
    // Allocate the graph
    G = GRAPHinit(n_nodes);
    if (G == NULL) return NULL;

    // Create threads for reading nodes and edges
    for (i = 0; i < num_threads_nodes + num_threads_edges; i++) {
        strcpy(threads_arg[i].filepath, filepath);
        threads_arg[i].filesize = filesize;
        threads_arg[i].G = &G;
        threads_arg[i].m_nodes = m_nodes;
        threads_arg[i].m_edges = m_edges;
        if (i < num_threads_nodes) {
            threads_arg[i].num_threads = num_threads_nodes;
            threads_arg[i].num_partitions = num_partitions_nodes;
            threads_arg[i].regionsize = filesize_nodes_region;
            threads_arg[i].index = i;
            threads_arg[i].linetype = 'n';
            threads_arg[i].start_offset = sizeof(int);
            threads_arg[i].linesize = sizeof(struct node_line_s);
            pthread_create(&threads_nodes[i], NULL, thread_read,
                           (void *)&threads_arg[i]);
        } else {
            j = i - num_threads_nodes;
            threads_arg[i].num_threads = num_threads_edges;
            threads_arg[i].num_partitions = num_partitions_edges;
            threads_arg[i].regionsize = filesize_edges_region;
            threads_arg[i].index = j;
            threads_arg[i].linetype = 'e';
            threads_arg[i].start_offset = filesize_nodes_region + sizeof(int);
            threads_arg[i].linesize = sizeof(struct edge_line_s);
            pthread_create(&threads_edges[j], NULL, thread_read,
                           (void *)&threads_arg[i]);
        }
    }
    // Join threads
    for (i = 0; i < num_threads_nodes; i++) {
        pthread_join(threads_nodes[i], NULL);
    }
    for (i = 0; i < num_threads_edges; i++) {
        pthread_join(threads_edges[i], NULL);
    }
    free(threads_nodes);
    free(threads_edges);
    free(threads_arg);
    free(m_nodes);
    free(m_edges);
    return G;
}

// #################################
// ### PARALLEL READ - VERSION 2 ###
// #################################

// Thread argument data structure
struct arg_t {
    int start1;
    int stop1;
    int start2;
    int stop2;
    void *src;
    int V;
    Graph G;
    pthread_spinlock_t *node;
    pthread_spinlock_t *edge;
};

static void parallelIo(void *arg) {
    Position p;
    struct node_line_s *node_line_d;
    struct edge_line_s *edge_line_d;
    // Read the nodes name(index 0-V) and the coordinates of each node
    struct arg_t *args = (struct arg_t *)arg;

    node_line_d = args->src + sizeof(int);
    node_line_d += args->start1;

    for (int i = args->start1; i < args->stop1; i++, node_line_d++) {
        p = POSITIONinit(node_line_d->x, node_line_d->y);
        pthread_spin_lock(args->node);  // lock
        STinsert(args->G->tab, p, node_line_d->index);
        pthread_spin_unlock(args->node);  // unlock
        POSITIONfree(p);
    }

    edge_line_d =
        args->src + sizeof(int) + (args->V * sizeof(struct node_line_s));
    edge_line_d += args->start2;

    for (int i = args->start2; i < args->stop2; i++, edge_line_d++) {
        if (edge_line_d->id1 >= 0 && edge_line_d->id2 >= 0) {
            pthread_spin_lock(args->edge);  // lock
            GRAPHinsertE(args->G, edge_line_d->id1, edge_line_d->id2,
                         edge_line_d->wt);
            pthread_spin_unlock(args->edge);  // unlock
        }
    }
    pthread_exit(NULL);
}
Graph GRAPHload_parallel2(char *filepath, int num_threads) {
    int V, T, i, j, k, v, e, id1, id2, nodexT, edgexT, copysz, fin;
    float E;
    char label1[MAXC], label2[MAXC];
    pthread_t *threads;
    struct arg_t *args;
    struct stat sb;
    void *src;
    pthread_spinlock_t node;
    pthread_spinlock_t edge;

    double wt;
    Graph G;
    Position p;

    // Open file
    fin = open(filepath, O_RDONLY);

    // Read the first line: number of nodes
    if (read(fin, &V, sizeof(int)) != sizeof(int)) return NULL;

    // Initialize the Graph ADT
    G = GRAPHinit(V);
    if (G == NULL) return NULL;

    // Memory Mapping
    if (fstat(fin, &sb) < 0) return NULL;
    copysz = sb.st_size;
    E = (float)(copysz - sizeof(int) - (V * sizeof(struct node_line_s))) /
        (sizeof(struct edge_line_s));
    src = mmap(0, copysz, PROT_READ, MAP_SHARED, fin, 0);
    if (src == MAP_FAILED) return NULL;

    // Initialize threads
    T = num_threads;
    nodexT = (int)ceil(((float)V) / T);
    edgexT = (int)ceil(E / T);
    threads = (pthread_t *)malloc(T * sizeof(pthread_t));
    if (threads == NULL) return NULL;
    args = (struct arg_t *)malloc(T * sizeof(struct arg_t));
    if (args == NULL) return NULL;

    pthread_spin_init(&node, 0);
    pthread_spin_init(&edge, 0);

    for (i = 0, j = 0, k = 0, v = V, e = E; i < T; i++) {
        args[i].src = src;
        args[i].V = V;
        args[i].G = G;
        args[i].node = &node;
        args[i].edge = &edge;
        args[i].start1 = j;
        if (v >= nodexT) {
            j += nodexT;
            v -= nodexT;
        } else {
            j += v;
            v = 0;
        }
        args[i].stop1 = j;
        args[i].start2 = k;
        if (e >= edgexT) {
            k += edgexT;
            e -= edgexT;
        } else {
            k += e;
            e = 0;
        }
        args[i].stop2 = k;
        pthread_create(&threads[i], NULL, parallelIo, (void *)&args[i]);
    }

    for (i = 0; i < T; i++) pthread_join(threads[i], NULL);

    // Close file
    close(fin);

    // Free space
    pthread_spin_destroy(&node);
    pthread_spin_destroy(&edge);
    munmap(src, copysz);
    free(args);
    free(threads);
    return G;
}

// #################################
// ### PARALLEL READ - VERSION 1 ###
// #################################

// Thread argument data structure
struct threadData {
    pthread_t threadId;
    int fd;
    int id;
    Graph G;
    int V;
};
// Global data
int len = 0;
int retVal = 1;
sem_t sem, sem2, sem3;
struct node_line_s v;
struct edge_line_s e;
static void *GRAPHloadParallel(void *arg) {
    struct threadData *td;
    td = (struct threadData *)arg;

    int i;
    char label1[MAXC], label2[MAXC];
    double wt;
    Position p;

    // Read the nodes name(index 0-V) and the coordinates of each node

#if DEBUGPRINT
    // Print nodes coordinates
    for (i = 0; i < td->V; i++) {
        POSITIONprint(STsearchByIndex(td->G->tab, i), 1);
    }
#endif
    // int fin = open(td->fd, O_RDONLY);

    while (1) {
        sem_wait(&sem3);
        if (len < td->V) {
            sem_wait(&sem);
            // fscanf(td->fd, "%d %d %d", &node_index, &node_x, &node_y);
            retVal = read(td->fd, &v, sizeof(struct node_line_s));
            // printf("%d %d %d\n", v.n, v.node_x, v.node_y);
            len++;
            p = POSITIONinit(v.x, v.y);
            STinsert(td->G->tab, p, v.index);
            sem_post(&sem);
            sem_post(&sem3);
            POSITIONfree(p);
        } else {
            sem_wait(&sem2);

            // retVal = fscanf(td->fd, "%d %d %lf", &id1, &id2, &wt);
            retVal = read(td->fd, &e, sizeof(struct edge_line_s));

            if ((e.id1 >= 0 && e.id2 >= 0) && (e.id1 != 0 || e.id2 != 0)) {
                // printf("%d %d %lf\n", e.n1, e.n2, e.e);
                GRAPHinsertE(td->G, e.id1, e.id2, e.wt);
            }
            if (retVal != sizeof(struct edge_line_s)) {
                sem_post(&sem2);
                sem_post(&sem3);
                return td->G;
            }
            sem_post(&sem2);
            sem_post(&sem3);
        }
    }
    pthread_exit(NULL);
    return td->G;
}

Graph GRAPHload_parallel1(char *filepath, int num_threads) {
    int V, i, fin;
    Graph G;
    struct threadData *td;
    void *retval;

    sem_init(&sem, 0, 1);
    sem_init(&sem2, 0, 1);
    sem_init(&sem3, 0, 1);

    // Open file
    fin = open(filepath, O_RDONLY);

    // Read the first line: number of nodes
    if (read(fin, &V, sizeof(int)) != sizeof(int)) return NULL;

    // Initialize the Graph ADT
    G = GRAPHinit(V);
    if (G == NULL) return NULL;
    // printf("V = %d\n", V);
    // Read the first line: number of nodes
    // fscanf(fin, "%d", &V);

    // Initialize the Graph ADT

    td = (struct threadData *)malloc(num_threads * sizeof(struct threadData));
    for (i = 0; i < num_threads; i++) {
        td[i].fd = fin;
        td[i].id = i;
        td[i].V = V;
        td[i].G = G;
        pthread_create(&(td[i].threadId), NULL, GRAPHloadParallel,
                       (void *)&td[i]);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(td[i].threadId, &retval);
    }

    sem_destroy(&sem);
    sem_destroy(&sem2);
    sem_destroy(&sem3);
    close(fin);

    return G;
}

