#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define L 100


struct threadData {
    pthread_t threadId;
    int id;
    int line;
    int sum;
};

struct V_data {
    int n;
    int lat;
    int lon;
};

struct E_data {
    int n1;
    int n2;
    double e;
};

int fp;
sem_t sem;
int len = 0;
int retVal = 1;
struct V_data v;
struct E_data e;

static void *readFile(void *arg) {
    struct threadData *td;
    td = (struct threadData *)arg;

    while (retVal > 0) {
        if (len < 10) {
            sem_wait(&sem);
            retVal = read(fp, &v, sizeof(struct V_data));
            printf("ID %d RETVAL %d %d %d %d\n", td->id, retVal, v.n, v.lat, v.lon);
            len++;
            sem_post(&sem);            
        } else {
            sem_wait(&sem);
            retVal = read(fp, &e, sizeof(struct E_data));
            printf("ID %d RETVAL %d %d %d %.1f\n", td->id, retVal, e.n1, e.n2, e.e);
            sem_post(&sem);
        }
        
        sleep(1);  // Delay Threads
    }

    fprintf(stdout, "Thread: %d\n", td->id);
    pthread_exit((void *)1);
}

int main(int argc, char *argv[]) {
    int i, nV, total, line;
    struct threadData *td;
    void *retval;
    fp = open(argv[2], O_RDONLY);
    sem_init(&sem, 0, 1);
    read(fp, &nV, sizeof(int));
    printf("%d\n", nV);
    td = (struct threadData *)malloc(nV * sizeof(struct threadData));
    for (i = 0; i < nV; i++) {
        td[i].id = i;
        td[i].line = td[i].sum = 0;
        pthread_create(&(td[i].threadId), NULL, readFile, (void *)&td[i]);
    }
    total = line = 0;
    for (i = 0; i < nV; i++) {
        pthread_join(td[i].threadId, &retval);
        total = td[i].sum;
        line += td[i].line;
    }
    fprintf(stdout, "Total: Sum=%d #Line=%d\n", total, line);
    sem_destroy(&sem);
    close(fp);
    return (1);
}