#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
struct node_line_s{
    int node_index;
    int node_x;
    int node_y;
};
struct edge_line_s{
    int id1;
    int id2;
    double wt;
};

int main(int argc, char** argv) {
    FILE *fp;
    int V, i, node_index, node_x, node_y;
    int fd;
    struct node_line_s nl;
    struct edge_line_s el;

    fp = fopen(argv[1], "r");
    fd = open(argv[2], O_RDWR|O_CREAT, 0777);

    fscanf(fp, "%d", &V);
    write(fd, &V, sizeof(int));

    for (i = 0; i < V; i++)
    {
        fscanf(fp, "%d %d %d", &nl.node_index, &nl.node_x, &nl.node_y);
        write(fd, &nl, sizeof(struct node_line_s));
    }

    while (fscanf(fp, "%d %d %lf", &el.id1, &el.id2, &el.wt) == 3)
    {
        write(fd, &el, sizeof(struct edge_line_s));
    }

    fclose(fp);
    close(fd);
}