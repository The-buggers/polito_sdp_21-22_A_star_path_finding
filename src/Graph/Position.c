#include "Position.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
struct position {
    double x;
    double y;
};
double POSITIONcompute_euclidean_distance(Position p1, Position p2) {
    return sqrt(pow(p1->x - p2->x, 2) + pow(p1->y - p2->y, 2));
}
double POSITIONcompute_manhattan_distance(Position p1, Position p2){
    return abs(p1->x - p2->x) + abs(p1->y - p2->y);
}
int POSITIONcmp(Position p1, Position p2) {
    return (p1->x == p2->x && p1->y == p2->y);
}
void POSITIONcpy(Position p1, Position p2) {
    p1->x = p2->x;
    p1->y = p2->y;
}
void POSITIONprint(Position p, FILE *fout) {
    fprintf(fout, "[%.4lf;%.4lf]\n", p->x, p->y);
}

Position POSITIONinit(double x, double y) {
    Position p;

    p = malloc(sizeof(*p));
    if (p == NULL) return NULL;
    p->x = x;
    p->y = y;
    return p;
}

void POSITIONfree(Position p) { free(p); }