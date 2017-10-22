/*
 * Simple race check
 * Author: dye
 * Date: 02/03/2015
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;
pthread_mutex_t l;




void *foo(void *arg) {
  long x = (long)arg;

  //////////////// Phase I: weak condition on lock
  if (x == 1 || g > 10) {
      pthread_mutex_lock(&l);
  }

  g = 10; 
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

  if (x == 1 && g > 10) {
      g = 5;
      RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_PROTECTED);
      RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_PROTECTED);
  }

  pthread_mutex_unlock(&l);


  //////////////  Phase II: strong condition on lock
  if (x == 1 && g > 10) {
      pthread_mutex_lock(&l);
  }

  g = 10;
  RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);
  RC_ACCESS(3, RC_ALIAS | RC_MHP | RC_RACE);

  if (x == 1 || g > 10) {
      g = 5;
      RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
      RC_ACCESS(4, RC_ALIAS | RC_MHP | RC_RACE);
  }

  pthread_mutex_unlock(&l);


  return NULL;
}



int main(int argc, char *argv[]) {
  pthread_t thread[10];
  int ret;
  long t = 1;

  for (int i = 0; i < 10; ++i) {
      ret = pthread_create(&thread[i], NULL, foo, (void *)t);
      if (ret){
        exit(-1);
      }
  }

  return 0;
}
