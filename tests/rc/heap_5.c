/*
 * Simple alias check
 * Author: dye
 * Date: 04/08/2015
 */

#include "aliascheck.h"
#include "pthread.h"

int g = 0;
int use_single_thread = 0;


void *foo(void *args) {

    int *p = (int*)args;
    *p = 20;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(2, RC_ALIAS | RC_MHP);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

}

int main(int argc, char *argv[]) {
  pthread_t threads[10];
  int ret;
  int *data = 0;

  for (int i = 0; i < 10; ++i) {
    data = (int *) malloc(sizeof(int));
    if (data == NULL) {
        continue;
    }

    if (argc > 2) {
        *data = 10;
        RC_ACCESS(2, RC_ALIAS | RC_MHP);
    }

    void *args = (void*)data;
    pthread_create(&threads[i], NULL, foo, args);

    *data = 10;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
  }

}
