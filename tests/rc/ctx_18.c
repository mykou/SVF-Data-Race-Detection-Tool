/*
 * Simple race check
 * Author: dye
 * Date: 09/08/2016
 */

#include "aliascheck.h"
#include "pthread.h"


void *bar(void *args) {
    int *p = (int *)args;
    *p = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
}


void foo(void *q) {
    pthread_t t1;
    pthread_create(&t1, NULL, bar, (void*) q);
}


int main(int argc, char *argv[]) {
    int x, y;
    foo(&x);
    foo(&y);

    return 0;
}
