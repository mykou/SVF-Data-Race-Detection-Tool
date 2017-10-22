/*
 * Simple race check
 * Author: dye
 * Date: 12/02/2016
 */

#include "aliascheck.h"
#include "pthread.h"


void bar(int *q) {
    *q = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_ALIAS | RC_MHP);
}


void *foo(void *args) {
    int *p = (int *)args;
    bar(p);

    return NULL;
}



int main(int argc, char *argv[]) {

    int x = 0;
    int y = 0;

    int *px = &x;
    int *py = &y;

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) px);

    bar(py);

    foo(py);

    x = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    y = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP);


    return 0;
}
