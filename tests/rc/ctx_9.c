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
    int *f3;
} S;

typedef struct {
    int f1;
    int f2;
    S s;
    int f3;
} SS;


void setPtr(SS *ss) {
    ss->s.f3 = &(ss->f3);
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

    SS ss1;
    SS ss2;

    setPtr(&ss1);
    setPtr(&ss2);

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) ss1.s.f3);

    foo(ss2.s.f3);

    ss1.f3 = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    ss2.f3 = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP);

    return 0;
}
