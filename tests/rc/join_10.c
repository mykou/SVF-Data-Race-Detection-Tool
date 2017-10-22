/*
 * Simple alias check
 * Author: dye
 * Date: 03/07/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  g = 10; 
  RC_ACCESS(1, RC_ALIAS);
  RC_ACCESS(1, RC_ALIAS);
  RC_ACCESS(2, RC_ALIAS);
  RC_ACCESS(3, RC_ALIAS);

}

void spawnJoinWrapper() {
    pthread_t tid;
    pthread_create(&tid, NULL, foo, (void *)0);
    pthread_join(tid, 0);
}

int main(int argc, char *argv[]) {

    spawnJoinWrapper();

    int x = g;
    RC_ACCESS(2, RC_ALIAS);
    printf("%d\n", x);

    spawnJoinWrapper();

    x = g;
    RC_ACCESS(3, RC_ALIAS);
    printf("%d\n", x);

}
