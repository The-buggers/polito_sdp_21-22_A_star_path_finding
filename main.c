#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Astar.h"
#include "Graph.h"
#define MAXC 11

#define TRY 20
#define NT 11

int main(int argc, char* argv[]) {
    Graph G;
    int fin, T;
    double time[NT];
    struct timespec begin, end;

    FILE* f = fopen("stats_spinlock.txt", "w");

    for (int i = 0; i < TRY; i++) {
        T = NT;
        while (T > 0) {
            fin = open(argv[1], O_RDONLY);
            if (fin < 0) exit(-1);
            clock_gettime(CLOCK_REALTIME, &begin);
            G = GRAPHloadParallel(fin, T);
            close(fin);
            clock_gettime(CLOCK_REALTIME, &end);
            ASTARshortest_path(G, 0, 23943);
            long seconds = end.tv_sec - begin.tv_sec;
            long nanoseconds = end.tv_nsec - begin.tv_nsec;
            double elapsed = seconds + nanoseconds * 1e-9;

            time[T] += elapsed;

            // TODO: add check on input nodes
            // GRAPHstore(G, stdout);
            GRAPHfree(G);
            T--;
        }
        printf("%d\n", i);
    }
    // fprintf(f, "\n");
    for (int i = 0; i < NT; i++) {
        time[i] = time[i] / (double)TRY;
        fprintf(f, "%d %.9f\n", i, time[i]);
    }
    fclose(f);
    return 0;
}
