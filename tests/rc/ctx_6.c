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


void bar(int *q) {
    *q = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_ALIAS | RC_MHP);

}


void *foo(void *args) {
    S *p = (S *)args;
    bar(&(p->f2));

    return NULL;
}



int main(int argc, char *argv[]) {

    S s1, s2;

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) &s1);

    bar(&s1.f1);

    foo(&s2);

    s1.f2 = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    s2.f2 = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP);

    return 0;
}
