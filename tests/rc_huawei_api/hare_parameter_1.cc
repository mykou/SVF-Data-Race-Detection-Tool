/*
 * Simple race check
 * Author: dye
 * Date: 01/11/2016
 */
#include "aliascheck.h"
#include "hare.h"


void foo(void *arg, long start, long end) {

    int *q = (int*) arg;
    *q = 10;
    RC_ACCESS(1, RC_ALIAS);
}


int main(int argc, char *argv[]) {

    hare_init();

    int x;
    int *p = &x;

    hare_parallel_for(2, 0, 8, 1, foo, p, 4, 2);

    x = 0;
    RC_ACCESS(1, RC_ALIAS);

    hare_finish();

    return 0;
}
