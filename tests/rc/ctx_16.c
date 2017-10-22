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
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
}


void foo(void *q) {
    pthread_t t1;
    pthread_create(&t1, NULL, bar, (void*) q);
}


S *mallocS() {
    return (S*)malloc(sizeof(S));
}


int main(int argc, char *argv[]) {

    S *p = mallocS();
    S *q = mallocS();
    p->next = q;

    for (int i = 0; i < argc; ++i) {
        foo(q);
    }

    p->x = 2;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);


    while (p->next) {
        p = p->next;
    }
    p->x = 2;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);


    return 0;
}
