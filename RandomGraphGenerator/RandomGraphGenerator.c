#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <math.h>
#define PRINT 0
typedef struct node_s
{
    int x;
    int y;
} node_t;

int graph_generation(char *filename, int max_x, int max_y, int V, int max_k);
int dense_graph_generation(char *filename, int max_x, int max_y, int V, int max_k);
int randcost_graph_generation(char *filename, int max_x, int max_y, int V, int max_k);
double compute_weight(int x1, int y1, int x2, int y2);
int main(int argc, char **argv)
{
    int mode = atoi(argv[2]);
    if (argc > 10)
    {
        fprintf(stdout, "Wrong Number Of Arguments");
        return 0;
    }
    if (mode == 1)
    {
        dense_graph_generation(argv[1], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    }
    else if (mode == 0)
    {
        graph_generation(argv[1], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    }else if(mode == 2){
        randcost_graph_generation(argv[1], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    }

    return 0;
}

int graph_generation(char *filename, int max_x, int max_y, int V, int max_k)
{
    FILE *fp;
    int x, y;
    int node_index;
    int random_node, valid_node;
    int **adj_mat;
    int n_iter, node_start;
    node_t *nodes;
    int k, edge;
    double wt;
    srand(time(NULL));

    if (max_k > V * (V - 1))
    {
        return -1;
    }
    // Allocate dynamically the adjacence matrix
    adj_mat = (int **)malloc(V * sizeof(int *));
    for (node_index = 0; node_index < V; node_index++)
        adj_mat[node_index] = (int *)calloc(V, sizeof(int));

    // Allocate dynamically the array of nodes
    nodes = (node_t *)malloc(V * sizeof(node_t));

    fp = fopen(filename, "w+t");
    //  Generate V random nodes in the plane
#if PRINT
    fprintf(stdout, "%d\n", V);
#endif
    fprintf(fp, "%d\n", V);
    for (node_index = 0; node_index < V; node_index++)
    {
        x = rand() % max_x;
        y = rand() % max_y;
        nodes[node_index].x = x;
        nodes[node_index].y = y;
#if PRINT
        fprintf(stdout, "%d %d %d\n", node_index, x, y);
#endif
        fprintf(fp, "%d %d %d\n", node_index, x, y);
    }

    // Generate for each node V a random path in the graph with length at most max_k
    for (node_index = 0; node_index < V; node_index++)
    {
        k = rand() % max_k;
        node_start = node_index;
        for (edge = 0; edge < k; edge++)
        {
            random_node = -1;
            n_iter = 0;
            // Extract a random valid node: neither myself nor one already connected with
            do
            {
                random_node = rand() % V;
                if (random_node == node_start || adj_mat[node_start][random_node] == 1)
                    valid_node = 0;
                else
                    valid_node = 1;
                n_iter++;
                if (n_iter > V)
                { // Force stop
                    break;
                }
            } while (valid_node == 0);
            if (random_node == -1)
                continue;

            // Edge [node_start --> random_node] is valid, add it to file
            wt = compute_weight(nodes[node_start].x,
                                nodes[node_start].y,
                                nodes[random_node].x,
                                nodes[random_node].y);
            adj_mat[node_start][random_node] = 1;
#if PRINT
            fprintf(stdout, "%d %d %.2lf\n", node_start, random_node, wt);
#endif
            fprintf(fp, "%d %d %.2lf\n", node_start, random_node, wt);

            // Next iteration will search for [andom_node --> new_random_node]
            node_start = random_node; // Move on the extracted node to go on
        }
    }

    free(nodes);
    fclose(fp);
}

int dense_graph_generation(char *filename, int max_x, int max_y, int V, int max_k)
{
    FILE *fp;
    int x, y;
    int node_index;
    int random_node, valid_node;
    int **adj_mat;
    int n_iter, node_start;
    node_t *nodes;
    int k, edge;
    double wt;
    srand(time(NULL));

    if (max_k > V * (V - 1))
    {
        return -1;
    }
    // Allocate dynamically the adjacence matrix
    adj_mat = (int **)malloc(V * sizeof(int *));
    for (node_index = 0; node_index < V; node_index++)
        adj_mat[node_index] = (int *)calloc(V, sizeof(int));

    // Allocate dynamically the array of nodes
    nodes = (node_t *)malloc(V * sizeof(node_t));

    fp = fopen(filename, "w+t");
    //  Generate V random nodes in the plane
#if PRINT
    fprintf(stdout, "%d\n", V);
#endif
    fprintf(fp, "%d\n", V);
    for (node_index = 0; node_index < V; node_index++)
    {
        x = rand() % max_x;
        y = rand() % max_y;
        nodes[node_index].x = x;
        nodes[node_index].y = y;
#if PRINT
        fprintf(stdout, "%d %d %d\n", node_index, x, y);
#endif
        fprintf(fp, "%d %d %d\n", node_index, x, y);
    }

    // For each node V connect it to k other nodes
    for (node_index = 0; node_index < V; node_index++)
    {
        k = rand() % max_k + 1;
        for (edge = 0; edge < k; edge++)
        {
            // Extract a random valid node: neither myself nor one already connected with
            do
            {
                random_node = rand() % V;
                if (random_node == node_index || adj_mat[node_index][random_node] == 1)
                    valid_node = 0;
                else
                    valid_node = 1;
                n_iter++;
                if (n_iter > V)
                { // Force stop
                    break;
                }
            } while (valid_node == 0);
            if (random_node == -1)
                continue;

            // Edge [node_index --> random_node] is valid, add it to file
            wt = compute_weight(nodes[node_index].x,
                                nodes[node_index].y,
                                nodes[random_node].x,
                                nodes[random_node].y);
            adj_mat[node_index][random_node] = 1;
#if PRINT
            fprintf(stdout, "%d %d %.2lf\n", node_start, random_node, wt);
#endif
            fprintf(fp, "%d %d %.2lf\n", node_index, random_node, wt);
        }
    }

    free(nodes);
    fclose(fp);
}
int randcost_graph_generation(char *filename, int max_x, int max_y, int V, int max_k)
{
    FILE *fp;
    int x, y;
    int node_index;
    int random_node, valid_node;
    int **adj_mat;
    int n_iter, node_start;
    node_t *nodes;
    int k, edge;
    double wt;
    srand(time(NULL));

    if (max_k > V * (V - 1))
    {
        return -1;
    }
    // Allocate dynamically the adjacence matrix
    adj_mat = (int **)malloc(V * sizeof(int *));
    for (node_index = 0; node_index < V; node_index++)
        adj_mat[node_index] = (int *)calloc(V, sizeof(int));

    // Allocate dynamically the array of nodes
    nodes = (node_t *)malloc(V * sizeof(node_t));

    fp = fopen(filename, "w+t");
    //  Generate V random nodes in the plane
#if PRINT
    fprintf(stdout, "%d\n", V);
#endif
    fprintf(fp, "%d\n", V);
    for (node_index = 0; node_index < V; node_index++)
    {
        x = rand() % max_x;
        y = rand() % max_y;
        nodes[node_index].x = x;
        nodes[node_index].y = y;
#if PRINT
        fprintf(stdout, "%d %d %d\n", node_index, x, y);
#endif
        fprintf(fp, "%d %d %d\n", node_index, x, y);
    }

    // For each node V connect it to k other nodes
    for (node_index = 0; node_index < V; node_index++)
    {
        k = rand() % max_k + 1;
        for (edge = 0; edge < k; edge++)
        {
            // Extract a random valid node: neither myself nor one already connected with
            do
            {
                random_node = rand() % V;
                if (random_node == node_index || adj_mat[node_index][random_node] == 1)
                    valid_node = 0;
                else
                    valid_node = 1;
                n_iter++;
                if (n_iter > V)
                { // Force stop
                    break;
                }
            } while (valid_node == 0);
            if (random_node == -1)
                continue;

            // Edge [node_index --> random_node] is valid, add it to file
            wt = (double)(rand() % 100);
            adj_mat[node_index][random_node] = 1;
#if PRINT
            fprintf(stdout, "%d %d %.2lf\n", node_start, random_node, wt);
#endif
            fprintf(fp, "%d %d %.2lf\n", node_index, random_node, wt);
        }
    }

    free(nodes);
    fclose(fp);
}
double compute_weight(int x1, int y1, int x2, int y2)
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}