/*
 * HeapRefinement.cpp
 *
 *  Created on: 04/02/2016
 *      Author: dye
 */

#include "HeapRefinement.h"
#include "ThreadEscapeAnalysis.h"
#include "LocksetAnalysis.h"
#include "MemoryPartitioning.h"
#include "Util/ThreadCallGraph.h"

using namespace llvm;
using namespace std;
using namespace rcUtil;


/*
 * Dump the destinations
 */
void HeapRefinement::HeapFlowDestinations::print() const {
    outs() << " --- HeapFlowDestinations ---\n";

    // Print the unique spawn site using the pointer that exclusively flows from
    // the heap address.
    outs() << "\tUnique spawn site:\t"
            << rcUtil::getSourceLoc(uniqueSpawnSite) << "\n";

    // Print the memory accesses dereferencing the pointer that exclusively
    // flows from the heap address.
    outs() << "\tMemory Accesses:\t";
    for (auto it = memoryAccessInsts.begin(), ie = memoryAccessInsts.end();
            it != ie; ++it) {
        outs() << rcUtil::getSourceLoc(*it) << "  ";
    }
    outs() << "\n";

    // Print the non-spawning call sites using the pointer that exclusively
    // flows from the heap address.
    outs() << "\tNon-spawning call sites:\t";
    for (auto it = csInsts.begin(), ie = csInsts.end(); it != ie; ++it) {
        outs() << rcUtil::getSourceLoc(*it) << "  ";
    }
    outs() << "\n";
}


/*
 * Initialization
 */
void HeapRefinement::init(MhpAnalysis *mhp, LocksetAnalysis *lsa,
        ThreadEscapeAnalysis *tea) {
    this->mhp = mhp;
    this->lsa = lsa;
    this->tea = tea;
    this->pag = mhp->getPTA()->getPAG();
}


/*
 * Check if two MHP Instructions access different instances of a given heap object.
 * @param objId the id of a given memory object
 * @param I1 memory access Instruction
 * @param I2 memory access Instruction
 * @return true if (1) objId is a heap object, and (2) I1 and I2 must access
 *         different instances of this heap object.
 */
bool HeapRefinement::accessDifferentHeapInstances(NodeID objId,
        const Instruction *I1, const Instruction *I2) {
    // Get the heap allocation site of the object.
    const Instruction *mallocSite = getHeapAllocationSite(objId);
    if (!mallocSite)    return false;

    // Get the interested destinations where the allocated heap object
    // flows to within the loop.
    const HeapFlowDestinations &flowsDsts = getFlowDestinationsInScope(mallocSite);
    const Instruction *spawnSite = flowsDsts.uniqueSpawnSite;
    if (!spawnSite)     return false;

    const Function *F = mallocSite->getParent()->getParent();
    assert(F == spawnSite->getParent()->getParent());

    // Examine the reachability from the spawn site
    bool branchReachableI1 = mhp->isBranchReachable(spawnSite, I1);
    bool branchReachableI2 = mhp->isBranchReachable(spawnSite, I2);

    // The MHP of I1 and I2 may not be caused by "spawnSite" but some other spawn site.
    if (!branchReachableI1 && !branchReachableI2)   return false;

    // Get forward reachable Instructions and BasicBlocks of the spawn site
    // (in the same loop iteration if spawnSite is in a loop).
    Loop *L = getLi(F)->getLoopFor(spawnSite->getParent());
    InstSet reachableInsts;
    BbSet reachableBbs;
    if (L) {
        getReachableCodeInSameIteration(spawnSite, L, reachableInsts, reachableBbs);
    } else {
        getReachableCode(spawnSite, reachableInsts, reachableBbs);
    }

    // Check if the trunk-only reachable Instruction happens before or after
    // the spawn site in the same loop iteration.
    const Instruction *trunkReachableInst = branchReachableI1 ? I2 : I1;

    // Collect Instructions from flowsDsts that is trunkReachableInst itself
    // or the call sites that have side effect of trunkReachableInst
    InstSet accessInsts;
    if (flowsDsts.memoryAccessInsts.count(trunkReachableInst)) {
        accessInsts.insert(trunkReachableInst);
    }
    getSideEffectCallSites(flowsDsts, trunkReachableInst, accessInsts);

    // If there is no Instruction collected, I1 and I2 must access different instances.
    if (accessInsts.empty())    return true;

    // If any of the collected Instruction is reachable from spawnSite, return false.
    for (auto it = accessInsts.begin(), ie = accessInsts.end(); it != ie; ++it) {
        const Instruction *I = *it;
        if (reachableInsts.count(I))   return false;
        if (reachableBbs.count(I->getParent()))    return false;
    }

    return true;
}


/*
 * Get the HeapFlowDestinations of a given heap object.
 */
const HeapRefinement::HeapFlowDestinations &HeapRefinement::getFlowDestinationsInScope(
        const Instruction *mallocSite) {
    assert(mallocSite);

    // Check if we have already computed this
    HeapFlowMap::const_iterator it = heapFlowMap.find(mallocSite);
    if (it != heapFlowMap.end())    return it->second;

    // Compute the needed info
    computeReachableTargetInScope(mallocSite);
    return heapFlowMap[mallocSite];
}


/*
 * Get the call sites, whose callee has side effect of a specific
 * memory access Instruction, from a given HeapFlowDestinations.
 * @param flowDsts the input HeapFlowDestinations
 * @param I the input memory access Instruction
 * @param csInsts the output call site Instructions
 */
void HeapRefinement::getSideEffectCallSites(
        const HeapFlowDestinations &flowDsts, const Instruction *I,
        InstSet &csInsts) {
    // Check if I is present in the callees of any flowDst.
    const Function *F = I->getParent()->getParent();
    const PTACallGraph *cg = lsa->getCallGraph();
    SmallSet<const Function*, 4> callees;
    for (auto it = flowDsts.csInsts.begin(), ie = flowDsts.csInsts.end();
            it != ie; ++it) {
        const Instruction *csInst = *it;
        callees.clear();
        rcUtil::getCallees(cg, csInst, callees);
        for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
            const Function *callee = *it;
            if (callee->isDeclaration())    continue;

            LocksetSummary *summary = lsa->getSccSummary(callee);
            if (summary && summary->isReachable(F)) {
                csInsts.insert(csInst);
            }
        }
    }
}


/*
 * Compute the HeapFlowDestinations of a given heap object.
 * The destinations are:
 *   (1) in the same loop of mallocSite if mallocSite is in a loop;
 *   (2) in the same function of mallocSite if mallocSite is not in a loop.
 * The exclusiveness guarantees the destinations use the desired instance
 * of this heap object.
 */
void HeapRefinement::computeReachableTargetInScope(
        const Instruction *mallocSite) {
    HeapFlowDestinations &flowDsts = heapFlowMap[mallocSite];
    bool globalVisible = tea->isGlobalVisible(mallocSite);

    // Get the loop information
    const BasicBlock *mallocBb = mallocSite->getParent();
    const Function *F = mallocBb->getParent();
    const LoopInfo *LI = FunctionPassPool::getLi(F);
    const Loop *mallocLoop = LI->getLoopFor(mallocBb);
//    if (!mallocLoop)    return;

    // Perform reachability analysis
    stack<const Value*> worklist;
    worklist.push(mallocSite);
    SmallSet<const Value*, 16> visited;
    SmallVector<const Instruction*, 16> spawnSites;

    while (!worklist.empty()) {
        const Value *V = worklist.top();
        worklist.pop();
        if (visited.count(V))   continue;
        visited.insert(V);

        for (Value::const_use_iterator it = V->use_begin(), ie = V->use_end();
                it != ie; ++it) {
            const Use &U = *it;
            const Value *user = U.getUser();
            if (visited.count(user))     continue;

            const Instruction *I = dyn_cast<Instruction>(user);
            if (I) {
                Loop *userInstLoop = LI->getLoopFor(I->getParent());
                if (const StoreInst *SI = dyn_cast<StoreInst>(I)) {
                    // Check if the value flows to a memory object
                    if (SI->getValueOperand() == I)     return;
                    // Check if the value is accesses by a StoreInst
                    if (SI->getPointerOperand() == V
                            && mallocLoop == userInstLoop
                            && exclusivelyFrom(mallocSite, V)) {
                        flowDsts.memoryAccessInsts.insert(I);
                    }
                    continue;
                }
                // Check if the value is accesses by a LoadInst
                else if (const LoadInst *LI = dyn_cast<LoadInst>(I)) {
                    if (LI->getPointerOperand() == V
                            && mallocLoop == userInstLoop
                            && exclusivelyFrom(mallocSite, V)) {
                        flowDsts.memoryAccessInsts.insert(I);
                    }
                    continue;
                }
                // Check if the value flows to a return site
                else if (isa<ReturnInst>(I)) {
                    return;
                }
                // Check if the value flows to a call site
                else if (analysisUtil::isCallSite(I)) {
                    // Check if the value flows to a memory access intrinsic call
                    if (rcUtil::isMemoryAccess(I)) {
                        // TODO: add an exclusivelyFlowsFrom(from, to) check here.
                        flowDsts.memoryAccessInsts.insert(I);
                    }
                    // Check if the value flows to a spawn site
                    else if (analysisUtil::isThreadForkCall(I)) {
                        const Value *args =
                                ThreadAPI::getThreadAPI()->getActualParmAtForkSite(I);
                        if (mallocLoop == userInstLoop
                                && exclusivelyFrom(mallocSite, args)) {
                            spawnSites.push_back(I);
                        }
                    }
                    // Normal call site
                    else {
                        if (globalVisible)      return;
                        flowDsts.csInsts.insert(I);
                    }
                    continue;
                }
            }
            worklist.push(user);
        }
    }

    // Check if there is only one spawn site that the value flows to
    if (spawnSites.size() == 1) {
        const Instruction *spawnSite = spawnSites.front();
        flowDsts.uniqueSpawnSite = spawnSite;

    } else {
        flowDsts.uniqueSpawnSite = NULL;
    }
}


/*
 * Get reachable Instructions and BasicBlocks from a given Instruction
 * in the same loop.
 * @param root the input root Instruction to start
 * @param L the input Loop where the analysis is scoped
 * @param reachableInsts the output reachable Instruction set
 * @param reachableBbs the output reachable BasicBlock set
 */
void HeapRefinement::getReachableCodeInSameIteration(const Instruction *root,
        Loop *L, InstSet &reachableInsts, BbSet &reachableBbs) {
    // Collect the Instructions following root in the same BasicBlock.
    const Instruction *I = root->getNextNode();
    while (!I->isTerminator()) {
        reachableInsts.insert(I);
        I = I->getNextNode();
    }

    // Collect the reachable BasicBlocks within the same loop.
    const BasicBlock *H = L->getHeader();
    const BasicBlock *rootBB = root->getParent();
    stack<const BasicBlock*> worklist;
    worklist.push(rootBB);
    while (!worklist.empty()) {
        const BasicBlock *BB = worklist.top();
        worklist.pop();
        if (BB == H || reachableBbs.count(BB))  continue;
        if (BB != rootBB)   reachableBbs.insert(BB);

        // Handle successors
        const TerminatorInst *TI = BB->getTerminator();
        for (int i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
            const BasicBlock *succBB = TI->getSuccessor(i);
            if (succBB == H || reachableBbs.count(succBB))  continue;
            worklist.push(succBB);
        }
    }
}


/*
 * Get reachable Instructions and BasicBlocks from a given Instruction.
 * @param root the input root Instruction to start
 * @param reachableInsts the output reachable Instruction set
 * @param reachableBbs the output reachable BasicBlock set
 */
void HeapRefinement::getReachableCode(const Instruction *root,
       InstSet &reachableInsts, BbSet &reachableBbs) {
    // Collect the Instructions following root in the same BasicBlock.
    const Instruction *I = root->getNextNode();
    while (!I->isTerminator()) {
        reachableInsts.insert(I);
        I = I->getNextNode();
    }

    // Collect the reachable BasicBlocks
    const BasicBlock *rootBB = root->getParent();
    stack<const BasicBlock*> worklist;
    worklist.push(rootBB);
    while (!worklist.empty()) {
        const BasicBlock *BB = worklist.top();
        worklist.pop();
        if (reachableBbs.count(BB))  continue;
        if (BB != rootBB)   reachableBbs.insert(BB);

        // Handle successors
        const TerminatorInst *TI = BB->getTerminator();
        for (int i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
            const BasicBlock *succBB = TI->getSuccessor(i);
            if (reachableBbs.count(succBB))  continue;
            worklist.push(succBB);
        }
    }
}


/*
 * Check if a destination Value is exclusively from a given source.
 */
bool HeapRefinement::exclusivelyFrom(const Instruction *src,
        const Value *dst) {
    // Start traversal from dst
    stack<const Value*> worklist;
    worklist.push(dst);
    while (!worklist.empty()) {
        const Value *V = worklist.top();
        worklist.pop();
        V = V->stripPointerCasts();

        // Here we find the matching source
        if (V == src)  return true;

        const Instruction *I = dyn_cast<Instruction>(V);
        if (!I)     return false;

        // We only handle GEP and BitCast at this moment
        if (isa<GetElementPtrInst>(I) || isa<BitCastInst>(I)) {
            worklist.push(I->getOperand(0));
            continue;
        }
        return false;
    }

    return false;
}

