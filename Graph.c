#include "Graph.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
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

struct threadData {
    pthread_t threadId;
    int fd;
    int id;
    Graph G;
    int V;
};

struct V_data {
    int n;
    double node_x;
    double node_y;
};

struct E_data {
    int n1;
    int n2;
    double e;
};

int len = 0;
int retVal = 1;
sem_t sem, sem2, sem3;
struct V_data v;
struct E_data e;

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

static void *GRAPHloadParallel(void *arg)  {
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
    //int fin = open(td->fd, O_RDONLY);

    while (1) {
        sem_wait(&sem3);
         if (len < td->V) {
            sem_wait(&sem);
            //fscanf(td->fd, "%d %d %d", &node_index, &node_x, &node_y);
            retVal = read(td->fd, &v, sizeof(struct V_data));
            //printf("%d %d %d\n", v.n, v.node_x, v.node_y);
            len++;
            p = POSITIONinit(v.node_x, v.node_y);
            STinsert(td->G->tab, p, v.n);
            sem_post(&sem); 
            sem_post(&sem3); 
            POSITIONfree(p);
        } else {
            sem_wait(&sem2);
            //retVal = fscanf(td->fd, "%d %d %lf", &id1, &id2, &wt);
            retVal = read(td->fd, &e, sizeof(struct E_data));
            
            if ((e.n1 >= 0 && e.n2 >= 0) && (e.n1 != 0 || e.n2 != 0)) {
                //printf("%d %d %lf\n", e.n1, e.n2, e.e);
                GRAPHinsertE(td->G, e.n1, e.n2, e.e);
            }
            if(retVal < 0){
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
/*Graph GRAPHload(FILE *fin) {
    int V, i;
    Graph G;
    struct threadData *td;
    void *retval;

    // Read the first line: number of nodes
    fscanf(fin, "%d", &V);

    // Initialize the Graph ADT
    G = GRAPHinit(V);

    if (G == NULL) return NULL;

    sem_init(&sem, 0, 1);
    sem_init(&sem2, 0, 1);
    sem_init(&sem3, 0, 1);

    printf("%d\n", V);
    td = (struct threadData *)malloc(10 * sizeof(struct threadData));
    for (i = 0; i < 10; i++) {
        td[i].fd = fin;
        td[i].id = i;
        td[i].V = V;
        td[i].G = G;
        pthread_create(&(td[i].threadId), NULL, GRAPHloadParallel, (void *)&td[i]);
    }
    for(i = 0; i < 10; i++) {
        pthread_join(td[i].threadId, &retval);
    }

    sem_destroy(&sem);
    sem_destroy(&sem2);
    sem_destroy(&sem3);

    return G;
}*/

Graph GRAPHloadBin(char *fd) {
    int V, i, fin;
    Graph G;
    struct threadData *td;
    void *retval;

    sem_init(&sem, 0, 1);
    sem_init(&sem2, 0, 1);
    sem_init(&sem3, 0, 1);

    fin = open(fd, O_RDONLY);
    read(fd, &V, sizeof(V));
    printf("V = %d\n", V);
    // Read the first line: number of nodes
    //fscanf(fin, "%d", &V);
    
    // Initialize the Graph ADT
    G = GRAPHinit(V);

    if (G == NULL) return NULL;
 
    td = (struct threadData *)malloc(10 * sizeof(struct threadData));
    for (i = 0; i < 10; i++) {
        td[i].fd = fin;
        td[i].id = i;
        td[i].V = V;
        td[i].G = G;
        pthread_create(&(td[i].threadId), NULL, GRAPHloadParallel, (void *)&td[i]);
    }
    for(i = 0; i < 10; i++) {
        pthread_join(td[i].threadId, &retval);
    }

    sem_destroy(&sem);
    sem_destroy(&sem2);
    sem_destroy(&sem3);
    close(fin);

    return G;
}

Graph GRAPHload_sequential(char *filepath) {
    int V, i, id1, id2, fd, nR;
    char label1[MAXC], label2[MAXC];
    double wt;
    Graph G;
    Position p;
    struct V_data nl;
    struct E_data el;

    fd = open(filepath, O_RDONLY);
    read(fd, &V, sizeof(V));
    printf("%d\n", V);
    G = GRAPHinit(V);
    if (G == NULL) return NULL;
    for (i = 0; i < V; i++) {
        read(fd, &nl, sizeof(struct V_data));
        //printf("%d %f %f\n", nl.n, nl.node_x, nl.node_y);
        p = POSITIONinit(nl.node_x, nl.node_y);
        STinsert(G->tab, p, nl.n);
        POSITIONfree(p);
    }
    while ((nR = read(fd, &el, sizeof(struct E_data))) > 0) {
        if (el.n1 >= 0 && el.n2 >= 0) GRAPHinsertE(G, el.n1, el.n2, el.e);
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