/*
 * Simple race check
 * Author: dye
 * Date: 24/02/2017
 */
#include "aliascheck.h"
#include "pthread.h"
#include <memory>

using namespace std;

shared_ptr<int> g;

void *foo(void *arg) {

    *g = 10;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

}


int main(int argc, char *argv[]) {

    g = shared_ptr<int>(new int);

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
