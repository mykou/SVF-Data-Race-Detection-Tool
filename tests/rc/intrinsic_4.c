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
pthread_mutex_t l;

void access() {
    S s = {1,1};
    g = s;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_PROTECTED);
}


void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  pthread_mutex_lock(&l);
  access();
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

  memset(&g, 0, sizeof(S));
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

  pthread_mutex_lock(&l);
  memset(&g, 0, sizeof(S));
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_PROTECTED);
  pthread_mutex_unlock(&l);

}
