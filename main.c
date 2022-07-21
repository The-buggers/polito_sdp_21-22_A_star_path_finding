#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Astar.h"
#include "Graph.h"
#define MAXC 11

int main(int argc, char* argv[]) {
    Graph G;
    int fin, T;
    struct timespec begin, end;

    FILE* f = fopen("stats.txt", "w");

    for (int i = 0; i < 1; i++) {
        T = 20;
        while (T > 0) {
            fin = open(argv[1], O_RDONLY);
            if (fin < 0) exit(-1);
            clock_gettime(CLOCK_REALTIME, &begin);
            G = GRAPHloadParallel(fin, T);
            close(fin);
            ASTARshortest_path(G, 0, 23943);
            clock_gettime(CLOCK_REALTIME, &end);
            long seconds = end.tv_sec - begin.tv_sec;
            long nanoseconds = end.tv_nsec - begin.tv_nsec;
            double elapsed = seconds + nanoseconds * 1e-9;

            fprintf(f, "%d %.9f\n", T, elapsed);

            // TODO: add check on input nodes
            // GRAPHstore(G, stdout);
            GRAPHfree(G);
            T--;
        }
        fprintf(f, "-------------------------------------------\n");
    }
    fclose(f);
    return 0;
}
