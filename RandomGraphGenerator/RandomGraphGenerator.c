#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <math.h>
#define PRINT 1
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

int random_graph(char *filename, int source, int dest, int num_paths, int max_length);
double compute_weight(int x1, int y1, int x2, int y2);
double gererate_random(double min, double max);
int max(int a, int b);
int main(int argc, char **argv)
{
    int source = atoi(argv[2]);
    int dest = atoi(argv[3]);
    int num_paths = atoi(argv[4]);
    int max_length = atoi(argv[5]);
    random_graph(argv[1], source, dest, num_paths, max_length);
    return 0;
}

int random_graph(char *filename, int source, int dest, int num_paths, int max_length)
{
    int fd;
    double x, y;
    int node_index;
    int random_node, valid_node;
    int **adj_mat;
    int n_iter, node_start;
    node_t *nodes;
    edge_t edge;
    int i, k, e, try;
    double wt;
    srand(time(NULL));
    
    int V = max(source, dest) + 1;
#if PRINT
    printf("Random graph params: V=%d, num_paths=%d, max_length=%d\n", V, num_paths, max_length);
#endif
    
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
        x = gererate_random(-180, 180); // longitude
        y = gererate_random(-90, 90); // latitude
        nodes[node_index].x = x;
        nodes[node_index].y = y;
        nodes[node_index].id = node_index;
#if PRINT
        fprintf(stdout, "%d %lf %lf\n", node_index, x, y);
#endif
        write(fd, &nodes[node_index], sizeof(node_t));
    }

    // Generate num_paths paths from source to dest in the graph with length at most max_length
    for (i=0; i<num_paths; i++)
    {
        k = rand() % max_length;
        node_start = source;
        // Generate k-1 edges
        for (e = 0; e < k-1; e++)
        {
            // Extract a random valid node: neither source nor the destination
            do{
                random_node = rand() % V;
                if(random_node == source || random_node == dest || node_start == random_node || adj_mat[node_start][random_node] == 1)
                    random_node = -1;
            }while(random_node == -1);
            
            if(random_node == -1)
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
        
        // Connect the last random_node to the destination if it doesn't exists
        if(random_node ==-1 || adj_mat[random_node][dest] == 1)
            continue;
        wt = compute_weight(nodes[random_node].x,
                            nodes[random_node].y,
                            nodes[dest].x,
                            nodes[dest].y);
        adj_mat[random_node][dest] = 1;
    #if PRINT
        fprintf(stdout, "%d %d %.2lf\n", random_node, dest, wt);
    #endif
        edge.a = random_node;
        edge.b = dest;
        edge.wt = wt;
        write(fd, &edge, sizeof(edge_t));
    }

    free(nodes);
    close(fd);
}

double compute_weight(int x1, int y1, int x2, int y2)
{
    double lat_1 = y1;
    double lon_1 = x1;
    double lat_2 = y2;
    double lon_2 = x2;

    double earth_radius = 6371e3;       // metres
    double phi_1 = lat_1 * M_PI / 180;  // φ in radians
    double phi_2 = lat_2 * M_PI / 180;
    double delta_phi = (lat_2 - lat_1) * M_PI / 180;  // λ in radians
    double delta_delta = (lon_2 - lon_1) * M_PI / 180;

    double a = pow(sin(delta_phi / 2), 2) +
               cos(phi_1) * cos(phi_2) * pow(sin(delta_delta / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return earth_radius * c;  // in metres
}
double gererate_random(double min, double max){
     return ( (double)rand() * ( max - min ) ) / (double)RAND_MAX + min;
}
int max(int a, int b){
    return (a > b) ? a : b;
}