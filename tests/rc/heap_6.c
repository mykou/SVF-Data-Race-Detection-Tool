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
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

}



int main(int argc, char *argv[]) {
  pthread_t threads[10];
  int ret;
  int *data = 0, prev = 0;

  for (int i = 0; i < 10; ++i) {
    data = (int *) malloc(sizeof(int));
    int *p = 0;
    if (argc > 2) {
        p = data;
    } else {
        p = prev;
    }

    *p = 10;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

    void *args = (void*)p;
    pthread_create(&threads[i], NULL, foo, args);

    *p = 10;
    RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

    prev = p;
  }

}
