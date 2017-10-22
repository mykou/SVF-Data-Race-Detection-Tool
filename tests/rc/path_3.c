/*
 * Simple race check
 * Author: dye
 * Date: 05/01/2016
 */
#include "aliascheck.h"
#include "pthread.h"


int c = 0;

int g = 0;


void *foo_1(void *args) {
    if (!c) {
        g = 1;
        RC_ACCESS(1, RC_ALIAS);
        RC_ACCESS(1, RC_ALIAS);
    }
    return NULL;
}


void *foo_2(void *args) {
    if (!c) {
        g = 1;
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    }
    return NULL;
}


void *foo_3(void *args) {
    if (!c) {
        g = 1;
        RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    }
    return NULL;
}


void *foo_4(void *args) {
    c = rand();
    if (!c) {
        g = 1;
        RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
    }
    return NULL;
}



int main(int argc, char *argv[]) {

    c = rand();

    if (c) {
        pthread_t t1, t2;
        pthread_create(&t1, NULL, foo_1, (void*) NULL);
        pthread_create(&t2, NULL, foo_1, (void*) NULL);
        pthread_join(t1, 0);
        pthread_join(t2, 0);
    }

    if (c) {
        c = rand();
        pthread_t t1, t2;
        pthread_create(&t1, NULL, foo_2, (void*) NULL);
        pthread_create(&t2, NULL, foo_2, (void*) NULL);
        pthread_join(t1, 0);
        pthread_join(t2, 0);
    }

    if (c) {
        pthread_t t1, t2;
        pthread_create(&t1, NULL, foo_3, (void*) NULL);
        pthread_create(&t2, NULL, foo_3, (void*) NULL);
        c = rand();
        pthread_join(t1, 0);
        pthread_join(t2, 0);
    }

    if (c) {
        pthread_t t1, t2;
        pthread_create(&t1, NULL, foo_4, (void*) NULL);

        int x = g;
        RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
        print("%d", x);

        pthread_join(t1, 0);
    }



    return 0;
}
