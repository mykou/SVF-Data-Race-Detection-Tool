/*
 * RCStat.cpp
 *
 *  Created on: 29/07/2015
 *      Author: dye
 */

#include "RCStat.h"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace llvm;
using namespace analysisUtil;


/*
 * Dump the statistics
 */
void RCStat::print() const {

    std::cout.flush();

    // format out put with a preserved width
    unsigned field_width = 30;
    std::cout.flags(std::ios::left);
    std::cout << std::fixed << std::setprecision(6);

    std::cout << pasMsg(" --- Analysis Timing ---\n");
    for (TIMEStatMap::const_iterator it = timeStatMap.begin(), eit =
            timeStatMap.end(); it != eit; ++it) {
        auto ns = (it->second.end - it->second.begin).count();
        double s = ns / 1000000000.0;
        std::cout << std::setw(field_width) << it->second.description << s << "\n";
    }

    std::cout << "\n";

    std::cout.flush();

}


