/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */

#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void bar() {
  g = 10; 
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
}

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;
  bar();
}

int main(int argc, char *argv[]) {
  pthread_t threads[10];
  int ret;
  long t;
  int i = 0;
  for (; i < 10; ++i) {
    ret = pthread_create(&threads[i], NULL, foo, (void *)t);
    if (ret){
      exit(-1);
    }
  }

}
