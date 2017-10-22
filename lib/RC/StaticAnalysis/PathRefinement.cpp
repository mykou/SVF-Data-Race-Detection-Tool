/*
 * PathRefinement.cpp
 *
 *  Created on: 04/02/2016
 *      Author: dye
 */

#include "PathRefinement.h"
#include "Util/ThreadCallGraph.h"

using namespace llvm;
using namespace std;
using namespace rcUtil;


/*
 * Compute all valid conditions from a given Instruction
 * along with its holding status.
 * Return if the result is a valid CmpVarConst.
 */
bool PathRefinement::CmpValConst::recalculate(const Instruction *I, bool hold) {

    C = NULL;

    if (!I)     return false;

    this->I = I;

    // Handle the CmpInst case
    if (const CmpInst *CI = dyn_cast<CmpInst>(I)) {
        const Value *opndLhs = I->getOperand(0);
        const Value *opndRhs = I->getOperand(1);

        // Identify the Constant
        C = dyn_cast<Constant>(opndRhs);
        if (C) {
            val = dyn_cast<Instruction>(opndLhs);
            pred = CI->getPredicate();
        } else {
            C = dyn_cast<Constant>(opndLhs);
            if (C) {
                val = dyn_cast<Instruction>(opndRhs);
                pred = getMirrorPredicate(CI->getPredicate());
            }
        }

        if (!val || !C)     return false;

        // Handle the variable (LoadInst on GlobalValue).
        if (const LoadInst *LI = dyn_cast<LoadInst>(val)) {
            ptr = dyn_cast<GlobalValue>(LI->getPointerOperand());
            if (!ptr)   return false;

            if (!hold)  pred = getInversePredicate(pred);
            return true;
        }
    }

    return false;
}


/*
 * Dump information of this CmpValConst
 */
void PathRefinement::CmpValConst::print() const {
    if (!isValid()) {
        outs() << " --- Invalid CmpValConst ---\n";
        return;
    }

    outs() << " --- CmpValConst ---\n";
    outs() << rcUtil::getSourceLoc(I) << "\n";
    outs() << "\tExpr: " << ptr->getName()
            << " " << rcUtil::getPredicateString(pred) << " "
            << C->getValueName() << "\n";
    outs() << "\tI: " << *I << "\n";
    outs() << "\tPtr: " << *ptr << "\n";
    outs() << "\tC: " << *C << "\n";
}


/*
 * Reset this CondVarInfo object content.
 */
void PathRefinement::CondVarInfo::reset(const Value *ptr) {

    this->ptr = ptr;

    computeModsites();

    identifyReachableCode();

}


/*
 * Collect all sites that may directly or indirectly modify this->ptr.
 */
void PathRefinement::CondVarInfo::computeModsites() {
    // Collect the direct modification sites, and initialize the worklist.
    stack<const Function*> worklist;
    for (Value::const_user_iterator it = ptr->user_begin(), ie =
            ptr->user_end(); it != ie; ++it) {
        const Value *user = *it;
        const StoreInst *SI = dyn_cast<StoreInst>(user);
        if (SI && SI->getPointerOperand() == ptr) {
            directModsites.insert(SI);
            const Function *F = SI->getParent()->getParent();
            worklist.push(F);
        }
    }

    // Compute the Functions that may have the side effect
    MhpAnalysis *mhp = refinement->getMhpAnalysis();
    PTACallGraph *cg = mhp->getCallGraph();
    FuncSet funcs;
    while (!worklist.empty()) {
        const Function *F = worklist.top();
        worklist.pop();

        // Check if we have already solved this Function
        if (funcs.count(F))     continue;
        funcs.insert(F);

        PTACallGraphNode *cgNode = cg->getCallGraphNode(F);
        for (auto it = cgNode->getInEdges().begin(), ie = cgNode->getInEdges().end();
                it != ie; ++it) {
            const PTACallGraphEdge *edge = *it;
            const Function *caller = edge->getSrcNode()->getFunction();
            if (!funcs.count(caller))   worklist.push(caller);
        }
    }

    // Compute the side effect Instruction sites
    PTACallGraphEdge::CallInstSet csInsts;
    for (auto it = funcs.begin(), ie = funcs.end(); it != ie; ++it) {
        const Function *F = *it;
        csInsts.clear();
        rcUtil::getAllValidCallSitesInvokingCallee(cg, F, csInsts);
        indirectModsites.insert(csInsts.begin(), csInsts.end());
    }
}


/*
 * Compute the reachable code of every modification site.
 */
void PathRefinement::CondVarInfo::identifyReachableCode() {
    for (auto it = directModsites.begin(), ie = directModsites.end();
            it != ie; ++it) {
        const Instruction *I = *it;
        identifyReachableCodeWithinFunctionScope(I, reachableCodeMap[I]);
    }
    for (auto it = indirectModsites.begin(), ie = indirectModsites.end();
            it != ie; ++it) {
        const Instruction *I = *it;
        identifyReachableCodeWithinFunctionScope(I, reachableCodeMap[I]);
    }
}


/*
 * Intra-procedural reachability analysis
 * @param I The input starting point.
 * @param reachable The output reachable CodeSet.
 */
void PathRefinement::CondVarInfo::identifyReachableCodeWithinFunctionScope(
        const Instruction *I, CodeSet &reachable) {
    static MhpAnalysis::ThreadJoinAnalysis::BlockingCodeInfo emptyBlockingCodeInfo;

    // Identify follower Instructions and BasicBlocks
    MhpAnalysis::ReachabilityAnalysis::identifyReachableInstsAndBBs(I,
            emptyBlockingCodeInfo, reachable);

    // Collect the follower callsites within the scope of F
    std::stack<const Function*> worklist;
    const MhpAnalysis *mhp = refinement->getMhpAnalysis();
    const PTACallGraph *tcg = static_cast<PTACallGraph*>(mhp->getThreadCallGraph());
    const Function *F = I->getParent()->getParent();
    MhpAnalysis::FuncSet callees;
    for (const_inst_iterator it = inst_begin(F), ie = inst_end(F); it != ie;
            ++it) {
        const Instruction *I = &*it;
        if (!reachable.covers(I))   continue;
        if (!isa<CallInst>(I) && !isa<InvokeInst>(I))   continue;

        callees.clear();
        rcUtil::getCallees<MhpAnalysis::FuncSet>(tcg, I, callees);
        for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
            const Function *callee = *it;
            if (!callee || callee->isDeclaration())     continue;
            if (reachable.count(callee))    continue;
            worklist.push(callee);
        }
    }

    // Identify iterative follower Functions from these callsites
    while (!worklist.empty()) {
       const Function* F = worklist.top();
       worklist.pop();

       // Skip previously visited node
       if (reachable.count(F))  continue;
       reachable.insert(F);

       for (const_inst_iterator it = inst_begin(F), ie = inst_end(F); it != ie;
               ++it) {
           const Instruction *I = &*it;
           if (!isa<CallInst>(I) && !isa<InvokeInst>(I))   continue;

           callees.clear();
           rcUtil::getCallees<MhpAnalysis::FuncSet>(tcg, I, callees);
           for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
               const Function *callee = *it;
               if (!callee || callee->isDeclaration())     continue;
               if (reachable.count(callee))    continue;
               worklist.push(callee);
           }
       }
    }
}


PathRefinement *PathRefinement::CondVarInfo::refinement = 0;



/*
 * Initialization
 */
void PathRefinement::init(MhpAnalysis *mhp, RCMemoryPartitioning *mp) {
    this->mhp = mhp;
    this->mp = mp;

    // Initialize CondVarInfo
    CondVarInfo::init(this);

    // Initialize guardExtractor
    ThreadCallGraph *tcg = mhp->getThreadCallGraph();
    guardExtractor.init(tcg, true);
}


/*
 * Check if two Instructions must be guarded by exclusive conditions.
 */
bool PathRefinement::pathRefined(const Instruction *I1,
        const Instruction *I2) {
    // Compute conditions and check if they have condition clash
    const PathRefinement::Guards &guards1 = getGuards(I1);
    const PathRefinement::Guards &guards2 = getGuards(I2);
    if (I1 != I2 && haveExclusiveGuard(guards1, guards2))     return true;

    // Check if "I1" or "I2" has condition conflict against all spawn sites.
    MhpAnalysis::InstSet spawnSites = mhp->mayHappenInParallel(I1, I2);
    assert(0 != spawnSites.size()
                    && "I1 and I2 are expected to be MHP "
                            "as solved by the course-grain analysis.");

    for (auto it = spawnSites.begin(), ie = spawnSites.end(); it != ie; ++it) {
        const Instruction *spawnSite = *it;
        const PathRefinement::Guards &guardsSpawnSite = getGuards(spawnSite);

        if (haveExclusiveGuard(guards1, guardsSpawnSite))   continue;
        if (I1 == I2)   return false;
        if (!haveExclusiveGuard(guards2, guardsSpawnSite))  return false;
    }

    return true;
}


/*
 * Check if two Guards are complementary.
 */
bool PathRefinement::haveExclusiveGuard(const Guards &guards1,
        const Guards &guards2) {

    // Pair every condition from guards1 and guards2
    for (int i = 0, ie = guards1.size(); i != ie; ++i) {
        const CmpValConst &cvc1 = guards1[i];

        for (int j = 0, je = guards2.size(); j != je; ++j) {
            const CmpValConst &cvc2 = guards2[j];

            // Examine if there is any exclusive condition between these two Guards
            bool isExclusive = cvc1.excludes(cvc2);

            // Examine further to check if there is any potential modification
            // that may happen between the two guards
            if (isExclusive) {
                const Value *varPtr = cvc1.getVariablePtr();
                const Instruction *LI1 = cvc1.getVariableVal();
                const Instruction *LI2 = cvc2.getVariableVal();
                bool mustBeConsistent = !mayNotBeConsistent(varPtr, LI1, LI2);
                if (mustBeConsistent)    return true;
            }
        }
    }

    return false;
}


/*
 * Check if the value of a variable may not be consistent
 * at two program points.
 * @param varPtr The pointer of the memory object.
 * @param LI1 A program point.
 * @param LI2 Another program point.
 * @return True if the value of the variable may be inconsistent.
 */
bool PathRefinement::mayNotBeConsistent(const Value *varPtr,
        const Instruction *LI1, const Instruction *LI2) {
    const CondVarInfo &condVarInfo = getCondVarInfo(varPtr);
    const CondVarInfo::ReachableCodeMap &reachableCodeMap =
            condVarInfo.getReachableCodeMap();

    // (1) If any of the modsites may affect only one of the two Instructions,
    // then the two Instructions may read different values of the variable.
    for (auto it = reachableCodeMap.begin(), ie =
            reachableCodeMap.end(); it != ie; ++it) {
        const CodeSet &reachableCode = it->second;
        if (reachableCode.covers(LI1) != reachableCode.covers(LI2))
            return true;
    }

    // (2) If there is any direct modsite may happen in parallel with either
    // LI1 or LI2, then the two Instructions may read different values.
    BVDataPTAImpl *pta = mhp->getPTA();
    PAG *pag = pta->getPAG();
    NodeID n = pag->getValueNode(varPtr);
    PointsTo &pts = pta->getPts(n);

    // We only handle global (singleton) object here.
    if (1 != pts.count())   return true;

    // We do not handle those over-budget riskyInstructionSet.
    RCMemoryPartitioning::PartID id = mp->getPartId(pts.find_first());
    RCMemoryPartitioning::RiskyInstructionSet &riskyInstSet =
            mp->getRiskyInstSet(id);
    if (riskyInstSet.isOverBudget())    return true;

    // Collect the Instructions which is risky when paired to either LI1 or LI2
    InstSet riskyInsts;
    RCMemoryPartitioning::RiskyInstructionSet::RiskyPairs &riskyPairs =
            riskyInstSet.getRiskyPairs();
    for (auto it = riskyPairs.begin(), ie = riskyPairs.end(); it != ie; ++it) {
        const Instruction *I1 = mp->getInstruction(it->first);
        const Instruction *I2 = mp->getInstruction(it->second);
        if (I1 == LI1 || I1 == LI2) {
            riskyInsts.insert(I2);
        } else if (I2 == LI1 || I2 == LI2) {
            riskyInsts.insert(I1);
        }
    }

    // Check if any direct modsite is contained in the risky Instruction set.
    for (auto it = condVarInfo.directModsitesBegin(), ie =
            condVarInfo.directModsitesEnd(); it != ie; ++it) {
        const Instruction *I = *it;
        if (riskyInsts.count(I))    return true;
    }

    return false;
}


