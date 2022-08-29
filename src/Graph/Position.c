#include "Position.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
struct position {
    double x;
    double y;
};
double POSITIONcompute_haversine_distance(Position p1, Position p2) {
    double lat_1 = p1->y;
    double lon_1 = p1->x;
    double lat_2 = p2->y;
    double lon_2 = p2->x;

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
double POSITIONcompute_manhattan_distance(Position p1, Position p2) {
    return abs(p1->x - p2->x) + abs(p1->y - p2->y);
}
int POSITIONcmp(Position p1, Position p2) {
    return (p1->x == p2->x && p1->y == p2->y);
}
double POSITIONget_latitude(Position p) {
    return p->y;
}
double POSITIONget_longitude(Position p) {
    return p->x;
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