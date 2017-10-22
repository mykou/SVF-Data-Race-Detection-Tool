/*
 * Simple race check
 * Author: dye
 * Date: 27/02/2017
 */
#include "aliascheck.h"
#include "pthread.h"
#include <vector>

using namespace std;


vector<int> g1;
vector<int> g2;


void *foo(void *arg) {

    int x = g1[0];
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    RC_ACCESS(2, RC_ALIAS | RC_MHP);

    g1[0] = 1;
    RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

    g2[0] = 2;
    RC_ACCESS(2, RC_ALIAS | RC_MHP);

}


int main(int argc, char *argv[]) {

    g1.push_back(1);
    g2.push_back(1);

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
