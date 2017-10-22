/*
 * Simple race check
 * Author: dye
 * Date: 20/01/2016
 */

#include "aliascheck.h"
#include "pthread.h"


int c1 = 0, c2 = 0, c3 = 0;
int g = 0;


void assignmentWrapper(int x) {
    g = x;
    RC_ACCESS(1, RC_ALIAS);
}


void bar1() {
    if (!c1 && !c3) {
        // Assignment with wrapper
        assignmentWrapper(1);
    }
}


void bar2() {
    if (c1 || c3) {
        // ...
    }
    else {
        // Assignment with wrapper
        assignmentWrapper(2);
    }
}


void *foo(void *args) {
    if (c1) {
        // Direct assignment
        g = 1;
        RC_ACCESS(1, RC_ALIAS);
    }

    return NULL;
}


int main(int argc, char *argv[]) {

    c1 = rand();
    c2 = rand();
    c3 = rand();

    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) NULL);

    if (!c1 && c2) {
        // Assignment with wrapper
        assignmentWrapper(2);
    }

    bar1();

    bar2();

    return 0;
}
