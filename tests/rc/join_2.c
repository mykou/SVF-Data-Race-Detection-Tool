/*
 * Simple alias check
 * Author: dye
 * Date: 23/06/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  g = 10; 
  RC_ACCESS(1, RC_ALIAS);
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;
  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }

  if (rand()) {
      pthread_join(thread, 0);
      int x = g;
      RC_ACCESS(1, RC_ALIAS);
      printf("%d\n", x);
  }
  else {
      int y = g;
      RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
      printf("%d\n", y);
  }

  int z = g;
  RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
  printf("%d\n", z);

}
