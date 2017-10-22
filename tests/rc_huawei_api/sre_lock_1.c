/*
 * Simple race check
 * Author: dye
 * Date: 01/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"
#include "sre.h"

int g = 0;
SRE_LOCK_ID l;

void *foo(void *arg) {

    SRE_SplSpecLock(l);
    g = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    SRE_SplSpecUnlock(l);

    return NULL;
}

int main(int argc, char *argv[]) {

    l = SRE_LockCreate(SRE_SPINLOCK_TYPE);

    pthread_t thread;
    int ret;
    ret = pthread_create(&thread, NULL, foo,NULL);
    if (ret) {
        exit(-1);
    }

    int x = g;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    printf("%d\n", x);
}
