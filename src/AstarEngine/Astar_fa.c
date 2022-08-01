#include "Astar.h"

#include <float.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#define maxWT DBL_MAX

// ###################################
// ### parallel A* - first attempt ###
// ###################################

struct threadData {
    pthread_t threadId;
    int id;
    Graph G;
    int V;
    int source;
    int dest;
};

PQ open_list;
int *previous;
double *fvalues, *hvalues, *gvalues, *cost; // f(n) for each node n
int *closed_list;
int len1 = 0;
int found = 0;
Position pos_dest;
double tot_cost = 0;
pthread_spinlock_t lockI, lockP;
static void *ASTARshortest_path_parallel_initializer(void *arg);
static void *ASTARshortest_path_parallel(void *arg);

static void *ASTARshortest_path_parallel_initializer(void *arg){
    struct threadData *td;
    td = (struct threadData *)arg;
    Position p;
    
    while(len1 < td->V){
        pthread_spin_lock(&lockI);
        if(len1 == td->V){
            pthread_spin_unlock(&lockI);
            return;
        }
        
        previous[len1] = -1;
        closed_list[len1] = -1;
        p = GRAPHget_node_position(td->G, len1);
        hvalues[len1] = heuristic_euclidean(p, pos_dest);
        fvalues[len1] = maxWT;
        gvalues[len1] = maxWT;
        len1++;

        pthread_spin_unlock(&lockI);
    }
    
    pthread_exit(NULL);
    return;
}

static void *ASTARshortest_path_parallel(void *arg){
    struct threadData *td;
    td = (struct threadData *)arg;
    int a, b;
    double f_extracted_node, g_b, f_b, a_b_wt;
    link t;

    while (1)  // while OPEN list not empty
    {
        pthread_spin_lock(&lockP);
        if(PQempty(open_list)){
            pthread_spin_unlock(&lockP);
            return;
        }
        // Take from OPEN list the node with min f(n) (min priority)
        f_extracted_node = fvalues[a = PQextractMin(open_list, fvalues)];

        // If the extracted node is the destination stop: path found
        if (a == td->dest) {
            found = 1;
            reconstruct_path(previous, td->source, td->dest, cost);
            pthread_spin_unlock(&lockP);
            break;
        }

        // For each successor 'b' of node 'a':
        for (t = GRAPHget_list_node_head(td->G, a);
             t != GRAPHget_list_node_tail(td->G, a); t = LINKget_next(t)) {
            b = LINKget_node(t);
            a_b_wt = LINKget_wt(t);

            // Compute f(b) = g(b) + h(b) = [g(a) + w(a,b)] + h(b)
            g_b = gvalues[a] + a_b_wt;
            f_b = g_b + hvalues[b];

            if (g_b < gvalues[b]) {
                previous[b] = a;
                cost[b] = a_b_wt;
                gvalues[b] = g_b;
                fvalues[b] = f_b;
                if (PQsearch(open_list, b) == -1) {
                    PQinsert(open_list, fvalues, b);
                } else {
                    PQchange(open_list, fvalues, b);
                }
            }
        }
        pthread_spin_unlock(&lockP);
    }
    pthread_exit(NULL);
    return;
}



void ASTARshortest_path_fa(Graph G, int source, int dest, int num_threads){
    printf("A star algorithm on graph from %d to %d\n", source, dest);
    int V = GRAPHget_num_nodes(G);
    struct threadData *td;
    int v, a, b, i;
    double f_extracted_node, g_b, f_b, a_b_wt;

    double tot_cost = 0;

    void *retval;
    Position pos_source = GRAPHget_node_position(G, source);
    pos_dest = GRAPHget_node_position(G, dest);
    Position p;
    link t;

    pthread_spin_init(&lockI, 0);
    pthread_spin_init(&lockP, 0);

    open_list = PQinit(V);
    previous = (int *)malloc(V * sizeof(int));
    fvalues = (double *)malloc(V * sizeof(double));
    hvalues = (double *)malloc(V * sizeof(double));
    gvalues = (double *)malloc(V * sizeof(double));
    closed_list = (int *)malloc(V * sizeof(int));
    cost = (double *)malloc(V * sizeof(double));
    if ((previous == NULL) || (fvalues == NULL) || (hvalues == NULL) ||
        (gvalues == NULL) || (closed_list == NULL) || (cost == NULL))
        return;
    
    td = (struct threadData *)malloc(num_threads * sizeof(struct threadData));

    for(i = 0; i < num_threads; i++){
        td[i].id = i;
        td[i].V = V;
        td[i].G = G;
        pthread_create(&(td[i].threadId), NULL, ASTARshortest_path_parallel_initializer,
                       (void *)&td[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(td[i].threadId, &retval);
    }

    fvalues[source] =
        compute_f(hvalues[source], 0);  // g(n) = 0 for n == source
    gvalues[source] = 0;
    // TRY PQchange(open_list, fvalues, source);
    PQinsert(open_list, fvalues, source);

    for(i = 0; i < num_threads; i++){
        td[i].id = i;
        td[i].V = V;
        td[i].G = G;
        td[i].source = source;
        td[i].dest = dest;
        pthread_create(&(td[i].threadId), NULL, ASTARshortest_path_parallel,
                       (void *)&td[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(td[i].threadId, &retval);
    }

    
    
    if (!found) {
        printf("+-----------------------------------+\n");
        printf("Path not found\n");
        printf("+-----------------------------------+\n\n");
        return;
    }

    pthread_spin_destroy(&lockI);
    pthread_spin_destroy(&lockP);
    free(td);
    free(open_list);
    free(previous);
    free(fvalues);
    free(hvalues);
    free(gvalues);
    free(cost);

}


