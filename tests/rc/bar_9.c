/*
 * Simple race check
 * Author: dye
 * Date: 05/08/2016
 */


#include "aliascheck.h"
#include "pthread.h"
#include "time.h"

pthread_barrier_t barrier;
int g;

void *thread (void *arg) {
    int tid = (int)arg;

    for (int i = 0; i < 100; ++i) {
        g = i;
        RC_ACCESS(1, RC_ALIAS);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);


        sleep (3);
        printf ("Thread %d waiting for barrier at iteration %d\n", tid, i);
        pthread_barrier_wait (&barrier);

        g = i;
        RC_ACCESS(1, RC_ALIAS);
        RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    }

}


int main () {

    // create a barrier object with a count of 10
    pthread_barrier_init (&barrier, NULL, 10);

    // start up 10 threads
    pthread_t t[10];
    for (int i = 0; i < 10; ++i) {
        pthread_create (&t[i], NULL, thread, i);
    }

    return 0;
}

