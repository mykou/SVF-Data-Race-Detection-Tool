/*
 * Simple alias check
 * Author: dye
 * Date: 21/12/2016
 */
#include "aliascheck.h"
#include <map>

using namespace std;

class C {
public:
    int f1;
    int f2;
};

map<int, C*> g;


int main(int argc, char *argv[]) {

    C c;

    g[0] = &c;

    map<int, C*>::iterator iter = g.find(0);

    C &c_ = *iter->second;

    int *p = &c.f1;
    int *q = &c.f2;
    int *p_ = &c_.f1;
    int *q_ = &c_.f2;

    NOALIAS(p, q);
    NOALIAS(p_, q_);
    MAYALIAS(p, p_);
    MAYALIAS(q, q_);

    return 0;
}
