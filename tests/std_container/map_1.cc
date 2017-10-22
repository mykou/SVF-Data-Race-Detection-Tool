/*
 * Simple alias check
 * Author: dye
 * Date: 20/12/2016
 */
#include "aliascheck.h"
#include <map>

using namespace std;

map<int, int> g;


int main(int argc, char *argv[]) {

    int *p = &g[0];

    map<int, int>::iterator iter = g.find(0);
    int *q = &iter->second;

    MAYALIAS(p, q);

    return 0;
}
