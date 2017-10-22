/*
 * Simple race check
 * Author: dye
 * Date: 23/09/2016
 */


#include "aliascheck.h"
#include "pthread.h"
#include "time.h"

pthread_barrier_t barrier_1;
pthread_barrier_t barrier_2;
int g;

void *thread (void *arg) {
    int tid = (int)arg;

    g = 1;
    RC_ACCESS(1, RC_ALIAS);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    sleep (3);
    printf ("Spawned thread %d waiting for barrier_1\n", tid);
    pthread_barrier_wait (&barrier_1);

    g = 1;
    RC_ACCESS(1, RC_ALIAS);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

}


int main () {
    // create a barrier object
    pthread_barrier_init (&barrier_1, NULL, 2);
    pthread_barrier_init (&barrier_2, NULL, 1);

    // start up 10 threads
    pthread_t t;
    pthread_create (&t, NULL, thread, 1);
    pthread_create (&t, NULL, thread, 2);

    g = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

    sleep (1);
    printf ("Main thread waiting for barrier_2\n");
    pthread_barrier_wait (&barrier_2);

    g = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    return 0;
}

