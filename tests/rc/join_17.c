/*
 * Simple race check
 * Author: dye
 * Date: 23/03/2016
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
int P = 10;

void *foo(void *threadid) {
  long tid;
  tid = (long)threadid;

  g = 10; 
  RC_ACCESS(1, RC_ALIAS);
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

}


int main(int argc, char *argv[]) {
  pthread_t thread[10];
  long ret;
  long t;
  for (unsigned long i = 0; i < P - 1; ++i) {
      ret = pthread_create(&thread[i], NULL, foo, (void *)t);
      if (ret != 0){
        exit(-1);
      }
  }
  for (int i = 0; i < P - 1; ++i) {
      ret = pthread_join(thread[i], 0);
      if (ret != 0) {
          exit(-1);
      }
  }

  int x = g;
  RC_ACCESS(1, RC_ALIAS);
  printf("%d\n", x);

}
