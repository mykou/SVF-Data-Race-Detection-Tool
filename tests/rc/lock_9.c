/*
 * Simple alias check
 * Author: dye
 * Date: 20/03/2016
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_rwlock_t l;


void *foo(void *arg) {
  long x;
  x = (long)arg;

  pthread_rwlock_rdlock(&l);
  g = 10; 
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
  pthread_rwlock_unlock(&l);

  g = 5;
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

  return NULL;
}



int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;
  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }

  pthread_rwlock_rdlock(&l);
  int x = g;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_PROTECTED);
  pthread_rwlock_unlock(&l);

  x = g;
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

  printf("%d\n", x); 

  return 0;
}
