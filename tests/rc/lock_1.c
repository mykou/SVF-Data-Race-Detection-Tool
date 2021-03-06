/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_mutex_t l;

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  pthread_mutex_lock(&l);
  g = 10; 
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
  pthread_mutex_unlock(&l);

}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;
  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }

  int x = g;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

  printf("%d\n", x); 
}
