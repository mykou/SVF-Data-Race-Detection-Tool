/*
 * Reorder.cpp
 *
 *  Created on: May 14, 2016
 *      Author: Peng Di
 */

#include "RC/Reorder.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>

/*
 * OrderPair is used to record pairs with order weight.
 */
class OrderPair {
private:
    size_t a, b, weight;
public:
    /*
     * Constructor
     */
    OrderPair(size_t _a, size_t _b): a(_a), b(_b) {
        weight = 0;
    }

    /*
     * Get the first instruction ID in the pair
     */
    size_t getA() {
        return a;
    }

    /*
     * Get the second instruction ID in the pair
     */
    size_t getB() {
        return b;
    }

    /*
     * Set the weight of the pair
     */
    void setWeight(size_t _weight) {
        weight = _weight;
    }

    /*
     * Overloading operator <
     */
    bool operator <(const OrderPair &m) const {
        if (weight == m.weight)
            if (a == m.a)
                return b < m.b;
            else
                return a < m.a;
        else
            return weight < m.weight;
    }
};

/*
 * Run reorder
 */
bool Reorder::run() {

    std::vector<OrderPair> pairs;
    std::map<size_t, size_t> times;
    FILE *pfile;
    if ((pfile = fopen("DCI.pairs", "r")) == NULL) {
        if ((pfile = fopen("../DCI.pairs", "r")) == NULL) {
            printf("Can't open DCI.pairs\n");
        }
    }

    if (pfile != NULL) {
        unsigned a, b;
        while (!feof(pfile)) {
            if (EOF == fscanf(pfile, "%u %u\n", &a, &b)) {
                printf("Error reading DCI.pairs\n");
            }

            OrderPair *np = new OrderPair(a,b);
            pairs.push_back(*np);
            if (times.find(a) == times.end()) {
                times.insert(std::make_pair<size_t,size_t>(a,1));
            } else {
                times[a]++;
            }
            if (times.find(b) == times.end()) {
                times.insert(std::make_pair<size_t,size_t>(b,1));
            } else {
                times[b]++;
            }
        }
        fclose(pfile);
    }

    for (std::vector<OrderPair>::iterator it = pairs.begin(), ei = pairs.end(); it!=ei; it++) {
        it->setWeight( times[it->getA()] + times[it->getB()]);
    }

    sort(pairs.begin(), pairs.end());

    pfile = fopen("Ordered.pairs", "wb");
    if (pfile == NULL) {
        printf ("Failed to open and create Ordered.pairs\n");
        return EXIT_FAILURE;
    }

    for (std::vector<OrderPair>::iterator it = pairs.begin(), ei = pairs.end(); it != ei; it++) {
        fprintf(pfile, "%zu %zu\n", it->getA(), it->getB());
    }

    fclose(pfile);

    return true;
}
