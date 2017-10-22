/*
 * Simple race check
 * Author: dye
 * Date: 10/11/2016
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_mutex_t l;


void bar() {
    g = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
}


void *foo(void *threadid) {
    pthread_mutex_lock(&l);
    bar();
    pthread_mutex_unlock(&l);

    bar();

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

  pthread_mutex_lock(&l);
  int x = g;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
  pthread_mutex_unlock(&l);

  printf("%d\n", x); 
}
