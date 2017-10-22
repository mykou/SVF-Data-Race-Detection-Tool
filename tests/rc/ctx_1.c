/*
 * Simple race check
 * Author: dye
 * Date: 10/02/2016
 */

// A context sensitivity example from goblint_zebedee,
// Stage I of RaceComb will report this false positive,
// Due to its context-insensitive pointer analysis.
// LockSmith does not report this issue.

#include "aliascheck.h"
#include "pthread.h"

int ServerPort = 0;

void *makeListener(int *portP) {
    *portP = 8080;
    RC_ACCESS(1, RC_ALIAS | RC_MHP);

    return NULL;
}


void *client(void *args) {
  int x = ServerPort;
  RC_ACCESS(1, RC_ALIAS | RC_MHP);

  printf("%d\n", x);
  return NULL;
}


// ClientListener() in zebedee.c as main() in this example.
int main(int argc, char *argv[]) {
  pthread_t thread[10];
  int localPort = 0;

  makeListener(&ServerPort);

  for (int i = 0; i < 10; ++i) {
      makeListener(&localPort);
      int ret;
      ret = pthread_create(&thread[i], NULL, client, (void *)0);
      if (ret){
        exit(-1);
      }
  }

}
