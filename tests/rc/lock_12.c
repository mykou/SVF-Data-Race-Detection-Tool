/*
 * Simple race check
 * Author: dye
 * Date: 04/03/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_mutex_t l;



void bar1() {
    g = 5;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
}


void bar2() {
    g = 5;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
}


void *foo(void *arg) {
  long x = (long)arg;

  if (x == 1) {
      pthread_mutex_lock(&l);
  }

  bar1();

  if (x == 1) {
      bar2();
  }

  pthread_mutex_unlock(&l);

  return NULL;
}



int main(int argc, char *argv[]) {
  pthread_t thread[10];
  int ret;
  long t = 1;

  for (int i = 0; i < 10; ++i) {
      ret = pthread_create(&thread[i], NULL, foo, (void *)t);
      if (ret){
        exit(-1);
      }
  }

  g = 1;
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);


  return 0;
}
