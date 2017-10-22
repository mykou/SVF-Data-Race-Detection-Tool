/*
 * Simple alias check
 * Author: dye
 * Date: 31/07/2015
 */

#include "aliascheck.h"
#include "pthread.h"

int g = 0;
int use_single_thread = 0;


void *foo(void *threadid) {

    g = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    if (4 < use_single_thread) {
        g = 10;
        RC_ACCESS(2, RC_ALIAS);
        RC_ACCESS(2, RC_ALIAS);
    }

}

int main(int argc, char *argv[]) {
  pthread_t threads[10];
  int ret;
  long t;

  use_single_thread = argc;

  if (use_single_thread > 4) {
      pthread_create(&threads[0], NULL, foo, (void *)t);
  } else {
      for (int i = 0; i < 10; ++i) {
          pthread_create(&threads[i], NULL, foo, (void *)t);
      }
  }

}
