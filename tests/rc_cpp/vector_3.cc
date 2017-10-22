/*
 * Simple race check
 * Author: dye
 * Date: 13/12/2016
 */
#include "aliascheck.h"
#include "pthread.h"
#include <vector>

using namespace std;

class C {
public:
    int f1;
    int f2;
};

vector<C> g;

void *foo(void *arg) {

    for (vector<C>::iterator it = g.begin(), ie = g.end(); it != ie; ++it) {
        C &c = *it;

        c.f1 = 0;
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);

        c.f1 = 1;
        RC_ACCESS(2, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(3, RC_MHP);

        c.f2 = 2;
        RC_ACCESS(3, RC_MHP);
    }
}


int main(int argc, char *argv[]) {

    C c;
    g.push_back(c);

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
