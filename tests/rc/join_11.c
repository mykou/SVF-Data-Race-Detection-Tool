/*
 * Simple alias check
 * Author: dye
 * Date: 03/07/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void *foo(void *threadid) {
    g = 10;
    RC_ACCESS(1, RC_ALIAS);
    RC_ACCESS(2, RC_ALIAS);
    RC_ACCESS(3, RC_ALIAS);

}

void *bar2(void *threadid) {
    g = 10;
    RC_ACCESS(3, RC_ALIAS);
    RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(5, RC_ALIAS);

}


void *bar(void *threadid) {
    g = 10;
    RC_ACCESS(1, RC_ALIAS);

    pthread_t tid;
    pthread_create(&tid, NULL, bar2, (void *)0);

    g = 10;
    RC_ACCESS(2, RC_ALIAS);
    RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);

    pthread_join(tid, 0);

    g = 10;
    RC_ACCESS(5, RC_ALIAS);

}



int main(int argc, char *argv[]) {

    pthread_t tid;
    pthread_create(&tid, NULL, foo, (void *)0);
    pthread_join(tid, 0);


    pthread_t tid2;
    pthread_create(&tid2, NULL, bar, (void *)0);
    pthread_join(tid2, 0);

}
