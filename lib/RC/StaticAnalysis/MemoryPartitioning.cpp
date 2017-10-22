/*
 * MemoryPartitioning.cpp
 *
 *  Created on: 22/05/2015
 *      Author: dye
 */

#include "MemoryPartitioning.h"
#include "ThreadEscapeAnalysis.h"
#include "BarrierAnalysis.h"
#include "LocksetAnalysis.h"
#include "ThreadJoinRefinement.h"
#include "PathRefinement.h"
#include "HeapRefinement.h"
#include "ContextSensitiveAliasAnalysis.h"

using namespace analysisUtil;
using namespace llvm;
using namespace std;


/*
 * Perform memory partitioning according to a set of memory accesses.
 */
void MemoryPartitioning::run() {
    // Collect mapping from objects to memory access ids
    PAG *pag = pta->getPAG();
    map<NodeID, bitmap> obj2accessIds;   // obj -> access ids
    for (int i = 0, e = Partition::accesses.size(); i != e; ++i) {
        const Instruction *I = Partition::accesses[i].getInstruction();
        const Value *p = Partition::accesses[i].getPointer();
        bool includeOffsetFields = isCallSite(I);

        NodeID n = pag->getValueNode(p);
        PointsTo &pts = pta->getPts(n);

        for (PointsTo::iterator it = pts.begin(), ie = pts.end(); it != ie;
                ++it) {
            NodeID obj = *it;
            if (rcUtil::isDummyObj(obj, pag))   continue;
            if (rcUtil::isFuncObj(obj, pag))    continue;

            obj2accessIds[obj].set(i);

            if (!includeOffsetFields)   continue;

            if (rcUtil::isFirstFieldObj(obj, pag)) {
                NodeBS &fields = pag->getAllFieldsObjNode(obj);
                for (NodeBS::iterator it = fields.begin(), ie = fields.end();
                        it != ie; ++it) {
                    obj2accessIds[*it].set(i);
                }
            }
        }
    }

    // Make partitions
    unordered_map<bitmap, PartID> eq;  // map relevant partitions to partition Id
    parts.clear();
    parts.push_back(Partition()); // 0 index reserved

    for (map<NodeID, bitmap>::iterator it = obj2accessIds.begin(), ie =
                obj2accessIds.end(); it != ie; ++it) {
        NodeID obj = it->first;
        bitmap &insts = it->second;
        PartID partId = eq[insts];
        if (!partId) {
            parts.push_back(Partition());
            partId = parts.size() - 1;
            eq[insts] = partId;
        }

        // part => objs
        parts[partId].objs.set(obj);

        // part => access ids
        if (parts[partId].accessIds.size())  continue;
        for (bitmap::iterator it = insts.begin(), ie = insts.end(); it != ie; ++it) {
            parts[partId].accessIds.push_back(*it);
        }
    }

    obj2accessIds.clear();
    eq.clear();

    // Compute mapping from objects to partitions
    obj2part.resize(pta->getPAG()->getTotalNodeNum());
    for (int i = 0, e = parts.size(); i < e; ++i) {
        for (PointsTo::iterator it = parts[i].objs.begin(), ie = parts[i].objs.end();
                it != ie; ++it) {
            NodeID obj = *it;
            obj2part[obj] = i;
        }
    }
}


/*
 * Print the memory partitioning information
 */
void MemoryPartitioning::print() const {
    // Compute partition information
    int numPartStoreOnly = 0;
    int numPartLoadOnly = 0;
    int numPartLoadStore = 0;
    int numPartEmpty = 0;

    for (int i = 0, e = parts.size(); i != e; ++i) {
        const AccessIdVector &accessIds = parts[i].accessIds;
        int numWrites = 0;
        int numReads = 0;
        for (int ii = 0, ee = accessIds.size(); ii != ee; ++ii) {
            AccessID id = accessIds[ii];
            if (MemoryPartitioning::isPrunedAccess(id))  continue;
            if (MemoryPartitioning::isWriteAccess(id)) {
                ++numWrites;
            } else {
                ++numReads;
            }
        }

        if (numReads && !numWrites) {
            ++numPartLoadOnly;
        } else if (numWrites && !numReads) {
            ++numPartStoreOnly;
        } else if (numWrites && numReads) {
            ++numPartLoadStore;
        }
    }

    numPartEmpty = getNumParts() - numPartLoadOnly - numPartStoreOnly
            - numPartLoadStore;

    // Print the relevant information
    outs() << pasMsg(" --- Memory Partitioning ---\n");
    outs() << "No. of pag nodes: \t" << pta->getPAG()->getTotalNodeNum() << "\n";
    outs() << "No. of objects: \t" << pta->getPAG()->getObjectNodeNum() << "\n";
    outs() << "No. of partitions in total: \t" << getNumParts() << "\n";
    outs() << " | No. of partitions (R only):\t" << numPartLoadOnly << "\n";
    outs() << " | No. of partitions (W only):\t" << numPartStoreOnly << "\n";
    outs() << " | No. of partitions (R & W):\t" << numPartLoadStore << "\n";
    outs() << " | No. of partitions (pruned):\t" << numPartEmpty << "\n";
    outs() << "\n";
}


/*
 * Print the information of a specific memory partition
 */
void MemoryPartitioning::print(PartID partId) const {
    const Partition &p = parts[partId];

    if (p.objs.empty())     return;

    PAG *pag = pta->getPAG();
    NodeID obj = p.objs.find_first();
    const Value *v = pag->getObject(obj)->getRefVal();
    outs() << "\n--------- " << p.objs.count() << " objs in this part ---------\n";
    outs() << " first obj NodeID is " <<  obj << "\n";
    outs() << "############# " << rcUtil::getSourceLoc(v) << " #############\n";

    const AccessIdVector &accessIds = p.accessIds;
    for (int ii = 0, ee = accessIds.size(); ii != ee; ++ii) {
        AccessID id = accessIds[ii];
        if (MemoryPartitioning::isPrunedAccess(id))  continue;

        const Instruction *I = MemoryPartitioning::getInstruction(id);
        const Value *p = MemoryPartitioning::getPointer(id);
        NodeID pId = pag->getValueNode(p);
        outs() << "------ " << rcUtil::getSourceLoc(I) << "\t";
        outs() << "(accessId: " << id << ")\n";
        outs() << "\tpts:\t";

        PointsTo& pts = pta->getPts(pId);
        for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
            outs() << *it << " ";
        }
        outs() << "\n";
    }
}


// all memory accesses
std::vector<MemoryAccess> MemoryPartitioning::Partition::accesses;


/*
 * Perform memory partitioning and setup RiskyInstructionSet for each memory partition.
 */
void RCMemoryPartitioning::run() {
    // Perform memory partitioning
    MemoryPartitioning::run();

    // Setup RiskyInstructionSet for each memory partition
    riskyInstSets.resize(parts.size());

    // Compute all risky pairs even if out-of-budget.
    // This provides sound paired IR annotation for dynamic analysis.
    setNeedComputeAllRiskyPairs(true);
}


/*
 * Apply ThreadEscapeAnalysis to roughly prune the memory accesses
 * that operate on some thread-local objects.
 */
void RCMemoryPartitioning::applyAnalysis(ThreadEscapeAnalysis *tea) {
    assert(tea && "Thread escape analysis must apply.");

    for (int i = 0, ie = parts.size(); i != ie; ++i) {
        if (tea->mayEscape(i))   continue;
        AccessIdVector &accessIds = parts[i].accessIds;
        for (int j = 0, je = accessIds.size(); j != je; ++j) {
            accessIds[j] = getPrunedAccessId();
        }
    }
}


/*
 * Apply MhpAnalysis to prune the non-risky memory accesses
 * that must not happen in parallel;
 * apply ThreadEscapeAnalysis to precisely prune some
 * thread-local objects;
 * apply ContextSensitiveAliasAnalysis to prune some infeasible
 * alias results.
 */
void RCMemoryPartitioning::applyAnalysis(MhpAnalysis *mhp,
        ThreadEscapeAnalysis *tea, ContextSensitiveAliasAnalysis *csaa) {
    assert(mhp && "MHP analysis must apply.");
    assert(tea && "Thread escape analysis must apply.");

    // Filter the memory accesses which must be sequential.
    for (int i = 0, e = parts.size(); i != e; ++i) {
        AccessIdVector &accessIds = parts[i].accessIds;
        for (int ii = 0, ee = accessIds.size(); ii != ee; ++ii) {
            AccessID id = accessIds[ii];
            if (isPrunedAccess(id))  continue;
            const Instruction *I = getInstruction(id);
            if (mhp->isSequential(I)) {
                accessIds[ii] = getPrunedAccessId();
            }
        }
    }

    // Identify risky memory access pairs using "mhp" and "tea"
    Inst2RmavMap reachable;
    for (int i = 0, e = parts.size(); i != e; ++i) {
        reachable.clear();
        computeReachabilityMapForPartition(i, mhp, tea, csaa, reachable);

        RiskyInstructionSet &riskyInstSet = riskyInstSets[i];
        identifyRiskyInstSetForPartition(i, reachable, riskyInstSet);
    }
}


/*
 * Apply BarrierAnalysis to prune the memory accesses
 * that must not happen in parallel.
 */
void RCMemoryPartitioning::applyAnalysis(BarrierAnalysis *ba) {
    assert(ba && "Barrier analysis must apply.");

    for (int i = 0, ie = parts.size(); i != ie; ++i) {
        RiskyInstructionSet &riskyInstSet = riskyInstSets[i];
        if (riskyInstSet.isOverBudget())    continue;

        // Check if the access pairs are protected by common lock(s)
        RiskyInstructionSet::RiskyPairs &riskyPairs =
                riskyInstSet.getRiskyPairs();
        for (auto it = riskyPairs.begin(), ie = riskyPairs.end(); it != ie;) {
            const Instruction *I1 = getInstruction(it->first);
            const Instruction *I2 = getInstruction(it->second);
            if (ba->separatedByBarrier(I1, I2)) {
                riskyPairs.erase(it++);
            } else {
                ++it;
            }
        }
    }
}


/*
 * Apply LocksetAnalysis to prune the properly protected memory accesses.
 */
void RCMemoryPartitioning::applyAnalysis(LocksetAnalysis *lsa) {
    assert(lsa && "Lockset analysis must apply.");

    for (int i = 0, ie = parts.size(); i != ie; ++i) {
        RiskyInstructionSet &riskyInstSet = riskyInstSets[i];
        if (riskyInstSet.isOverBudget())    continue;

        // Check if the access pairs are protected by common lock(s)
        RiskyInstructionSet::RiskyPairs &riskyPairs =
                riskyInstSet.getRiskyPairs();
        for (auto it = riskyPairs.begin(), ie = riskyPairs.end(); it != ie;) {
            const Instruction *I1 = getInstruction(it->first);
            const Instruction *I2 = getInstruction(it->second);
            if (lsa->protectedByCommonLocks(I1, I2)) {
                riskyPairs.erase(it++);
            } else {
                ++it;
            }
        }
    }
}


/*
 * Apply ThreadJoinRefinement to prune some conservative results
 * of course-grain MhpAnalysis, where join is imprecisely handled;
 * and apply PathRefinement to prune the memory accesses
 * under exclusive conditions.
 */
void RCMemoryPartitioning::applyAnalysis(ThreadJoinRefinement *joinRefinement,
        PathRefinement *pathRefine) {
    assert(joinRefinement && "Heap refinement analysis must apply.");
    assert(pathRefine && "Path refinement analysis must apply.");

    for (int i = 0, ie = parts.size(); i != ie; ++i) {
        RiskyInstructionSet &riskyInstSet = riskyInstSets[i];
        if (riskyInstSet.isOverBudget())    continue;

        // Check if the access pairs are guarded by exclusive conditions.
        RiskyInstructionSet::RiskyPairs &riskyPairs =
                riskyInstSet.getRiskyPairs();
        for (auto it = riskyPairs.begin(), ie = riskyPairs.end(); it != ie;) {
            const Instruction *I1 = getInstruction(it->first);
            const Instruction *I2 = getInstruction(it->second);
            if (joinRefinement->branchJoinRefined(I1, I2)
                    || pathRefine->pathRefined(I1, I2)) {
                riskyPairs.erase(it++);
            } else {
                ++it;
            }
        }
    }
}


/*
 * Apply MhpRefinementAnalysis to prune the memory accesses
 * on different heap instances.
 */
void RCMemoryPartitioning::applyAnalysis(HeapRefinement *heapRefine) {
    assert(heapRefine && "Heap refinement analysis must apply.");

    for (int i = 0, ie = parts.size(); i != ie; ++i) {
        RiskyInstructionSet &riskyInstSet = riskyInstSets[i];
        if (riskyInstSet.isOverBudget())    continue;

        // Check if the access pairs must accessing different heap instances.
        const PointsTo &pts = getPartObjs(i);
        if (1 != pts.count())   continue;

        NodeID objId = pts.find_first();
        RiskyInstructionSet::RiskyPairs &riskyPairs =
                riskyInstSet.getRiskyPairs();
        for (auto it = riskyPairs.begin(), ie = riskyPairs.end(); it != ie;) {
            const Instruction *I1 = getInstruction(it->first);
            const Instruction *I2 = getInstruction(it->second);
            if (heapRefine->accessDifferentHeapInstances(objId, I1, I2)) {
                riskyPairs.erase(it++);
            } else {
                ++it;
            }
        }
    }
}


/*
 * Apply context-sensitive CFL-reachability-based alias analysis to prune
 * infeasible pairs.
 */
void RCMemoryPartitioning::applyAnalysis(ContextSensitiveAliasAnalysis *csaa) {
    assert(csaa && "Context-sensitive alias analysis must apply.");

    for (int i = 0, ie = parts.size(); i != ie; ++i) {
        RiskyInstructionSet &riskyInstSet = riskyInstSets[i];
        if (riskyInstSet.isOverBudget())    continue;

        RiskyInstructionSet::RiskyPairs &riskyPairs =
                riskyInstSet.getRiskyPairs();

        for (auto it = riskyPairs.begin(), ie = riskyPairs.end(); it != ie;) {
            if (csaa->mustNotAccessAliases(it->first, it->second)) {
                riskyPairs.erase(it++);
            } else {
                ++it;
            }
        }
    }

}


/*
 * Prune the non-risky memory accesses according to the
 * RiskyInstructionSet information
 */
void RCMemoryPartitioning::pruneNonRiskyMemoryAccess() {
    SmallSet<AccessID, 16> riskyIds;
    // Iterative every memory partition
    for (int i = 0, e = parts.size(); i != e; ++i) {

        // Skip if the RiskyInstructionSet is over budget
        if (getRiskyInstSet(i).isOverBudget())  continue;

        // Gather the risky memory access ids
        riskyIds.clear();
        getRiskyAccessIds(i, riskyIds);

        // Prune the non-risky memory accesses
        AccessIdVector &accessIds = parts[i].accessIds;
        for (int j = 0, je = accessIds.size(); j != je; ++j) {
            if (riskyIds.count(accessIds[j]))     continue;
            accessIds[j] = getPrunedAccessId();
        }
    }
}


/*
 * Check if a memory access must not access a given memory partition.
 * @param accessId the memory access id
 * @param rp the ReachablePoint of the access
 * @param partId the memory partition id
 * @param csaa the ContextSensitiveAliasAnalysis
 * @return whether the memory access must not read/write the partition
 */
bool RCMemoryPartitioning::mustNotAccessPart(AccessID accessId,
        const MhpAnalysis::ReachablePoint &rp, PartID partId,
        ContextSensitiveAliasAnalysis *csaa) const {
    assert(csaa && "Context-sensitive alias analysis must apply.");
    return csaa->mustNotAccessMemoryPartition(accessId, rp, partId);
}


/*
 * Compute all reachable memory accesses that access a given memory
 * partition for every spawn site.
 * @param partId the input memory partition id
 * @param mhp the input MhpAnalysis
 * @param tea the input ThreadEscapeAnalysis
 * @param csaa the input ContextSensitiveAliasAnalysis
 * @param reachable the output reachability information
 */
void RCMemoryPartitioning::computeReachabilityMapForPartition(PartID partId,
        MhpAnalysis *mhp, ThreadEscapeAnalysis *tea,
        ContextSensitiveAliasAnalysis *csaa, Inst2RmavMap &reachable) const {

    // Get the spawn sites of partId
    SmallSet<const Instruction*, 8> vislbleSpawnSites;
    tea->getVisibleSpawnSites(partId, vislbleSpawnSites);

    // Iterate each memory access to compute reachability information
    const AccessIdVector &accessIds = parts[partId].accessIds;
    for (AccessIdVector::const_iterator it = accessIds.begin(),
            ie = accessIds.end(); it != ie; ++it) {
        AccessID id = *it;
        if (isPrunedAccess(id))  continue;

        const Instruction *I = getInstruction(id);
        MhpAnalysis::BackwardReachablePoints &brp =
                mhp->getBackwardReachablePoints(I);
        for (MhpAnalysis::BackwardReachablePoints::const_iterator it =
                brp.begin(), ie = brp.end(); it != ie; ++it) {
            const MhpAnalysis::ReachablePoint &rp = *it;
            const Instruction *spawnSite = rp.getSpawnSite();

            // Skip it if the memory partition is not visible from spawnSite
            if (!vislbleSpawnSites.count(spawnSite))        continue;

            // Skip it if the access must not access the memory partition.
            if (csaa && mustNotAccessPart(id, rp, partId, csaa))    continue;

            MhpAnalysis::ReachableType t = rp.getReachableType();
            reachable[spawnSite].push_back(ReachableMemoryAccess(id, t));
        }
    }
}


/*
 * Apply reachability information to identify risky memory access pairs
 * of a memory partition.
 * @param partId the input memory partition id
 * @param reachable the input reachability information
 * @param riskyInstSet the output risky access pairs
 */
void RCMemoryPartitioning::identifyRiskyInstSetForPartition(PartID partId,
        Inst2RmavMap &reachable, RiskyInstructionSet &riskyInstSet) const {
    // Iterate every spawn site.
    for (auto it = reachable.begin(), ie = reachable.end(); it != ie; ++it) {
        // Stop if we are running out of budget.
        if (!needComputeAllRiskyPairs()) {
            if (riskyInstSet.isOverBudget())    break;
        }

        // Analyze the memory accesses.
        AccessIdVector trunkWrites;
        AccessIdVector trunkReads;
        AccessIdVector branchWrites;
        AccessIdVector branchReads;

        const ReachableMemoryAccessVector &rmav = it->second;
        for (int i = 0, e = rmav.size(); i != e; ++i) {
            AccessID id = rmav[i].id;
            MhpAnalysis::ReachableType t = rmav[i].type;
            bool isWrite = isWriteAccess(id);
            switch (t) {
            case MhpAnalysis::REACHABLE_TRUNK:
            {
                if (isWrite)
                    trunkWrites.push_back(id);
                else
                    trunkReads.push_back(id);
                break;
            }
            case MhpAnalysis::REACHABLE_BRANCH:
            {
                if (isWrite)
                    branchWrites.push_back(id);
                else
                    branchReads.push_back(id);
                break;
            }
            case MhpAnalysis::REACHABLE_NOT:
                break;
            }
        }

        // Add all risky memory access pairs to
        riskyInstSet.addPairsFrom(trunkWrites, branchWrites);
        riskyInstSet.addPairsFrom(trunkWrites, branchReads);
        riskyInstSet.addPairsFrom(trunkReads, branchWrites);
    }
}


/*
 * Record the Instruction pairs as the combinations from ids1 and ids2.
 */
void RCMemoryPartitioning::RiskyInstructionSet::addPairsFrom(
        const AccessIdVector &ids1, const AccessIdVector &ids2) {
    // Skip if the remaining budget cannot fit the new pairs
    if (!needComputeAllRiskyPairs()) {
        int numNewPairs = ids1.size() * ids2.size();
        if (!hasBudget(numNewPairs))    return;
    }

    // Populate riskyPairs with the new pairs
    for (int i = 0, e = ids1.size(); i != e; ++i) {
        for (int ii = 0, ee = ids2.size(); ii != ee; ++ii) {
            if (ids1[i] < ids2[ii])
                riskyPairs.insert(make_pair(ids1[i], ids2[ii]));
            else
                riskyPairs.insert(make_pair(ids2[ii], ids1[i]));
        }
    }
}

bool RCMemoryPartitioning::RiskyInstructionSet::computeAllRiskyPairs;
