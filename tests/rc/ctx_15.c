/*
 * Simple race check
 * Author: dye
 * Date: 08/08/2016
 */

#include "aliascheck.h"
#include "pthread.h"

typedef struct SomeStruct {
    int x;
    struct SomeStruct *next;
} S;


void *bar(void *args) {
    S *p = (S *)args;
    p->x = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
}


void foo(void *q) {
    pthread_t t1;
    pthread_create(&t1, NULL, bar, (void*) q);
}



int main(int argc, char *argv[]) {
    S s;
    S *p = &s;
    p->next = p;

    for (int i = 0; i < argc; ++i) {
        foo(p);
    }

    while (p->next) {
        p = p->next;
    }
    p->x = 2;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);


    return 0;
}
