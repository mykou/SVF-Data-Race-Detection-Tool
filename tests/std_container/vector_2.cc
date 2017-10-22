/*
 * Simple alias check
 * Author: dye
 * Date: 22/12/2016
 */
#include "aliascheck.h"
#include <vector>

using namespace std;

class C {
public:
    int f1;
    int f2;
};

vector<C*> g;


int main(int argc, char *argv[]) {

    C c;

    g.push_back(&c);

    C &c_ = *g[0];

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
