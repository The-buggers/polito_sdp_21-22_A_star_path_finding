#include "Astar.h"
static void reconstruct_path_r(int *parentVertex, int j, double *costToCome,
                               double *tot_cost);
double heuristic_haversine(Position source, Position dest) {
    return POSITIONcompute_haversine_distance(source, dest);
}

double compute_f(double h, double g) { return h + g; }

int hash_function(int index, int Nthread) { return index % Nthread; }

double heuristic(Position p1, Position p2, char heuristic_type){
    if(heuristic_type == 'h'){
        heuristic_haversine(p1, p2);
    }
}

void reconstruct_path(int *parentVertex, int source, int dest,
                      double *costToCome) {
    int i;
    double tot_cost = 0;
    printf("+-----------------------------------+\n");
    printf("Path from %d to %d: [ ", source, dest);
    reconstruct_path_r(parentVertex, dest, costToCome, &tot_cost);
    printf("]");

    printf("\nCost: %.2lf\n", tot_cost);
    printf("+-----------------------------------+\n\n");
}

static void reconstruct_path_r(int *parentVertex, int j, double *costToCome,
                               double *tot_cost) {
    if (parentVertex[j] == -1) {
        return;
    } else {
        reconstruct_path_r(parentVertex, parentVertex[j], costToCome, tot_cost);
        if (parentVertex[parentVertex[j]] == -1) {
            printf("%d, ", parentVertex[j]);
            *tot_cost = *tot_cost + costToCome[parentVertex[j]];
        }
        printf("%d, ", j);
        *tot_cost = *tot_cost + costToCome[j];
    }
}