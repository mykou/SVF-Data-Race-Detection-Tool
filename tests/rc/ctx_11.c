/*
 * Simple race check
 * Author: dye
 * Date: 15/02/2016
 */

#include "aliascheck.h"
#include "pthread.h"



void bar(int *q) {
    *q = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    int *t = (int*)malloc(sizeof(int));
    *t = 2;
    RC_ACCESS(2, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP);
}


void *foo(void *args) {
    int *p = (int*) args;
    bar(p);

    return NULL;
}



int main(int argc, char *argv[]) {
    int *a = (int*)malloc(sizeof(int));

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) a);

    foo(a);

    return 0;
}
