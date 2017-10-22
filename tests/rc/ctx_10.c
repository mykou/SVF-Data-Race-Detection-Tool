/*
 * Simple race check
 * Author: dye
 * Date: 15/02/2016
 */

#include "aliascheck.h"
#include "pthread.h"

int *g; // Used to cheat ThreadEscapeAnalysis

void bar(int *q) {
    *q = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);

    int *t = (int*)malloc(sizeof(int));
    *t = 2;
    RC_ACCESS(2, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP);

    g = t;
}


void *foo(void *args) {
    int *p = (int*)malloc(sizeof(int));
    bar(p);

    g = p;
    return NULL;
}



int main(int argc, char *argv[]) {
    pthread_t t;
    pthread_create(&t, NULL, foo, (void*) NULL);

    foo(NULL);

    return 0;
}
