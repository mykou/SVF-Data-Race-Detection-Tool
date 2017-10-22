/*
 * Simple race check
 * Author: dye
 * Date: 01/11/2016
 */
#include "aliascheck.h"
#include "hare.h"

typedef struct {
    int *p1;
    int *p2;
} S;

void foo(void *arg, long start, long end) {

    S *q = (S*) arg;

    *q->p1 = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    *q->p2 = 20;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
}


int main(int argc, char *argv[]) {

    hare_init();

    int a, b;
    S s1, s2;
    s1.p1 = &a;
    s1.p2 = &b;
    s2.p1 = &b;
    s2.p2 = &a;

    foo(&s2, 0, 1);

    hare_parallel_for(2, 0, 8, 1, foo, &s1, 4, 2);

    hare_finish();

    return 0;
}
