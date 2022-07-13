#include <stdlib.h>
#include <stdio.h>
#include "Position.h"
struct position
{
    int x;
    int y;
};
float compute_euclidean_distance(Position p1, Position p2){
    return 0;
}
int POSITIONcmp(Position p1, Position p2){
    return (p1->x == p2->x && p1->y == p2->y);
}
void POSITIONcpy(Position p1, Position p2){
    p1->x = p2->x;
    p1->y = p2->y;
}
void POSITIONprint(Position p, FILE *fout){
    fprintf(fout, "[%d;%d]\n", p->x, p->y);
}

Position POSITIONinit(int x, int y){
    Position p;

    p = malloc(sizeof(*p));
    if(p == NULL)
        return NULL;
    p->x = x;
    p->y = y;
    return p;
}

void POSITIONfree(Position p){
    free(p);
}