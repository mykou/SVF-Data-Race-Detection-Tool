/*
 * Simple alias check
 * Author: dye
 * Date: 21/12/2016
 */
#include "aliascheck.h"
#include <map>

using namespace std;

map<int, int*> g;


int main(int argc, char *argv[]) {

    int n;

    g[0] = &n;

    for (map<int, int*>::iterator it = g.begin(), ie = g.end();
            it != ie; ++it) {
        int *q = it->second;
        int *p = &n;

        MAYALIAS(p, q);
    }

    return 0;
}
