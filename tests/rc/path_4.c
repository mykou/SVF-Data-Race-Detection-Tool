/*
 * Simple race check
 * Author: dye
 * Date: 05/01/2016
 */
#include "aliascheck.h"
#include "pthread.h"


int c1, c2 = 0;

int g1, g2 = 0;


void *foo(void *args) {
    if (c1) {
        g1 = 1;
        RC_ACCESS(1, RC_ALIAS);
    }

    if (c2) {
        g2 = 1;
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

        c2 = rand();
    }
}


void *bar(void *args) {
    if (!c1) {
        g1 = 2;
        RC_ACCESS(1, RC_ALIAS);
    }

    if (!c2) {
        g2 = 2;
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    }
}


int main(int argc, char *argv[]) {

    c1 = c2 = rand();

    pthread_t t1, t2;
    pthread_create(&t1, NULL, foo, (void*) NULL);
    pthread_create(&t2, NULL, bar, (void*) NULL);


    return 0;
}
