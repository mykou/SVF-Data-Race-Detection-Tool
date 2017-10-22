/*
 * ThreadEscapeAnalysis.cpp
 *
 *  Created on: 30/10/2015
 *      Author: dye
 */

#include "ThreadEscapeAnalysis.h"
#include "MemoryPartitioning.h"
#include "Util/ThreadCallGraph.h"


using namespace llvm;
using namespace std;


/*
 * Initialization
 */
void ThreadEscapeAnalysis::init(ThreadCallGraph *tcg, PointerAnalysis *pta,
        MemoryPartitioning *mp = 0) {
    this->tcg = tcg;
    this->pta = pta;
    this->mp = mp;
}


/*
 * Perform thread escape analysis
 */
void ThreadEscapeAnalysis::analyze() {

    collectGlobalVisibleObjs();

    collectSpawnSiteVisibleObjs();

}


/*
 * Check if the object pointed to by a given pointer is visible in
 * a specific set of memory partitions.
 * @param p the input pointer
 * @param visibleParts the input visible partitions
 * @return the output boolean value
 */
bool ThreadEscapeAnalysis::isVisible(const Value *p,
        PointsTo &visibleParts) const {
    PAG *pag = pta->getPAG();
    PointsTo &pts = pta->getPts(pag->getObjectNode(p));
    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
        NodeID obj = *it;
        NodeID baseObj = pag->getBaseObjNode(obj);
        if (baseObj == obj || rcUtil::isFirstFieldObj(obj, pag)) {
            NodeBS &fields = pag->getAllFieldsObjNode(baseObj);
            for (NodeBS::iterator it = fields.begin(), ie = fields.end();
                    it != ie; ++it) {
                NodeID obj = *it;
                if (visibleParts.test(mp->getPartId(obj)))  return true;
            }
        } else {
            if (visibleParts.test(mp->getPartId(obj)))  return true;
        }
    }
    return false;
}


/*
 * Compute the iterative points-to set of a given pointer.
 * The result contains all the memory objects that are visible from
 * the given pointer (i.e., those can be directly or indirectly
 * accessed via the pointer).
 * @param val the input pointer.
 * @param ret the output conjuncted points-to set.
 */
void ThreadEscapeAnalysis::getIterativePtsForAllFields(const Value *val,
        PointsTo &ret) const {
    PAG *pag = pta->getPAG();
    NodeID root = pag->getValueNode(val);
    stack<NodeID> worklist;
    worklist.push(root);

    while (!worklist.empty()) {
        NodeID n = worklist.top();
        worklist.pop();

        NodeID r = n;
        if (mp) r = mp->getPartId(r);

        // Skip visited node
        if (ret.test(r))    continue;
        ret.set(r);

        PointsTo &pts = pta->getPts(n);
        for (PointsTo::iterator it = pts.begin(), ie = pts.end(); it != ie;
                ++it) {
            NodeID m = *it;
            NodeID baseObj = pag->getBaseObjNode(m);
            if (baseObj == m || rcUtil::isFirstFieldObj(m, pag)) {
                NodeBS &fields = pag->getAllFieldsObjNode(baseObj);
                for (NodeBS::iterator it = fields.begin(), ie = fields.end();
                        it != ie; ++it) {
                    worklist.push(*it);
                }
            } else {
                worklist.push(m);
            }
        }
    }

    // Remove the root node, since it is a val node (not an obj node)
    NodeID r = root;
    if (mp) r = mp->getPartId(r);
    ret.reset(r);
}


/*
 * Collect visible objects from global values
 */
void ThreadEscapeAnalysis::collectGlobalVisibleObjs() {
    Module *M = pta->getModule();
    for (Module::global_iterator it = M->global_begin(), ie = M->global_end();
            it != ie; ++it) {
        const Value *v = &*it;
        assert(!isa<Function>(v));
        getIterativePtsForAllFields(v, globalVisible);
    }
}


/*
 * Collect visible objects from the arguments of spawn sites
 */
void ThreadEscapeAnalysis::collectSpawnSiteVisibleObjs() {
    // Fork sites
    ThreadCallGraph::CallSiteSet::iterator it = tcg->forksitesBegin(),
            ie = tcg->forksitesEnd();
    for (; it != ie; ++it) {
        const Instruction *spawnSite = *it;
        const Value *arg = ThreadAPI::getThreadAPI()->getActualParmAtForkSite(spawnSite);
        getIterativePtsForAllFields(arg, spawnSiteVisibleMap[spawnSite]);
        allSpawnSiteVisible |= spawnSiteVisibleMap[spawnSite];
    }

    // parallel_for sites
    it = tcg->parForSitesBegin();
    ie = tcg->parForSitesEnd();
    for (; it != ie; ++it) {
        const Instruction *parForSite = *it;
        const Value *arg = ThreadAPI::getThreadAPI()->getTaskDataAtHareParForSite(parForSite);
        getIterativePtsForAllFields(arg, spawnSiteVisibleMap[parForSite]);
        allSpawnSiteVisible |= spawnSiteVisibleMap[parForSite];
    }
}

