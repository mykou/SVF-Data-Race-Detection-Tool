/*
 * Simple race check
 * Author: dye
 * Date: 31/10/2016
 */
#include "aliascheck.h"
#include "hare.h"

int g = 0;

void foo(void *arg, long start, long end) {

    g = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

}


int main(int argc, char *argv[]) {

    hare_init();

    hare_parallel_for(2, 0, 8, 1, foo, NULL, 4, 2);

    hare_finish();

    return 0;
}
