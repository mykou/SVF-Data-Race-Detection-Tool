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

}

int main(int argc, char *argv[]) {
  pthread_t thread[10];
  int ret;
  long t;
  for (int i = 0; i < argc; ++i) {
      pthread_create(&thread[i], NULL, foo, (void *)t);
  }
  for (int i = 0; i < argc; ++i) {
      pthread_join(thread[i], 0);
  }

  int x = g;
  RC_ACCESS(1, RC_ALIAS);
  printf("%d\n", x);

}
