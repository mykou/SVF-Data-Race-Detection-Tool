/*
 * Simple alias check
 * Author: dye
 * Date: 26/05/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int *g = 0;

void *foo(void *arg) {
  *g = 10;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

  return 0;
}

int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;

  g = malloc(sizeof(int));

  ret = pthread_create(&thread, NULL, foo, (void *)0);
  if (ret){
    exit(-1);
  }

  int x = *g;
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

  printf("%d\n", x); 
}
