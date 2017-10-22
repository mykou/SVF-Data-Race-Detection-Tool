/*
 * Simple alias check
 * Author: dye
 * Date: 13/08/2015
 */
#include "aliascheck.h"
#include "pthread.h"

/*
 * Example from goblint_zebedee: zebedee.c.
 */

typedef struct {
    int f1;
    int f2;
} S;

int inLine;



void *handler(void *args) {
  S *p = (S*)args;

  p->f1 = 0;
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  RC_ACCESS(1, RC_ALIAS | RC_MHP);


}


void spawnHandler() {
    S *p = (S*)malloc(sizeof(S));

    if (inLine) {
        handler(p);
        return;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, handler, p);

}


int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;

  inLine = (argc == 5);

  for (int i = 0; i < argc; ++i)
      spawnHandler();

}
