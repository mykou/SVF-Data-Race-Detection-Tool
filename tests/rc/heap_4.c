/*
 * Simple alias check
 * Author: dye
 * Date: 04/08/2015
 */

#include "aliascheck.h"
#include "pthread.h"

int g = 0;
int use_single_thread = 0;


void *foo(void *data) {

    int *p = (int *) malloc(sizeof(int));
    *p = 20;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);

}


int main(int argc, char *argv[]) {
  pthread_t threads[10];
  int ret;
  int t;

  for (int i = 0; i < 10; ++i) {
      pthread_create(&threads[i], NULL, foo, (void *)(&t));
  }

}
