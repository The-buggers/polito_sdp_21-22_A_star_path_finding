#ifndef POSITION_H
#define POSITION_H
#include <stdio.h>
typedef struct position *Position;
Position POSITIONinit(int x, int y);
void POSITIONfree(Position p);
double POSITIONcompute_euclidean_distance(Position p1, Position p2);
int POSITIONcmp(Position p1, Position p2);
void POSITIONcpy(Position p1, Position p2);
void POSITIONprint(Position p, FILE* fout);
#endif