/*
 * Simple race check
 * Author: dye
 * Date: 07/11/2016
 */
#include "aliascheck.h"
#include "hare.h"

class C {
public:
    static int f1;
    static int f2;
};

int C::f1 = 0;
int C::f2 = 1;


void foo(void *arg, long start, long end) {

    C::f1 = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_MHP);

    C::f2 = 2;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_MHP);
}


int main(int argc, char *argv[]) {

    hare_init();

    hare_parallel_for(2, 0, 8, 1, foo, NULL, 4, 2);

    hare_finish();

    return 0;
}
