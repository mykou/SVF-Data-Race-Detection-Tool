/*
 * ResultValidator.cpp
 *
 *  Created on: 26/10/2015
 *      Author: dye
 */

#include "ResultValidator.h"
#include "ThreadEscapeAnalysis.h"
#include "LocksetAnalysis.h"
#include "BarrierAnalysis.h"
#include "ThreadJoinRefinement.h"
#include "PathRefinement.h"
#include "HeapRefinement.h"
#include "ContextSensitiveAliasAnalysis.h"

using namespace llvm;


/*
 * Perform the validation.
 */
void ResultValidator::analyze() {
    // Check if there is any validation target in the input program.
    if (!hasValidationTarget())     return;

    // We recompute partitions here, since some accesses might
    // be pruned in previous analysis.
    mp->run();

    // Build the mapping from Instruction to memory access ids
    vector<MemoryPartitioning::Partition> &parts = mp->getParts();
    for (int i = 0, e = parts.size(); i != e; ++i) {
        const AccessIdVector &accessIds = parts[i].accessIds;
        for (int j = 0, je = accessIds.size(); j != je; ++j) {
            AccessID id = accessIds[j];
            assert(!MemoryPartitioning::isPrunedAccess(id));
            const Instruction *I = MemoryPartitioning::getInstruction(id);
            inst2accessIds[I].push_back(id);
            inst2partIds[I].push_back(i);
        }
    }

    // Reset relevant analysis
    if (csaa) {
        csaa->reset();
    }

    // Call super analyze() function to do the rest of the validation work.
    RCResultValidator::analyze();
}


/*
 * Check if two Instructions may access aliases.
 */
bool ResultValidator::mayAccessAliases(const Instruction *I1,
        const Instruction *I2) {
    for (int i = 0, ie = inst2partIds[I1].size(); i != ie; ++i) {
        PartID partId1 = inst2partIds[I1][i];
        for (int j = 0, je = inst2partIds[I2].size(); j != je; ++j) {
            PartID partId2 = inst2partIds[I2][j];
            if (partId1 == partId2)     return true;
        }
    }
    return false;
}


/*
 * Check if two Instructions may happen in parallel.
 */
bool ResultValidator::mayHappenInParallel(const Instruction *I1,
        const Instruction *I2) {
    MhpAnalysis::InstSet spawnSites = mhp->mayHappenInParallel(I1, I2);
    if (spawnSites.empty())     return false;
    if (joinRefine->branchJoinRefined(I1, I2))  return false;
    if (pathRefine->pathRefined(I1, I2))        return false;
    if (ba->separatedByBarrier(I1, I2))         return false;
    return true;
}


/*
 * Check if two Instructions are properly protected.
 */
bool ResultValidator::protectedByCommonLocks(const Instruction *I1,
        const Instruction *I2) {
    return lsa->protectedByCommonLocks(I1, I2);
}


/*
 * Check if two Instructions may cause a data race.
 */
bool ResultValidator::mayHaveDataRace(const Instruction *I1,
        const Instruction *I2) {
    // Check if they may happen in parallel.
    MhpAnalysis::InstSet mhpSpawnSites = mhp->mayHappenInParallel(I1, I2);
    if (mhpSpawnSites.empty())      return false;
    if (joinRefine->branchJoinRefined(I1, I2))  return false;
    if (pathRefine->pathRefined(I1, I2))        return false;
    if (ba->separatedByBarrier(I1, I2))         return false;

    // Check if they are protected properly
    if (protectedByCommonLocks(I1, I2))     return false;

    // Check if they may access aliases
    for (int i = 0, ie = inst2partIds[I1].size(); i != ie; ++i) {
        int partId1 = inst2partIds[I1][i];
        int accessId1 = inst2accessIds[I1][i];
        int isWrite1 = MemoryPartitioning::isWriteAccess(accessId1);

        for (int j = 0, je = inst2partIds[I2].size(); j != je; ++j) {
            int partId2 = inst2partIds[I2][j];
            int accessId2 = inst2accessIds[I2][j];
            int isWrite2 = MemoryPartitioning::isWriteAccess(accessId2);

            // There should be no race if at least one of the following
            // four conditions is satisfied:
            // (1) Neither of the accesses is a write.
            if (!(isWrite1 || isWrite2))    continue;

            // (2) They access different memory partitions
            if (partId1 != partId2)     continue;

            // (3) The accessed memory object must be thread-local.
            if (!tea->mayEscape(partId1, mhpSpawnSites))    continue;

            // (4) They must access different instances of a heap object.
            if (heapRefined(partId1, I1, I2))   continue;

            // (5) They come from infeasible calling contexts.
            if (csaa && csaa->mustNotAccessAliases(accessId1, accessId2))   continue;

            // There may be a race if non of the above conditions holds.
            return true;
        }
    }

    return false;
}


/*
 * Check if two Instructions access different heap instances.
 * @param partId memory partition id
 * @param I1 memory access instruction
 * @param I2 memory access instruction
 * @return true if (1) partId contains exactly one heap object,
 *         and (2) I1 and I2 must access different instances of the object.
 */
bool ResultValidator::heapRefined(PartID partId, const Instruction *I1,
        const Instruction *I2) {
    const PointsTo &pts = mp->getPartObjs(partId);
    // We only handle the partitions with only one heap object here.
    if (1 != pts.count())   return false;

    NodeID objId = pts.find_first();
    return heapRefine->accessDifferentHeapInstances(objId, I1, I2);
}

