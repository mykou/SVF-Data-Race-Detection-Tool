/*
 * Simple race check
 * Author: dye
 * Date: 18/01/2016
 */

#include "aliascheck.h"
#include "pthread.h"


int c = 0;
int g = 0;


void assignmentWrapper(int x) {
    g = x;
    RC_ACCESS(1, RC_ALIAS);
}


void *foo(void *args) {
    if (!c) {
        // Assignment with wrapper
        assignmentWrapper(1);
        assignmentWrapper(1);
    }

    return NULL;
}


int main(int argc, char *argv[]) {

    c = rand();

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) NULL);

    if (c) {
        // Direct assignment
        g = 2;
        RC_ACCESS(1, RC_ALIAS);
    }

    return 0;
}
