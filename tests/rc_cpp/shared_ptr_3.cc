/*
 * Simple race check
 * Author: dye
 * Date: 27/02/2017
 */
#include "aliascheck.h"
#include "pthread.h"
#include <memory>
#include <map>

using namespace std;

//#define USE_SHARED_PTR


class C {
public:
    int x;
    int y;
    map<int, int> m;
};

struct S {

    shared_ptr<C> _p;

#ifdef USE_SHARED_PTR
    shared_ptr<C> p;
#else
    C *p;
#endif

    S() {
        _p = shared_ptr<C>(new C());

#ifdef USE_SHARED_PTR
        p = shared_ptr<C>(new C());
#else
        p = new C();
#endif

    }

    void bar1() {
        p->m.insert(pair<int,int>(0, 0));
    }

    void bar2() {
        p->m[0] = 10;
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
    }
};


void *foo(void *arg) {

    struct S *q = (struct S*)arg;

    q->bar1();

    q->bar2();

}


int main(int argc, char *argv[]) {
    struct S s;

    pthread_t thread[10];
    for (int i = 0; i < 10; ++i) {
        int ret;
        ret = pthread_create(&thread[i], NULL, foo, (void *)&s);
        if (ret){
          exit(-1);
        }
    }

    return 0;
}
