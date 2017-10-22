/*
 * Simple race check
 * Author: dye
 * Date: 14/12/2016
 */
#include "aliascheck.h"
#include "pthread.h"
#include <set>

using namespace std;


set<int*> g;

void *foo(void *arg) {

    int *p = (int*)malloc(sizeof(int));

    // There is no simple way to use RC_ACCESS to validate container
    // methods derived from templates (e.g., insert(typename))
    g.insert(p);

    for (set<int*>::iterator it = g.begin(), ie = g.end(); it != ie; ++it) {
        int *q = *it;

        int x = *q;
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

        *q = 1;
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    }

    return NULL;
}


int main(int argc, char *argv[]) {

    pthread_t thread[10];
    for (int i = 0; i < 10; ++i) {
        int ret;
        ret = pthread_create(&thread[i], NULL, foo, (void *)0);
        if (ret){
          exit(-1);
        }
    }

    return 0;
}
