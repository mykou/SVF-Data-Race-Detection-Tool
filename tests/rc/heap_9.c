/*
 * Simple race check
 * Author: dye
 * Date: 22/04/2016
 */
#include "aliascheck.h"
#include "pthread.h"

typedef struct {
    int f1;
    int f2;
} S;

int inLine;

void *bar(void *args) {
  S *p = (S*)args;

  p->f1 = 0;
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

}


void foo() {
    S *p = (S*)malloc(sizeof(S));

    bar(p);

    pthread_t tid;
    pthread_create(&tid, NULL, bar, p);

    p->f1 = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

}


int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;

  foo();
  foo();

}
