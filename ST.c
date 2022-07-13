#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ST.h"
//#include "Position.h"
struct symboltable
{
  Position *p; // dynamic array of positions
  int maxN;
  int N;
};
ST STinit(int maxN)
{
  ST st;
  st = malloc(sizeof(*st));
  if (st == NULL)
  {
    printf("Memory allocation error\n");
    return NULL;
  }
  st->p = (Position *)malloc(maxN * sizeof(Position));
  if (st->p == NULL)
  {
    printf("Memory allocation error\n");
    free(st);
    return NULL;
  }

  for (int i = 0; i < maxN; i++)
  {
    st->p[i] = POSITIONinit(0, 0);
    if (st->p[i] == NULL)
    {
      printf("Memory allocation error\n");
      return NULL;
    }
  }

  st->maxN = maxN;
  st->N = 0;
  return st;
}
void STfree(ST st)
{
  int i;
  if (st == NULL)
    return;
  for (i = 0; i < st->N; i++)
    if (st->p[i] != NULL)
      free(st->p[i]);
  free(st->p);
  free(st);
}
int STsize(ST st)
{
  return st->N;
}
void STinsert(ST st, Position *pos, int node_index)
{
  if (node_index >= st->maxN)
  {
    st->p = realloc(st->p, (2 * st->maxN) * sizeof(char *));
    if (st->p == NULL)
      return;
    st->maxN = 2 * st->maxN;
  }
  POSITIONcpy(st->p[node_index], pos);
  st->N++;
}
int STsearch(ST st, Position *pos)
{
  int i;
  for (i = 0; i < st->N; i++)
    if (st->p[i] != NULL && POSITIONcmp(pos, st->p[i]) == 1)
      return i;
  return -1;
}
Position STsearchByIndex(ST st, int i)
{
  if (i < 0 || i >= st->N)
    return NULL;
  return (st->p[i]);
}
