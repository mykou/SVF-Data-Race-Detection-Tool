/*
 * Simple alias check
 * Author: dye
 * Date: 02/09/2015
 */
#include "aliascheck.h"
#include "pthread.h"

typedef struct {
    int x;
    pthread_t tid;
} TdInfo;

int g = 0;
TdInfo *thread = 0;
int nthreads = 10;

void *foo(void *arg) {

    g = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

}


void bar() {
    g = 5;
    RC_ACCESS(2, RC_ALIAS);
    RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);

}

void terminate(void *args) {
    for (int i = 0; i < nthreads; ++i) {
        pthread_cancel(thread[i].tid);
    }

    bar();

    exit(0);
}


int main(int argc, char *argv[]) {

    nthreads = rand();

    thread = (TdInfo*)malloc(sizeof(TdInfo) * nthreads);

    pthread_t tid;
    pthread_create(&tid, NULL, terminate, NULL);

    int ret;
    for (int i = 0; i < nthreads; ++i) {
        ret = pthread_create(&thread[i].tid, NULL, foo, NULL);
        if (ret) {
            exit(-1);
        }
    }

    int x = g;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
    printf("%d\n", x);

}
