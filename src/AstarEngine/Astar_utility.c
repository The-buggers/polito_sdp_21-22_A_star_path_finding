#include "Astar.h"

double heuristic_euclidean(Position source, Position dest) {
    return POSITIONcompute_euclidean_distance(source, dest);
}

double compute_f(double h, double g) { return h + g; }
int hash_function(int index, int Nthread) { return index % Nthread; }