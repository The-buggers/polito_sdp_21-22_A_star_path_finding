#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Astar.h"
#include "Graph.h"
#define MAXC 11

int main(int argc, char* argv[]) {
    Graph G;
    FILE* fperf;
    struct timespec begin, end;
    int partitions_nodes, partitions_edges, th_nodes, th_edges;
    long seconds, nanoseconds;
    double elapsed;
    // Measure parallel read performance
    clock_gettime(CLOCK_REALTIME, &begin);
    // G = GRAPHload_parallel3(argv[1], 100, 100, 5, 5);
    // G = GRAPHload_parallel2(fin);
    ASTARshortest_path(G, 0, 23943);
    clock_gettime(CLOCK_REALTIME, &end);
    GRAPHfree(G);
    seconds = end.tv_sec - begin.tv_sec;
    nanoseconds = end.tv_nsec - begin.tv_nsec;
    elapsed = seconds + nanoseconds * 1e-9;
    fprintf(stdout, "%d %d %d %d: %.9f s\n", 100, 100, 5, 5, elapsed);
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
                    fprintf(fperf, "%d %d %d %d: %.9f s\n", partitions_nodes,
                            partitions_edges, th_nodes, th_edges, elapsed);
                }
            }
        }
    }
    */
    // G= GRAPHload_sequential(argv[1]);
    //  GRAPHstore(G, stdout);
    /*
    fin = fopen(argv[1], "r");
    if (fin == NULL)
      exit(-1);
    G = GRAPHload(fin);
    clock_gettime(CLOCK_REALTIME, &begin);
    ASTARshortest_path(G, 0, 23943);
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("Time measured: %.9f seconds.\n", elapsed);
    // TODO: add check on input nodes
    //GRAPHstore(G, stdout);
    GRAPHfree(G);
    */
    return 0;
}
