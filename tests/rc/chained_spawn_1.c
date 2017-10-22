/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void *bar(void *threadid) {
  long tid;
  tid = (long)threadid;

  g = 10; 
  RC_ACCESS(1, RC_ALIAS);
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

}

void *foo(void *threadid) {
  int y = g;
  RC_ACCESS(1, RC_ALIAS);
  printf("%d\n", y);

  pthread_t thread;
  int ret;
  long t;
  ret = pthread_create(&thread, NULL, bar, (void *)t);

  int x = g;
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
  printf("%d\n", x);

}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;
  int i = 0;
  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }

}
