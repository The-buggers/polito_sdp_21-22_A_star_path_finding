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
    int id;
    double x;
    double y;
} node_t;
typedef struct edge_s
{
    int a;
    int b;
    double wt;
} edge_t;

int random_graph_mode0(char *filename, double max_x, double max_y, int V, int max_k);
double compute_weight(int x1, int y1, int x2, int y2);
double gererate_random(double min, double max);
int main(int argc, char **argv)
{
    int mode = atoi(argv[2]);
    if (mode == 0)
    {
        random_graph_mode0(argv[1], atof(argv[3]), atof(argv[4]), atoi(argv[5]), atoi(argv[6]));
    }
    return 0;
}

int random_graph_mode0(char *filename, double max_x, double max_y, int V, int max_k)
{
    int fd;
    double x, y;
    int node_index;
    int random_node, valid_node;
    int **adj_mat;
    int n_iter, node_start;
    node_t *nodes;
    edge_t edge;
    int k, e;
    double wt;
    srand(time(NULL));
#if PRINT
    printf("Random graph params: V=%d, max_x=%lf, max_y=%lf, k=%d\n", V, max_x, max_y, k);
#endif
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

    fd = open(filename, O_RDWR|O_CREAT, 0777);
    //  Generate V random nodes in the plane
#if PRINT
    fprintf(stdout, "%d\n", V);
#endif
    write(fd, &V, sizeof(int));
    for (node_index = 0; node_index < V; node_index++)
    {
        x = gererate_random(0, max_x);
        y = gererate_random(0, max_y);
        nodes[node_index].x = x;
        nodes[node_index].y = y;
        nodes[node_index].id = node_index;
#if PRINT
        fprintf(stdout, "%d %lf %lf\n", node_index, x, y);
#endif
        write(fd, &nodes[node_index], sizeof(node_t));
    }

    // Generate for each node V a random path in the graph with length at most max_k
    for (node_index = 0; node_index < V; node_index++)
    {
        k = rand() % max_k;
        node_start = node_index;
        for (e = 0; e < k; e++)
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
            edge.a = node_start;
            edge.b = random_node;
            edge.wt = wt;
            write(fd, &edge, sizeof(edge_t));

            // Next iteration will search for [andom_node --> new_random_node]
            node_start = random_node; // Move on the extracted node to go on
        }
    }

    free(nodes);
    close(fd);
}

double compute_weight(int x1, int y1, int x2, int y2)
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}
double gererate_random(double min, double max){
     return ( (double)rand() * ( max - min ) ) / (double)RAND_MAX + min;
}