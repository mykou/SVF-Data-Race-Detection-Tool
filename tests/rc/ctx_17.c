/*
 * Simple race check
 * Author: dye
 * Date: 09/08/2016
 */

#include "aliascheck.h"
#include "pthread.h"

struct SomeStruct1;
struct SomeStruct2;

typedef struct SomeStruct1 {
    int x;
    struct SomeStruct2 *next;
} S1;

typedef struct SomeStruct2 {
    int y;
    struct SomeStruct1 *next;
} S2;


void *bar(void *args) {
    S1 *p = (S1 *)args;
    p->x = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_MHP);
}


void foo(void *q) {
    pthread_t t1;
    pthread_create(&t1, NULL, bar, (void*) q);
}


int main(int argc, char *argv[]) {

    S1 s1;
    S2 s2;
    s1.next = &s2;
    s2.next = &s1;

    for (int i = 0; i < argc; ++i) {
        foo(&s1);
    }

    S1 *p = &s1;
    p = p->next->next;
    p->x = 2;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    p->next->y = 3;
    RC_ACCESS(2, RC_MHP);


    return 0;
}
