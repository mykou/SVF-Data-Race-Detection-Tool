/*
 * InterproceduralAnalysis.cpp
 *
 *  Created on: 23/06/2015
 *      Author: dye
 */

#include "InterproceduralAnalysis.h"

using namespace llvm;
using namespace std;


/*
 * Get DominatorTree for Function F.
 */
DominatorTree &FunctionPassPool::getDt(const Function *F) {
    map<const Function*, DominatorTree>::iterator it = func2DtMap.find(F);
    if (it != func2DtMap.end())
        return it->second;

    DominatorTree &dt = func2DtMap[F];
    dt.recalculate(*const_cast<Function*>(F));
    return dt;
}


/*
 * Get DomFrontier for Function F.
 */
DomFrontier &FunctionPassPool::getDf(const Function *F) {
    map<const Function*, DomFrontier>::iterator it = func2DfMap.find(F);
    if (it != func2DfMap.end())
        return it->second;

    DomFrontier &df = func2DfMap[F];
    df.init(F);
    return df;
}


/*
 * Get PostDominatorTree for Function F.
 */
PostDominatorTree &FunctionPassPool::getPdt(const Function *F) {
    map<const Function*, PostDominatorTree>::iterator it =
            func2PdtMap.find(F);
    if (it != func2PdtMap.end())
        return it->second;

    PostDominatorTree &pdt = func2PdtMap[F];
    pdt.recalculate(*const_cast<Function*>(F));
    return pdt;
}


/*
 * Get ScalarEvolution for Function F.
 */
ScalarEvolution &FunctionPassPool::getSe(const Function *F) {
    if (F != func2ScevCache.first) {
        // Create a ScalarEvolution instance
        Function &func = *const_cast<Function*>(F);
        TargetLibraryInfo &tli = modulePass->getAnalysis<
                TargetLibraryInfoWrapperPass>().getTLI();
        AssumptionCache &ac = modulePass->getAnalysis<
                AssumptionCacheTracker>().getAssumptionCache(func);
        DominatorTree &dt = getDt(F);
        LoopInfo &li = *getLi(F);
        ScalarEvolution *SE = new ScalarEvolution(func, tli, ac, dt, li);

        // Update the cache
        func2ScevCache.first = F;
        if (func2ScevCache.second) {
            delete func2ScevCache.second;
        }
        func2ScevCache.second = SE;
    }
    return *func2ScevCache.second;
}


/*
 * Get LoopInfo for Function F.
 */
LoopInfo *FunctionPassPool::getLi(const Function *F) {
    if (F != func2LiCache.first) {
        func2LiCache.first = F;
        if (func2LiCache.second) {
            delete func2LiCache.second;
        }
        func2LiCache.second = new LoopInfo(getDt(F));
    }
    return func2LiCache.second;
}


/*
 * Get ReturnInst for Function F. Return NULL if it does not exist.
 */
const ReturnInst *FunctionPassPool::getRet(const Function *F) {
    map<const Function*, const ReturnInst*>::iterator it =
            func2RetMap.find(F);
    if (it != func2RetMap.end())
        return it->second;

    // Identify the unique ReturnInst
    for (Function::const_iterator it = F->begin(), ie = F->end(); it != ie;
            ++it) {
        const BasicBlock *BB = &*it;
        const TerminatorInst *termInst = BB->getTerminator();
        if (const ReturnInst *RI = dyn_cast<ReturnInst>(termInst)) {
            func2RetMap[F] = RI;
            return RI;
        }
    }
    return NULL;
}


/*
 * Set ModulePass
 */
void FunctionPassPool::setModulePass(ModulePass *modulePass) {
    if (NULL == FunctionPassPool::modulePass)
        FunctionPassPool::modulePass = modulePass;
}


/*
 * Clear all saved analysis and release the memory
 */
void FunctionPassPool::clear() {

    // Clear caches
    func2LiCache.first = NULL;
    if (func2LiCache.second) {
        delete func2LiCache.second;
        func2LiCache.second = NULL;
    }

    func2ScevCache.first = NULL;
    if (func2ScevCache.second) {
        delete func2ScevCache.second;
        func2ScevCache.second = NULL;
    }

    // Clear maps
    func2DtMap.clear();
    func2DfMap.clear();
    func2PdtMap.clear();
    func2LiMap.clear();
    func2RetMap.clear();

    // Set modulePass to NULL
    modulePass = NULL;
}


/*
 * Dump the Functions whose analysis exists in this pool
 */
void FunctionPassPool::print() {
    outs() << " --- FunctionPassPool ---\n";

    // Dump Functions with DominatorTree pass
    outs() << "\tDominatorTree:\t";
    for (auto it = func2DtMap.begin(), ie = func2DtMap.end(); it != ie; ++it) {
        outs() << it->first->getName() << "  ";
    }
    outs() << "\n";

    // Dump Functions with ReturnInst analysis
    outs() << "\tReturnInst:\t";
    for (auto it = func2RetMap.begin(), ie = func2RetMap.end(); it != ie;
            ++it) {
        outs() << it->first->getName() << "  ";
    }
    outs() << "\n";

    // Dump Function with cached ScalarEvolution pass
    outs() << "\tCached ScalarEvolution:\t" << func2ScevCache.first->getName()
            << "\n";

    // Dump Function with cached LoopInfo pass
    outs() << "\tCached LoopInfo:\t" << func2LiCache.first->getName() << "\n";
}


/*
 * Static class members
 */
map<const Function*, DominatorTree> FunctionPassPool::func2DtMap;

map<const Function*, DomFrontier> FunctionPassPool::func2DfMap;

map<const Function*, PostDominatorTree> FunctionPassPool::func2PdtMap;

map<const Function*, LoopInfo*> FunctionPassPool::func2LiMap;

map<const Function*, const ReturnInst*> FunctionPassPool::func2RetMap;

pair<const Function*, ScalarEvolution*> FunctionPassPool::func2ScevCache =
        make_pair((const Function*) NULL, (ScalarEvolution*) NULL);

pair<const Function*, LoopInfo*> FunctionPassPool::func2LiCache = make_pair(
        (const Function*) NULL, (LoopInfo*) NULL);

ModulePass *FunctionPassPool::modulePass = NULL;


/*
 * Dump the SCC information.
 */
void TopoSccOrder::SCC::print() const {
    outs() << " --- SCC ---\n";
    outs() << "\tRep Function:\t" << rep->getName() << "\n";
    outs() << "\tRecursive:\t" << recursive << "\n";
    outs() << "\t" << scc.size() << " Functions:\t";
    for (const_iterator it = begin(), ie = end(); it != ie; ++it) {
        outs() << (*it)->getName() << "  ";
    }
    outs() << "\n";
}

/*
 * Initialization
 * @param cg The callgraph used to compute the SCCs.
 * @param conservativeCg This is used to identify the dead Functions.
 */
void TopoSccOrder::init(PTACallGraph *cg, const PTACallGraph *conservativeCg) {
    this->cg = cg;

    // Identify the SCCs in the call graph
    typedef SCCDetection<PTACallGraph*> CallGraphSCC;
    CallGraphSCC *cgScc = new CallGraphSCC(cg);
    cgScc->find();

    // Identify non-dead Functions from conservativeCg
    const Function *root = cg->getModule()->getFunction("main");
    rcUtil::identifyReachableCallGraphNodes(conservativeCg, root,
            mainReachableFuncs);

    // Re-arrange the SCCs into topoSccs
    CallGraphSCC::GNodeStack topoNodeStack = cgScc->topoNodeStack();
    do {
        NodeID r = topoNodeStack.top();
        topoNodeStack.pop();
        assert(
                r == cgScc->repNode(r)
                        && "topoNodeStack should only contain rep nodes. "
                                "I am not quite sure about this though...");

        // Skip if the Function is not reachable from "main" or is a declaration.
        const Function *F = cg->getCallGraphNode(r)->getFunction();
        if (isDeadFunction(F) || F->isDeclaration())
            continue;

        // Create a SCC instance
        SCC *scc = new SCC(F, cgScc->isInCycle(r));
        topoSccs.push_back(scc);

        // Add subnodes into relevant data structures
        const NodeBS &subNodes = cgScc->subNodes(r);
        for (auto it = subNodes.begin(), ie = subNodes.end(); it != ie; ++it) {
            NodeID n = *it;
            const Function *F = cg->getCallGraphNode(n)->getFunction();
            scc->addFunction(F);
            func2SccMap[F] = scc;
        }
    } while (!topoNodeStack.empty());
}


/*
 * Release resources
 */
void TopoSccOrder::release() {
    for (typename SCCS::iterator it = topoSccs.begin(), ie = topoSccs.end();
            it != ie; ++it) {
        SCC *scc = *it;
        delete scc;
    }
    topoSccs.clear();
}

/*
 * Static class members
 */
TopoSccOrder InterproceduralAnalysisBaseStaticContainer::topoSccOrder;

BVDataPTAImpl *InterproceduralAnalysisBaseStaticContainer::pta = NULL;
