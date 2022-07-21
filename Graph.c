#include "Graph.h"

#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PQ.h"
#include "Position.h"
#define maxWT INT_MAX
#define MAXC 10
struct node {
    int v;
    double wt;
    link next;
};
struct graph {
    int V;
    int E;
    link *ladj;
    ST tab;
    link z;
};
static Edge EDGEcreate(int v, int w, double wt);
static link NEW(int v, double wt, link next);
static void insertE(Graph G, Edge e);
static void removeE(Graph G, Edge e);
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
Graph GRAPHload(FILE *fin) {
    int V, i, id1, id2;
    char label1[MAXC], label2[MAXC];
    int node_index, node_x, node_y;
    double wt;
    Graph G;
    Position p;

    // Read the first line: number of nodes
    fscanf(fin, "%d", &V);

    // Initialize the Graph ADT
    G = GRAPHinit(V);
    if (G == NULL) return NULL;

    // Read the nodes name(index 0-V) and the coordinates of each node
    for (i = 0; i < V; i++) {
        fscanf(fin, "%d %d %d", &node_index, &node_x, &node_y);

        p = POSITIONinit(node_x, node_y);
        STinsert(G->tab, p, node_index);
        POSITIONfree(p);
    }

#if DEBUGPRINT
    // Print nodes coordinates
    for (i = 0; i < G->V; i++) {
        POSITIONprint(STsearchByIndex(G->tab, i));
    }
#endif

    while (fscanf(fin, "%d %d %lf", &id1, &id2, &wt) == 3) {
        if (id1 >= 0 && id2 >= 0) GRAPHinsertE(G, id1, id2, wt);
    }
    return G;
}
void GRAPHedges(Graph G, Edge *a) {
    int v, E = 0;
    link t;
    for (v = 0; v < G->V; v++)
        for (t = G->ladj[v]; t != G->z; t = t->next)
            a[E++] = EDGEcreate(v, t->v, t->wt);
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

// Test: Djkstra algorithm
void GRAPHspD(Graph G, int id) {
    int v;
    link t;
    PQ pq = PQinit(G->V);
    int *st, *mindist;
    st = malloc(G->V * sizeof(int));
    mindist = malloc(G->V * sizeof(int));
    if ((st == NULL) || (mindist == NULL)) return;
    for (v = 0; v < G->V; v++) {
        st[v] = -1;
        mindist[v] = maxWT;
        PQinsert(pq, mindist, v);
    }
    mindist[id] = 0;
    st[id] = id;
    PQchange(pq, mindist, id);
    while (!PQempty(pq)) {
        if (mindist[v = PQextractMin(pq, mindist)] != maxWT) {
            for (t = G->ladj[v]; t != G->z; t = t->next)
                if (mindist[v] + t->wt < mindist[t->v]) {
                    mindist[t->v] = mindist[v] + t->wt;
                    PQchange(pq, mindist, t->v);
                    st[t->v] = v;
                }
        }
    }
    printf("\n Shortest path tree\n");
    for (v = 0; v < G->V; v++) printf("parent of %d is %d \n", v, st[v]);
    printf("\n Minimum distances from node %d\n", id);
    for (v = 0; v < G->V; v++) printf("mindist[%d] = %d \n", v, mindist[v]);
    PQfree(pq);
}
int GRAPHget_num_nodes(Graph G) { return G->V; }
Position GRAPHget_node_position(Graph G, int id) {
    return STsearchByIndex(G->tab, id);
}
link GRAPHget_list_node_head(Graph G, int v) { return G->ladj[v]; }
link GRAPHget_list_node_tail(Graph G, int v) { return G->z; }
link LINKget_next(link t) { return t->next; }
double LINKget_wt(link t) { return t->wt; }
int LINKget_node(link t) { return t->v; }
/*
void GRAPHshortest_path_astar(Graph G, int source, int dest){
    int v;
    link t;
    PQ pq = PQinit(G->V);
    int *st, *mindist;

    st = malloc(G->V * sizeof(int));
    mindist = malloc(G->V * sizeof(int));
    if ((st == NULL) || (mindist == NULL))
        return;

    // Compute Heuristic function for each node [Euclidean distance from node N
to node dest]

    // Add all the nodes inside the priority queue
    for (v = 0; v < G->V; v++)
    {
        st[v] = -1;
        mindist[v] = maxWT;
        PQinsert(pq, mindist, v);
    }

    mindist[id] = 0;
    st[id] = id;
    PQchange(pq, mindist, id);
    while (!PQempty(pq))
    {
        if (mindist[v = PQextractMin(pq, mindist)] != maxWT)
        {
            for (t = G->ladj[v]; t != G->z; t = t->next)
                if (mindist[v] + t->wt < mindist[t->v])
                {
                    mindist[t->v] = mindist[v] + t->wt;
                    PQchange(pq, mindist, t->v);
                    st[t->v] = v;
                }
        }
    }
    printf("\n Shortest path tree\n");
    for (v = 0; v < G->V; v++)
        printf("parent of %d is %d \n", v, st[v]);
    printf("\n Minimum distances from node %d\n", id);
    for (v = 0; v < G->V; v++)
        printf("mindist[%d] = %d \n", v, mindist[v]);
    PQfree(pq);
}*/
// ^(\s)*$\n

// PARALLEL READ
struct node_line_s {
    int index;
    int x;
    int y;
};

struct edge_line_s {
    int id1;
    int id2;
    double wt;
};

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
};

struct read_block_s {
    off_t start;
    off_t end;
    off_t cur;
    ssize_t size;
};

static void *thread_read(void *arg);
pthread_mutex_t *m;

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
Graph GRAPHload_parallel3(char *filepath, int num_partitions,
                          int num_threads_nodes, int num_threads_edges) {
    int fd;
    int n_nodes;
    int i, j;
    pthread_t *threads_nodes, *threads_edges;
    struct thread_arg_s *threads_arg;
    struct stat stat_buf;
    ssize_t filesize, filesize_nodes_region, filesize_edges_region;
    Graph G;
    struct node_line_s nl;

    // Allocate array of threads and global mutex
    m = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m, NULL);
    threads_nodes = (pthread_t *)malloc(num_threads_nodes * sizeof(pthread_t));
    threads_edges = (pthread_t *)malloc(num_threads_nodes * sizeof(pthread_t));
    threads_arg = (struct thread_arg_s *)malloc(
        (num_threads_edges + num_threads_nodes) * sizeof(struct thread_arg_s));
    if (m == NULL || threads_nodes == NULL || threads_edges == NULL || threads_arg == NULL) {
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
        threads_arg[i].num_partitions = num_partitions;
        threads_arg[i].filesize = filesize;
        threads_arg[i].G = &G;  // &G
        if (i < num_threads_nodes) {
            threads_arg[i].num_threads = num_threads_nodes;
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

    return G;
}

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

    // If i have more threads than the number of partitions
    // other possible check: if(index > num_partitions)
    // /printf("T %d %c INFO: rb.start: %ld\n", index, linetype, rb.start);
    if (rb.start > regionsize + start_offset) {
#if DEBUGPARALLELREAD
        printf("THREAD %d %c NO READ, EXIT\n", index, linetype);
#endif
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
                printf("[T NODE %d] Node: %d - [%d; %d] - Cur: %ld\n", index,
                       nl.index, nl.x, nl.y, rb.cur);
#endif
                p = POSITIONinit(nl.x, nl.y);
                pthread_mutex_lock(m);
                STinsert(G->tab, p, nl.index);
                pthread_mutex_unlock(m);
                POSITIONfree(p);

            } else if (linetype == 'e') {
                read(fd, &el, linesize);
#if DEBUGPARALLELREAD
                printf("[T EDGE %d] Edge: %d --> %d - wt = %lf\n", index,
                       el.id1, el.id2, el.wt);
#endif
                pthread_mutex_lock(m);
                GRAPHinsertE(G, el.id1, el.id2, el.wt);
                pthread_mutex_unlock(m);
            }
            rb.cur += linesize;
        } while (rb.cur < rb.end);

        // Compute next region to read
        rb.start += rb.size * num_thread;  // MODIFY HERE
        rb.end = ((rb.start + rb.size) <= (regionsize + start_offset))
                     ? (rb.start + rb.size)
                     : (regionsize + start_offset);
        rb.cur = rb.start;

    } while (rb.start < start_offset + regionsize);

    pthread_exit(NULL);
}