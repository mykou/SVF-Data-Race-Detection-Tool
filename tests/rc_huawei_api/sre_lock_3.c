/*
 * Simple race check
 * Author: dye
 * Date: 03/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"
#include "sre.h"

int g = 0;
SRE_LOCK_ID l;

void *foo(void *arg) {

    g = 5;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    SRE_SplSpecLock(l);
    g = 10;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_PROTECTED);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_PROTECTED);
    SRE_SplSpecUnlock(l);

    return NULL;
}

int main(int argc, char *argv[]) {

    l = SRE_LockCreate(SRE_SPINLOCK_TYPE);

    pthread_t thread[10];
    int ret;
    long t;
    int i = 0;
    for (; i < 10; ++i) {
        ret = pthread_create(&thread[i], NULL, foo, NULL);
        if (ret) {
            exit(-1);
        }
    }

    int x = g;

    printf("%d\n", x);
}
