#include <stdio.h>
#include <stdlib.h>
#define M 1000000
int main(int argc, char* argv[]){
    FILE *fp_co, *fp_gr, *fp_out;
    int num_nodes, num_edges, i, v, id1, id2;
    double x, y, wt;

    num_nodes = atoi(argv[1]);
    num_edges = atoi(argv[2]);
    fp_co = fopen(argv[3], "r");
    fp_gr = fopen(argv[4], "r");
    fp_out = fopen(argv[5], "w+");

    // First line: number of nodes
    fprintf(fp_out, "%d\n", num_nodes);

    // num_nodes lines: nodes coordinates
    for(i=0; i<num_nodes; i++){
        fscanf(fp_co, "%*c %d %lf %lf\n", &v, &x, &y);
        v -= 1;
        x /= M;
        y /= M;
        fprintf(fp_out, "%d %lf %lf\n", v, x, y);
    }

    // num_edges lines: edges
    for(i=0; i<num_edges; i++){
        fscanf(fp_gr, "%*c %d %d %lf\n", &id1, &id2, &wt);
        id1 -= 1;
        id2 -= 1;
        wt /= 10;
        fprintf(fp_out, "%d %d %lf\n", id1, id2, wt);
    }

    fclose(fp_out);
    fclose(fp_co);
    fclose(fp_gr);
}