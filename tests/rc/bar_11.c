/*
 * Simple race check
 * Author: dye
 * Date: 20/09/2016
 */


#include "aliascheck.h"
#include "pthread.h"
#include "time.h"

pthread_barrier_t barrier;
int g;


void barrier_wait_wrapper(pthread_barrier_t *b) {
    pthread_barrier_wait(b);
}


void *thread (void *arg) {

    for (int i = 0; i < 100; ++i) {
        g = i;
        RC_ACCESS(1, RC_ALIAS);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);


        sleep (3);
        printf ("Non-main thread waiting for barrier at iteration %d\n", i);
        barrier_wait_wrapper (&barrier);

        g = i;
        RC_ACCESS(3, RC_ALIAS);
        RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
    }

}


int main () {

    // create a barrier object with a count of 2
    pthread_barrier_init (&barrier, NULL, 2);

    // start up a thread
    pthread_t t;
    pthread_create (&t, NULL, thread, NULL);


    for (int i = 0; i < 100; ++i) {
        g = i;
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(3, RC_ALIAS);

        sleep (1);
        printf ("Main thread waiting for barrier at iteration %d\n", i);
        barrier_wait_wrapper (&barrier);

        g = i;
        RC_ACCESS(1, RC_ALIAS);
        RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
    }


    return 0;
}

