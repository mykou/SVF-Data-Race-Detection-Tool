/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int *g = 0;

void *foo(void *arg) {
  int *p = (int*)arg;
  *p = 10;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
  *g = 10;
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

  return 0;
}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;

  int *p = malloc(sizeof(int));
  g = malloc(sizeof(int));

  ret = pthread_create(&thread, NULL, foo, (void *)p);
  if (ret){
    exit(-1);
  }

  int x = *p;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
  int y = *g;
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

  printf("%d\n", x); 
}
