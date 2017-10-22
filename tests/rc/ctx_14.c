/*
 * Simple race check
 * Author: dye
 * Date: 15/06/2016
 */

#include "aliascheck.h"
#include "pthread.h"


void *bar(void *args) {
    int *p = (int *)args;
    *p = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
}


void foo(int *q) {
    pthread_t t1;
    pthread_create(&t1, NULL, bar, (void*) q);
}



int main(int argc, char *argv[]) {

    int x = 0;
    int *px = &x;

    for (int i = 0; i < argc; ++i) {
        foo(px);
    }

    return 0;
}
