/*
 * Simple race check
 * Author: dye
 * Date: 01/11/2016
 */
#include "aliascheck.h"
#include "hare.h"

typedef struct {
    int *p;
    int x;
} S;

void foo(void *arg, long start, long end) {

    S *q = (S*) arg;

    q->x = 10;
    RC_ACCESS(1, RC_ALIAS);

    *q->p = 20;
    RC_ACCESS(2, RC_ALIAS);

}


int main(int argc, char *argv[]) {

    hare_init();

    int a;
    S s;
    s.p = &a;

    hare_parallel_for(2, 0, 8, 1, foo, &s, 4, 2);

    s.x = 0;
    RC_ACCESS(1, RC_ALIAS);

    a = 0;
    RC_ACCESS(2, RC_ALIAS);

    hare_finish();

    return 0;
}
