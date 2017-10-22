/*
 * Simple race check
 * Author: dye
 * Date: 01/03/2016
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_mutex_t l;


void lock() {
    pthread_mutex_lock(&l);
}


void unlock() {
    pthread_mutex_unlock(&l);
}


void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  lock();
  g = 5;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
  unlock();

  g = 10; 
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

}


int main(int argc, char *argv[]) {
  pthread_t thread[10];
  int ret;
  long t;
  int i = 0;
  for (; i < 10; ++i) {
      ret = pthread_create(&thread[i], NULL, foo, (void *)t);
      if (ret){
        exit(-1);
      }
  }

  int x = g;

  printf("%d\n", x); 
  return 0;
}
