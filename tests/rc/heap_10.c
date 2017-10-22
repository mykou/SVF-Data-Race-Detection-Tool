/*
 * Simple alias check
 * Author: dye
 * Date: 11/09/2015
 */
#include "aliascheck.h"
#include "pthread.h"

/*
 * Example from goblint_ptester: ptester.c.
 */

typedef struct {
    int fd;
    int rbytes;
} rtinfo;



void *read_thread(void *args) {
  rtinfo *rip = (rtinfo*)args;

  int x = rip->fd;
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  printf("%d", x);

  rip->rbytes += 10;
  RC_ACCESS(2, RC_ALIAS | RC_MHP);
  RC_ACCESS(3, RC_ALIAS | RC_MHP);
  RC_ACCESS(4, RC_ALIAS | RC_MHP);
  RC_ACCESS(4, RC_ALIAS | RC_MHP);

}


void *test_thread(void *args) {
    rtinfo *rip = (rtinfo*)malloc(sizeof(rtinfo));
    rip->fd = 0;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);
    RC_ACCESS(5, RC_ALIAS | RC_MHP);
    RC_ACCESS(5, RC_ALIAS | RC_MHP);

    rip->rbytes = 0;
    RC_ACCESS(2, RC_ALIAS | RC_MHP);

    pthread_t tid;
    pthread_create(&tid, NULL, read_thread, rip);

    pthread_join(tid, 0);

    int x = rip->rbytes;
    RC_ACCESS(3, RC_ALIAS | RC_MHP);
    printf("%d", x);

}


int main(int argc, char *argv[]) {
  pthread_t tid[10];
  int ret;

  for (int i = 0; i < argc; ++i)
      pthread_create(&tid[i], NULL, test_thread, NULL);

}
