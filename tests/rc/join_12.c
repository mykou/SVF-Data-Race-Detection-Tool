/*
 * Simple alias check
 * Author: dye
 * Date: 19/08/2015
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
    RC_ACCESS(3, RC_ALIAS);

}


void terminate() {
    for (int i = 0; i < nthreads; ++i) {
        pthread_join(thread[i].tid, 0);
    }

    int x = g;
    RC_ACCESS(2, RC_ALIAS);
    printf("%d\n", x);
    exit(0);
}


int main(int argc, char *argv[]) {

    if (argc == 1) {
        nthreads = 5;
    }

    thread = (TdInfo*)malloc(sizeof(TdInfo) * nthreads);
    int ret;
    for (int i = 0; i < nthreads; ++i) {
        ret = pthread_create(&thread[i].tid, NULL, foo, NULL);
        if (ret) {
            exit(-1);
        }
    }

    if (argc == 7) {
        terminate();
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(thread[i].tid, 0);
    }

    int x = g;
    RC_ACCESS(3, RC_ALIAS);
    printf("%d\n", x);

}
