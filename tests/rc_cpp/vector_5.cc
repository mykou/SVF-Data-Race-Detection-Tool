/*
 * Simple race check
 * Author: dye
 * Date: 13/12/2016
 */
#include "aliascheck.h"
#include "pthread.h"
#include <vector>

using namespace std;


vector<int> g;

void *foo(void *arg) {

    // There is no simple way to use RC_ACCESS to validate container
    // methods derived from templates (e.g., push_back(typename))
    g.push_back(1);

    int x = g.front();
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    g.back() = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

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
