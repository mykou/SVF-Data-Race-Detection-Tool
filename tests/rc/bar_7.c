/*
 * Simple race check
 * Author: dye
 * Date: 26/07/2016
 */


#include "aliascheck.h"
#include "pthread.h"
#include "time.h"

pthread_barrier_t barrier;
int g;

int c;

void foo() {
    g = 1;
    RC_ACCESS(1, RC_ALIAS);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
}


void bar() {
    if (c < 10) {
        g = 2;
        RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    }
}


void *thread1 (void *arg) {
    time_t  now;

    time (&now);
    printf ("T1 starting at %s", ctime (&now));

    foo();
    bar();

    sleep (3);
    pthread_barrier_wait (&barrier);


    time (&now);
    printf ("barrier in T1 done at %s", ctime (&now));
}


void *thread2 (void *arg) {
    time_t  now;

    time (&now);
    printf ("T2 starting at %s", ctime (&now));

    if (c > 0) {
        foo();
    }

    sleep (5);
    pthread_barrier_wait (&barrier);

    bar();

    time (&now);
    printf ("barrier in T2 done at %s", ctime (&now));
}


int main (int argc, char **argv) {
    c = argc;

    time_t  now;

    // create a barrier object with a count of 3
    pthread_barrier_init (&barrier, NULL, 3);

    // start up two threads, thread1 and thread2
    pthread_t t1, t2;
    pthread_create (&t1, NULL, thread1, NULL);
    pthread_create (&t2, NULL, thread2, NULL);

    // at this point, thread1 and thread2 are running

    g = 0;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);


    // now wait for completion
    time (&now);
    printf ("main() waiting for barrier at %s", ctime (&now));
    pthread_barrier_wait (&barrier);


    g = 0;
    RC_ACCESS(1, RC_ALIAS);


    // after this point, all three threads have completed.
    time (&now);
    printf ("barrier in main() done at %s", ctime (&now));

    return 0;
}

