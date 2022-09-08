#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./AstarEngine/Astar.h"
#include "./DijkstraEngine/Dijkstra.h"
#include "./Graph/Graph.h"
#define MAXC 10
#define MEASURE_ALGORITHM 1
void start_timer(struct timespec* begin);
double stop_timer(struct timespec begin);

int main(int argc, char* argv[]) {
    Graph G, R;
    FILE* fperf;
    struct timespec begin, end;
    int reading_type, reading_threads;
    char algo_type[MAXC], heuristic_type;
    int partitions_nodes, partitions_edges, th_nodes, th_edges, source, dest,
        num_threads;

    /////////////////
    /// READING ////
    ////////////////
    reading_type = atoi(argv[2]);
    start_timer(&begin);
    switch (reading_type) {
        case 3:
            // parallel reading approach 3
            partitions_nodes = atoi(argv[3]);
            partitions_edges = atoi(argv[4]);
            th_nodes = atoi(argv[5]);
            th_edges = atoi(argv[6]);
            strcpy(algo_type, argv[7]);
            if (strcmp(algo_type, "seq") != 0 &&
                strcmp(algo_type, "dijkstra") != 0 &&
                strcmp(algo_type, "pnba") != 0) {
                num_threads = atoi(argv[8]);
                source = atoi(argv[9]);
                dest = atoi(argv[10]);
                heuristic_type = argv[11][0];
            } else {
                source = atoi(argv[8]);
                dest = atoi(argv[9]);
                heuristic_type = argv[10][0];
            }
            printf("Parallel Reading Approach: 3\n");
            G = GRAPHload_parallel3(argv[1], partitions_nodes, partitions_edges,
                                    th_nodes, th_edges);
            break;
        case 0:
            // sequential reading
            strcpy(algo_type, argv[3]);
            if (strcmp(algo_type, "seq") != 0 &&
                strcmp(algo_type, "dijkstra") != 0 &&
                strcmp(algo_type, "pnba") != 0) {
                num_threads = atoi(argv[4]);
                source = atoi(argv[5]);
                dest = atoi(argv[6]);
                heuristic_type = argv[7][0];
            } else {
                source = atoi(argv[4]);
                dest = atoi(argv[5]);
                heuristic_type = argv[6][0];
            }
            printf("Sequential Reading\n");
            G = GRAPHload_sequential(argv[1]);
            break;
        case 4:
            // parallel reading approach 2 for PNBA*
            reading_threads = atoi(argv[3]);
            strcpy(algo_type, argv[4]);
            if (strcmp(algo_type, "seq") != 0 &&
                strcmp(algo_type, "dijkstra") != 0 &&
                strcmp(algo_type, "pnba") != 0) {
                num_threads = atoi(argv[5]);
                source = atoi(argv[6]);
                dest = atoi(argv[7]);
                heuristic_type = argv[8][0];
            } else {
                source = atoi(argv[5]);
                dest = atoi(argv[6]);
                heuristic_type = argv[7][0];
            }
            printf("Parallel Reading Approach: 2 - Read G and R\n");
            GRAPHload_parallel2(argv[1], reading_threads, &G, &R);
        default:
            // reading approach 1,2
            reading_threads = atoi(argv[3]);
            strcpy(algo_type, argv[4]);
            if (strcmp(algo_type, "seq") != 0 &&
                strcmp(algo_type, "dijkstra") != 0 &&
                strcmp(algo_type, "pnba") != 0) {
                num_threads = atoi(argv[5]);
                source = atoi(argv[6]);
                dest = atoi(argv[7]);
                heuristic_type = argv[8][0];
            } else {
                source = atoi(argv[5]);
                dest = atoi(argv[6]);
                heuristic_type = argv[7][0];
            }
            printf("Parallel Reading Approach: %d\n", reading_type);
            G = GRAPHload_parallel1(argv[1], reading_threads);
            break;
    }
    printf("Reading time: %.9f seconds\n\n", stop_timer(begin));
    fperf = fopen("out.txt", "a");

#if MEASURE_ALGORITHM
    //////////////////
    /// ALGORITHM ////
    //////////////////
    start_timer(&begin);
    if (strcmp(algo_type, "seq") == 0) {
        ASTARshortest_path_sequential(G, source, dest, heuristic_type);
    } else if (strcmp(algo_type, "dijkstra") == 0) {
        DIJKSTRA_shortest_path_sequential(G, source, dest);
    } else if (strcmp(algo_type, "sf") == 0) {
        ASTARshortest_path_sas_sf(G, source, dest, heuristic_type, num_threads);
    } else if (strcmp(algo_type, "b") == 0) {
        ASTARshortest_path_sas_b(G, source, dest, heuristic_type, num_threads);
    } else if (strcmp(algo_type, "sf2") == 0) {
        ASTARshortest_path_sas_sf_v2(G, source, dest, heuristic_type,
                                     num_threads);
    } else if (strcmp(algo_type, "pnba") == 0) {
        ASTARshortest_path_ab_ba(G, R, source, dest, heuristic_type);
    }
    printf("A* algorithm time: %.9f seconds\n#threads: %d\n", stop_timer(begin),
           num_threads);
    // fprintf(fperf, "%.9f\n", stop_timer(begin));
    fclose(fperf);
#endif
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