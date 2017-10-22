/*
 * Simple alias check
 * Author: dye
 * Date: 13/07/2015
 */
#include "aliascheck.h"
#include "pthread.h"


typedef struct {
    int f1;
    int f2;
} S;

S g = {0,0};

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;
  S s = {1,1};

  g = s;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;
  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }

  memset(&g, 0, sizeof(S));
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

}
