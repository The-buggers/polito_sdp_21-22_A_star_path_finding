#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./AstarEngine/Astar.h"
#include "./DijkstraEngine/Dijkstra.h"
#include "./Graph/Graph.h"
#define MAXC 11
#define PARALLELREADTYPE 3
void start_timer(struct timespec* begin);
double stop_timer(struct timespec begin);

int main(int argc, char* argv[]) {
    Graph G;
    FILE* fperf;
    struct timespec begin, end;
    int partitions_nodes, partitions_edges, th_nodes, th_edges, source, dest, num_threads;
    char heuristic_type;

    // Get command line parameters
    source = atoi(argv[2]);
    dest = atoi(argv[3]);
    num_threads = atoi(argv[4]);
    heuristic_type = argv[5][0];

    // Start computation
    start_timer(&begin);
#if PARALLELREADTYPE == 3
    // ######################################
    // ### TEST PARALLEL READ - VERSION 3 ###
    // ######################################
    printf("Parallel read type: 3\n");
    G = GRAPHload_parallel3(argv[1], 3, 3, 2, 2);
    /*
    fperf = fopen("performance.txt", "w+");
    for (partitions_nodes = 1; partitions_nodes < 3; partitions_nodes++) {
        for (partitions_edges = 1; partitions_edges < 3; partitions_edges++) {
            for (th_nodes = 1; th_nodes < 2; th_nodes++) {
                for (th_edges = 1; th_edges < 2; th_edges++) {
                    clock_gettime(CLOCK_REALTIME, &begin);
                    G = GRAPHload_parallel3(argv[1], partitions_nodes,
                                            partitions_edges, th_nodes,
                                            th_edges);
                    clock_gettime(CLOCK_REALTIME, &end);
                    GRAPHfree(G);
                    seconds = end.tv_sec - begin.tv_sec;
                    nanoseconds = end.tv_nsec - begin.tv_nsec;
                    elapsed = seconds + nanoseconds * 1e-9;
                }
            }
        }
    }*/
#elif PARALLELREADTYPE == 2
    // ######################################
    // ### TEST PARALLEL READ - VERSION 2 ###
    // ######################################
    printf("Parallel read type: 2\n");
    G = GRAPHload_parallel2(argv[1], 2);
#elif PARALLELREADTYPE == 1
    // ######################################
    // ### TEST PARALLEL READ - VERSION 1 ###
    // ######################################
    printf("Parallel read type: 1\n");
    G = GRAPHload_parallel1(argv[1], 10);
#elif PARALLELREADTYPE == 0
    // ############################
    // ### TEST SEQUENTIAL READ ###
    // ############################
    printf("Sequential read\n");
    G = GRAPHload_sequential(argv[1]);
#endif
    printf("Reading time: %.9f seconds\n\n", stop_timer(begin));

    start_timer(&begin);
    //ASTARshortest_path_sequential(G, source, dest, heuristic_type);
    ASTARshortest_path_sas_sf(G, source, dest, heuristic_type, num_threads);
    //ASTARshortest_path_sas_b(G, source, dest, heuristic_type, num_threads);
    //ASTARshortest_path_fa(G, source, dest, heuristic_type, num_threads);
    //ASTARshortest_path_mp(G, source, dest, heuristic_type, num_threads);
    //DIJKSTRA_shortest_path_sequential(G, source, dest);
    printf("A* algorithm time: %.9f seconds\n", stop_timer(begin));
    return 0;
}

void start_timer(struct timespec* begin) {
    clock_gettime(CLOCK_REALTIME, begin);
}
double stop_timer(struct timespec begin) {
    long seconds, nanoseconds;
    double elapsed;
    struct timespec end;

    clock_gettime(CLOCK_REALTIME, &end);
    seconds = end.tv_sec - begin.tv_sec;
    nanoseconds = end.tv_nsec - begin.tv_nsec;
    elapsed = seconds + nanoseconds * 1e-9;
    return elapsed;
}