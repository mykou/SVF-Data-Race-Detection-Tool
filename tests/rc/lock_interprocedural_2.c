/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_mutex_t l;

void lock_wrapper(pthread_mutex_t *l) {
l1: pthread_mutex_lock(l);
    if (rand()) return;
    pthread_mutex_unlock(l);
    goto l1;
}

void unlock_wrapper(pthread_mutex_t *l) {
    pthread_mutex_unlock(l);
}

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  lock_wrapper(&l);
  g = 10; 
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
  unlock_wrapper(&l);

}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;
  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }

  lock_wrapper(&l);
  int x = g;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
  unlock_wrapper(&l);

  printf("%d\n", x); 
}
