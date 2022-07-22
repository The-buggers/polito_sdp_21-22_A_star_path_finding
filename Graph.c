#include "Graph.h"

#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

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
struct arg_t {
    int start1;
    int stop1;
    int start2;
    int stop2;
    void *src;
    int V;
    Graph G;
    pthread_mutex_t *node_m;
    pthread_mutex_t *edge_m;
};
struct row1_t {
    int index;
    int x;
    int y;
};
struct row2_t {
    int a;
    int b;
    double wt;
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
static void parallelIo(void *arg) {
    Position p;
    struct row1_t *row1_d;
    struct row2_t *row2_d;
    // Read the nodes name(index 0-V) and the coordinates of each node
    struct arg_t *args = (struct arg_t *)arg;

    row1_d = args->src + sizeof(int);
    row1_d += args->start1;
    // printf("Thread starts from %d and stop at %d\n", args->start1,
    // args->stop1);
    for (int i = args->start1; i < args->stop1; i++, row1_d++) {
        // printf("%d %d %d\n", row1_d->index, row1_d->x, row1_d->y);
        p = POSITIONinit(row1_d->x, row1_d->y);
        pthread_mutex_lock(args->node_m);  // lock
        STinsert(args->G->tab, p, row1_d->index);
        pthread_mutex_unlock(args->node_m);  // unlock
        POSITIONfree(p);
    }
    // printf("Edge starts from %d and stop at %d\n", args->start2,
    // args->stop2);
    row2_d = args->src + sizeof(int) + (args->V * sizeof(struct row1_t));
    row2_d += args->start2;
    for (int i = args->start2; i < args->stop2; i++, row2_d++) {
        // printf("%d %d %lf\n", row2_d->a, row2_d->b, row2_d->wt);
        if (row2_d->a >= 0 && row2_d->b >= 0) {
            pthread_mutex_lock(args->edge_m);  // lock
            GRAPHinsertE(args->G, row2_d->a, row2_d->b, row2_d->wt);
            pthread_mutex_unlock(args->edge_m);  // unlock
        }
    }
    pthread_exit(NULL);
}
Graph GRAPHloadParallel(int fin) {
    int V, T, i, j, k, v, e, id1, id2, nodexT, edgexT, copysz, fd;
    float E;
    char label1[MAXC], label2[MAXC];
    pthread_t *threads;
    struct arg_t *args;
    struct stat sb;
    void *src;
    pthread_mutex_t node;
    pthread_mutex_t edge;

    double wt;
    Graph G;
    Position p;

    // Read the first line: number of nodes
    if (read(fin, &V, sizeof(int)) != sizeof(int)) return NULL;

    // Initialize the Graph ADT
    G = GRAPHinit(V);
    if (G == NULL) return NULL;

    // Memory Mapping
    if (fstat(fin, &sb) < 0) return NULL;
    copysz = sb.st_size;
    int a = (copysz - sizeof(int) - (V * sizeof(struct row1_t)));
    int b = (sizeof(struct row2_t));
    E = (float)(copysz - sizeof(int) - (V * sizeof(struct row1_t))) /
        (sizeof(struct row2_t));
    src = mmap(0, copysz, PROT_READ, MAP_SHARED, fin, 0);
    if (src == MAP_FAILED) return NULL;

    // Initialize threads
    T = 2;
    nodexT = (int)ceil(((float)V) / T);
    edgexT = (int)ceil(E / T);
    threads = (pthread_t *)malloc(T * sizeof(pthread_t));
    if (threads == NULL) return NULL;
    args = (struct arg_t *)malloc(T * sizeof(struct arg_t));
    if (args == NULL) return NULL;

    pthread_mutex_init(&node, NULL);
    pthread_mutex_init(&edge, NULL);

    for (i = 0, j = 0, k = 0, v = V, e = E; i < T; i++) {
        args[i].src = src;
        args[i].V = V;
        args[i].G = G;
        args[i].node_m = &node;
        args[i].edge_m = &edge;
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

    munmap(src, copysz);
    free(args);
    free(threads);
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