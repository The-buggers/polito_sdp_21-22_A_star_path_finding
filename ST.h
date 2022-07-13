#ifndef ST_H
#define ST_H
#include "Position.h"
typedef struct symboltable *ST;
ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
void  STinsert(ST st, Position* pos, int node_index);
int   STsearch(ST st, Position *pos);
Position STsearchByIndex(ST st, int i);
#endif



