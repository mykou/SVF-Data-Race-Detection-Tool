/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  g = 10; 
  RC_ACCESS(1, RC_ALIAS);

}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;
  long t;

  int x = g;
  RC_ACCESS(1, RC_ALIAS);

  ret = pthread_create(&thread, NULL, foo, (void *)t);
  if (ret){
    exit(-1);
  }


  printf("%d\n", x); 
}
