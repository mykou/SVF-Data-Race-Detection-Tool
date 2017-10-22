/*
 * ThreadJoinRefinement.cpp
 *
 *  Created on: 03/02/2016
 *      Author: dye
 */


#include "ThreadJoinRefinement.h"
#include "MhpAnalysis.h"

using namespace llvm;


/*
 * Initialization
 */
void ThreadJoinRefinement::init(MhpAnalysis *mhp) {
    this->mhp = mhp;

    mhp->performRefinementAnalysis();
}


/*
 * Check if two Instructions cannot happen in parallel due to branch join.
 */
bool ThreadJoinRefinement::branchJoinRefined(const Instruction *I1,
        const Instruction *I2) const {
    // This is not going to happen when I1 and I2 are the same Instruction.
    if (I1 == I2)   return false;

    bool mayNotBeExclusive = false;
    MhpAnalysis::BackwardReachablePoints &brp1 = mhp->getBackwardReachablePoints(I1);
    MhpAnalysis::BackwardReachablePoints &brp2 = mhp->getBackwardReachablePoints(I2);
    for (MhpAnalysis::BackwardReachablePoints::const_iterator it =
            brp1.begin(), ie = brp1.end(); it != ie; ++it) {
        const MhpAnalysis::ReachablePoint &rp1 = *it;
        const Instruction *spawnSite = rp1.getSpawnSite();
        for (MhpAnalysis::BackwardReachablePoints::const_iterator it =
                brp2.begin(), ie = brp2.end(); it != ie; ++it) {
            const MhpAnalysis::ReachablePoint &rp2 = *it;
            if (spawnSite != rp2.getSpawnSite())    continue;
            if (rp1.getReachableType() == rp2.getReachableType())   continue;
            if (mhp->branchJoinRefined(spawnSite, I1, I2)) {
                continue;
            }

            // If there is no conflict, then these two accesses must be risky.
            mayNotBeExclusive = true;
            break;
        }
    }

    return !mayNotBeExclusive;
}


