/*
 * Simple race check
 * Author: dye
 * Date: 15/02/2016
 */

#include "aliascheck.h"
#include "pthread.h"


typedef struct {
    int f1;
    int f2;
} S;


void setPtr(int **t, int *v) {
    *t = v;
}


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

    S s1;
    S s2;

    int *p1;
    int *p2;

    setPtr(&p1, &s1.f2);
    setPtr(&p2, &s2.f2);

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) p1);

    foo(p2);

    s1.f2 = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    s2.f2 = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP);

    return 0;
}
