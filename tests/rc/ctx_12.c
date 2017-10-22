/*
 * Simple race check
 * Author: dye
 * Date: 17/03/2016
 */

#include "aliascheck.h"
#include "pthread.h"


void *foo(void *args) {
    int *p = (int *)args;
    *p = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_ALIAS | RC_MHP);

    return NULL;
}


int main(int argc, char *argv[]) {

    int x = 0;
    int y = 0;

    int *px = &x;
    int *py = &y;

    pthread_t t1, t2;
    pthread_create(&t1, NULL, foo, (void*) px);
    pthread_create(&t2, NULL, foo, (void*) py);

    x = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    y = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

    return 0;
}
