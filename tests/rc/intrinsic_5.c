/*
 * Simple alias check
 * Author: dye
 * Date: 13/08/2015
 */
#include "aliascheck.h"
#include "pthread.h"

typedef struct {
    int f1;
    int f2;
} S;


void *handler(void *args) {
  S *p = (S*)args;

  p->f1 = 0;
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  RC_ACCESS(2, RC_ALIAS | RC_MHP);
  RC_ACCESS(3, RC_MHP);


}



void spawnHandler(S *p) {
    S *q = malloc(sizeof(S));
    memcpy(q, p, sizeof(S));
    RC_ACCESS(2, RC_ALIAS | RC_MHP);

    pthread_t tid;
    pthread_create(&tid, NULL, handler, q);
}


void foo() {
    S s;
    s.f1 = 1;
    RC_ACCESS(3, RC_MHP);

    s.f2 = 2;

    spawnHandler(&s);
}


int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;

  foo();
  foo();

}
