/*
 * ContextSensitiveAliasAnalysis.cpp
 *
 *  Created on: 11/04/2016
 *      Author: dye
 */

#include "ContextSensitiveAliasAnalysis.h"
#include "MhpPathFinder.h"

using namespace analysisUtil;
using namespace llvm;
using namespace std;


/*
 * Instantiation of the "print()" template function with
 * "OperationCollector::CsID" as the typename parameter.
 */
template<>
void CtxBase<OperationCollector::CsID>::print() const {
    outs() << "Ctx[" << context.size() << "]:\t";
    OperationCollector *oc = OperationCollector::getInstance();
    for (int i = 0, e = context.size(); i != e; ++i) {
        OperationCollector::CsID csId = context[i];
        const Instruction *I = oc->getCsInst(csId);
        outs() << rcUtil::getSourceLoc(I);
        if (i + 1 != e) {
            outs() << "  -->  ";
        }
    }
    outs() << "\n";
}


/*
 * Instantiation of the "print()" template function with
 * "OperationCollector::CsID" as the typename parameter.
 */
template<>
void CsPts<HybridCtx<OperationCollector::CsID> >::print() const {
    outs() << "=========== CsPts ===========\t";

    // Status
    string status = isSolved() ? "solved" : "unsolved";
    if (isOutOfBudget())    status = "out of budget";
    if (isInProcess())      status = "in process";

    outs() << status << "\n";

    OperationCollector *oc = OperationCollector::getInstance();

    // Iterate every map pairs
    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
        NodeID n = it->first;
        outs() << n << "  " << rcUtil::getSourceLoc(n) <<  ":\n";
        const CtxSet &ctxSet = it->second;

        // Iterate every ctx
        for (auto it = ctxSet.begin(), ie = ctxSet.end(); it != ie; ++it) {
            const HybridCtx<OperationCollector::CsID> &ctx = (*it);
            const AbstractCtx &abstractCtx = ctx.getAbstractCtx();
            auto &concreteCtx = ctx.getConcreteCtx();

            outs() << "\tCtx[" << ctx.size() << "]:\t";

            // Print abstractCtx
            if (abstractCtx.isValid()) {
                const Instruction *spawnSite = abstractCtx.getSpawnSite();
                MhpAnalysis::ReachableType t = abstractCtx.getReachableType();
                string s;
                if (MhpAnalysis::REACHABLE_TRUNK == t) {
                    s = "TRUNK";
                } else if (MhpAnalysis::REACHABLE_BRANCH == t) {
                    s = "BRANCH";
                } else {
                    s = "INVALID";
                }
                outs() << "AbstractCtx: "
                        << rcUtil::getSourceLoc(spawnSite)
                        << " (" << s << ")";
                if (concreteCtx.size()) {
                    outs() << "  <--  ";
                }
            }

            // Print concreteCtx
            for (int i = 0, e = concreteCtx.size(); i != e; ++i) {
                OperationCollector::CsID csId = concreteCtx[i];
                const Instruction *I = oc->getCsInst(csId);
                outs() << rcUtil::getSourceLoc(I);
                if (i + 1 != e) {
                    outs() << "  -->  ";
                }
            }
            outs() << "\n";
        }
    }
}



/*
 * Instantiation of the "print()" template function with
 * "OperationCollector::CsID" as the typename parameter.
 */
template<>
void CsPts<CtxBase<OperationCollector::CsID> >::print() const {
    outs() << "=========== CsPts ===========\t";

    // Status
    string status = isSolved() ? "solved" : "unsolved";
    if (isOutOfBudget())    status = "out of budget";
    if (isInProcess())      status = "in process";

    outs() << status << "\n";

    OperationCollector *oc = OperationCollector::getInstance();

    // Iterate every map pairs
    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
        NodeID n = it->first;
        outs() << n << "  " << rcUtil::getSourceLoc(n) <<  ":\n";
        const CtxSet &ctxSet = it->second;

        // Iterate every ctx
        for (auto it = ctxSet.begin(), ie = ctxSet.end(); it != ie; ++it) {
            auto &ctx = *it;

            outs() << "\tCtx[" << ctx.size() << "]:\t";

            // Print concreteCtx
            for (int i = 0, e = ctx.size(); i != e; ++i) {
                OperationCollector::CsID csId = ctx[i];
                const Instruction *I = oc->getCsInst(csId);
                outs() << rcUtil::getSourceLoc(I);
                if (i + 1 != e) {
                    outs() << "  -->  ";
                }
            }
            outs() << "\n";
        }
    }
}

// Static member of AbstractCtx class.
MhpAnalysis *AbstractCtx::mhp = NULL;



/*
 * Check if two memory accesses must not access aliases.
 */
bool ContextSensitiveAliasAnalysis::mustNotAccessAliases(AccessID a1,
        AccessID a2) {

    // Perform the refinement analysis
    bool ret = performRefinement(a1, a2);

    // Update statistic information
    if (ret) {
        incrementRefinedPairCount();
    } else {
        incrementUnrefinedPairCount();
    }

    return ret;
}



/*
 * Print statistics
 */
void ContextSensitiveAliasAnalysis::printStat() const {

    // Set format
    std::cout << std::fixed << std::setprecision(2);

    // Print the statistics
    std::cout << " pairs refined: " << getRefinedPairCount()
            << " / " << getTotalQueryPairCount();
    if (getTotalQueryPairCount()) {
        std::cout << " (" << getRefinementPercentage() << "%)";
    }
    std::cout << "\n";
}


/*
 * Initialization.
 */
void HybridCtxBasedCSAA::init(RCMemoryPartitioning *mp,
        MhpAnalysis *mhp) {

    // Initialize CflReachabilityAnalysis
    CFL::init(mp, mhp);

    // Initialize AbstractCtx class
    AbstractCtx::init(mhp);
}


/*
 * Reset the analysis.
 */
void HybridCtxBasedCSAA::reset() {

    // Reset CflReachabilityAnalysis
    CFL::reset();

    // Reset statistics
    resetStat();
}


/*
 * Check if two memory accesses must not access aliases.
 */
bool HybridCtxBasedCSAA::mustNotAccessMemoryPartition(AccessID accessId,
        const MhpAnalysis::ReachablePoint &rp, PartID partId) {

    // Get the context-sensitive access set
    const Pts &accessSet = getCsAccessSet(accessId, rp);
    if (!accessSet.isSolved())      return false;

    // Get the objects in the memory partition
    const PointsTo &partObjs = mp->getPartObjs(partId);

    // Check if the access set must not contain any object that exists in
    // the memory partition; otherwise, return false.
    const PAG *pag = mp->getPta()->getPAG();
    const MemoryAccess &A = mp->getMemoryAccess(accessId);
    const Instruction *I = A.getInstruction();
    bool isIntrisic = isCallSite(I);

    Pts::const_iterator it1 = accessSet.begin();
    Pts::const_iterator ie1 = accessSet.end();
    PointsTo::iterator it2 = partObjs.begin();
    PointsTo::iterator ie2 = partObjs.end();

    while (it1 != ie1 && it2 != ie2) {
        // Get the object from the access set
        NodeID obj1 = it1->first;
        if (isIntrisic) {
            obj1 = pag->getBaseObjNode(obj1);
        }

        // Get the object from the partition
        NodeID obj2 = *it2;

        // Return false if they are aliases
        if (obj1 == obj2)   return false;

        // Compare and move forward the relevant iterator
        if (obj1 < obj2) {
            ++it1;
        } else {
            ++it2;
        }
    }

    return true;
}


/*
 * Dump the context-sensitive alias analysis information.
 */
void HybridCtxBasedCSAA::print() const {

    // Print phase title
    std::string title = " --- Hybrid Context-Sensitive Alias Analysis ---\n";
    std::cout << analysisUtil::pasMsg(title);

    // Print the statistics
    printStat();

    // Print the CFL-based reachability analysis information
    CFL::print();
}


/*
 * Get the context-sensitive access set of a given pointer
 * from a memory access along with a ReachablePoint.
 */
const HybridCtxBasedCSAA::Pts &HybridCtxBasedCSAA::getCsAccessSet(
        AccessID id, const MhpAnalysis::ReachablePoint &rp) {
    const MemoryAccess &A = mp->getMemoryAccess(id);
    const Value *p = A.getPointer();
    NodeID n = pag->getValueNode(p);

    AbstractCtx aCtx = AbstractCtx(rp);
    Ctx ctx;
    ctx.setAbstractCtx(aCtx);

    static Pts pts_;
    Pts &pts = useCache ? ptsCache.getPts(n, ctx) : pts_;

    getPts(n, ctx, pts);

    return pts;
}


/*
 * Check if two memory accesses must not access aliases
 */
bool HybridCtxBasedCSAA::performRefinement(AccessID a1, AccessID a2) {

    // Settings
    dbg = false;
    worklistBased = true;

    // Get the node ids
    const MemoryAccess &A1 = mp->getMemoryAccess(a1);
    const MemoryAccess &A2 = mp->getMemoryAccess(a2);
    const Value *p1 = A1.getPointer();
    const Value *p2 = A2.getPointer();
    NodeID n1 = pag->getValueNode(p1);
    NodeID n2 = pag->getValueNode(p2);

    if (dbg) {
        outs() << "\n##### CflReachability Debug Info #####\n";
        outs() << "trying to refine ...  "
                << n1 << ": " << rcUtil::getSourceLoc(n1) << "\t"
                << n2 << ": " << rcUtil::getSourceLoc(n2) << "\n";
        A1.print();
        A2.print();
    }

    // Get the context-sensitive analysis' results.
    // Check the alias status for every reachable pairs.
    const Instruction *I1 = A1.getInstruction();
    const Instruction *I2 = A2.getInstruction();
    auto reachablePointPairs = mhp->getReachablePointPairs(I1, I2);
    bool isIntrinsic1 = isCallSite(I1);
    bool isIntrinsic2 = isCallSite(I2);

    for (int i = 0, e = reachablePointPairs.size(); i != e; ++i) {
        // Setup contexts
        MhpAnalysis::ReachablePoint &rp1 = reachablePointPairs[i].first;
        MhpAnalysis::ReachablePoint &rp2 = reachablePointPairs[i].second;
        AbstractCtx aCtx1 = AbstractCtx(rp1);
        AbstractCtx aCtx2 = AbstractCtx(rp2);

        Ctx ctx1;
        Ctx ctx2;
        ctx1.setAbstractCtx(aCtx1);
        ctx2.setAbstractCtx(aCtx2);

        // Get the Pts
        Pts pts1_;
        Pts pts2_;
        Pts &pts1 = useCache ? ptsCache.getPts(n1, ctx1) : pts1_;
        Pts &pts2 = useCache ? ptsCache.getPts(n2, ctx2) : pts2_;
        bool ret1 = false;
        bool ret2 = false;

        ret1 = getPts(n1, ctx1, pts1);
        if (!ret1) {
            if (dbg) {
                pts1.print();
            }
            return false;
        }
        ret2 = getPts(n2, ctx2, pts2);
        if (!ret2) {
            if (dbg) {
                pts2.print();
            }
            return false;
        }

        // Now we compare if they are aliases.
        // We are going to modify the pts due to field sensitivity.
        // Thus we make sure the modifications happen on the local clone
        // variable pts_ rather than the cache.
        if (useCache) {
            pts1_ = pts1;
            pts2_ = pts2;
        }

        // Include the struct base object of any field occurrence,
        // when the opposite pts corresponds to an intrinsic accesses
        if (isIntrinsic1) {
            includeFIObjForAnyField(pts2_);
        }
        if (isIntrinsic2) {
            includeFIObjForAnyField(pts1_);
        }

        // Include the struct base object of every first field
        // (i.e. object with zero offset) occurrence.
        includeFIObjForFirstField(pts1_);
        includeFIObjForFirstField(pts2_);

        if (dbg) {
            outs() << "\ndbg ----- spawnSite: "
                    << rcUtil::getSourceLoc(rp1.getSpawnSite()) << "\t";
            outs() << rp1.getReachableType() << "  "
                    << rp2.getReachableType() << "\n";

            outs() << rcUtil::getSourceLoc(p1) << "\t";
            pts1_.print();
            outs() << "\n";

            outs() << rcUtil::getSourceLoc(p2) << "\t";
            pts2_.print();
            outs() << "\n\n";
        }

        // Check alias
        if (pts1_.alias(pts2_)) {
            if (dbg) {
                outs() << "They are aliases !!!!!!!!!\n";
            }

            return false;
        }
    }

    if (dbg) {
        outs() << "REFINED !\n";
    }

    return true;
}



/*
 * Initialization.
 */
void StandardCSAA::init(RCMemoryPartitioning *mp,
        MhpAnalysis *mhp) {

    // Initialize CflReachabilityAnalysis
    CFL::init(mp, mhp);

    // Initialize MhpPathFinder
    mhpPathFinder = new MhpPathFinder();
    mhpPathFinder->init(mhp);
}


/*
 * Reset the analysis.
 */
void StandardCSAA::reset() {

    // Reset CflReachabilityAnalysis
    CFL::reset();

    // Reset statistics
    resetStat();
}


/*
 * Check if two memory accesses must not access aliases.
 */
bool StandardCSAA::mustNotAccessMemoryPartition(AccessID accessId,
        const MhpAnalysis::ReachablePoint &rp, PartID partId) {

    // TODO: add context-sensitive pointer analysis here.

    return false;
}


/*
 * Dump the context-sensitive alias analysis information.
 */
void StandardCSAA::print() const {

    // Print phase title
    std::string title = " --- Standard Context-Sensitive Alias Analysis ---\n";
    std::cout << analysisUtil::pasMsg(title);

    // Print the statistics
    printStat();

    // Print the CFL-based reachability analysis information
    CFL::print();
}


/*
 * Check if two memory accesses must not access aliases
 */
bool StandardCSAA::performRefinement(AccessID a1, AccessID a2) {

    // Settings
    dbg = false;
    worklistBased = true;

    // Get the node ids
    const MemoryAccess &A1 = mp->getMemoryAccess(a1);
    const MemoryAccess &A2 = mp->getMemoryAccess(a2);
    const Value *p1 = A1.getPointer();
    const Value *p2 = A2.getPointer();
    NodeID n1 = pag->getValueNode(p1);
    NodeID n2 = pag->getValueNode(p2);

//    outs() << "----------  " << a1 << "  ---  " << a2 << " \n";
//    A1.print();
//    A2.print();
//    outs() << "\n\n";

//    if (0 == a1 && 0 == a2) {
//        dbg = true;
//    } else {
//        dbg = false;
//    }
//    dbg = true;

    if (dbg) {
        outs() << "\n##### CflReachability Debug Info #####\n";
        outs() << "trying to refine ...  "
                << n1 << ": " << rcUtil::getSourceLoc(n1) << "\t"
                << n2 << ": " << rcUtil::getSourceLoc(n2) << "\n";
        A1.print();
        A2.print();
        outs() << "\n";
    }

    // Get the context-sensitive analysis' results.
    // Check the alias status for every reachable pairs.
    const Instruction *I1 = A1.getInstruction();
    const Instruction *I2 = A2.getInstruction();
    auto reachablePointPairs = mhp->getReachablePointPairs(I1, I2);
    bool isIntrinsic1 = isCallSite(I1);
    bool isIntrinsic2 = isCallSite(I2);

    for (int i = 0, e = reachablePointPairs.size(); i != e; ++i) {
        // Setup dbg
        if (dbg) {
            mhpPathFinder->enableDbg();
        } else {
            mhpPathFinder->disableDbg();
        }

        // Setup contexts
        MhpAnalysis::ReachablePoint &rp1 = reachablePointPairs[i].first;
        MhpAnalysis::ReachablePoint &rp2 = reachablePointPairs[i].second;

        // Get MhpPaths
        const MhpPathFinder::MhpPaths *mhpPaths =
                mhpPathFinder->getMhpPaths(I1, rp1, I2, rp2);

        // Return false if mhpPathFinder is out of budget
        if (!mhpPaths) {
            assert(mhpPathFinder->isOutOfBudget() &&
                    "mhpPathFinder is running out-of-budget");
            return false;
        }

        // Perform context-sensitive alias analysis for all MHP paths
        for (auto it = mhpPaths->begin(), ie = mhpPaths->end(); it != ie;
                ++it) {
            Ctx ctx1 = it->first;
            Ctx ctx2 = it->second;

            // Get the Pts for both queries,
            // return false if either query is not successfully solved.
            Pts pts1_;
            Pts pts2_;
            Pts &pts1 = useCache ? ptsCache.getPts(n1, ctx1) : pts1_;
            Pts &pts2 = useCache ? ptsCache.getPts(n2, ctx2) : pts2_;
            bool ret1 = false;
            bool ret2 = false;

            if (dbg) {
                outs() << "\n************ Start to get Pts for " << n1 << "\n";
            }
            ret1 = getPts(n1, ctx1, pts1);
            if (!ret1) {
                if (dbg) {
                    outs() << "Query is not successfully solved.\n";
                }
                return false;
            }

            if (dbg) {
                outs() << "\n************ Start to get Pts for " << n2 << "\n";
            }
            ret2 = getPts(n2, ctx2, pts2);
            if (!ret2) {
                if (dbg) {
                    outs() << "Query is not successfully solved.\n";
                }
                return false;
            }

            // Now we compare if they are aliases.
            // We are going to modify the pts due to field sensitivity.
            // Thus we make sure the modifications happen on the local clone
            // variable pts_ rather than the cache.
            if (useCache) {
                pts1_ = pts1;
                pts2_ = pts2;
            }

            // Include the struct base object of any field occurrence,
            // when the opposite pts corresponds to an intrinsic accesses
            if (isIntrinsic1) {
                includeFIObjForAnyField(pts2_);
            }
            if (isIntrinsic2) {
                includeFIObjForAnyField(pts1_);
            }

            // Include the struct base object of every first field
            // (i.e. object with zero offset) occurrence.
            includeFIObjForFirstField(pts1_);
            includeFIObjForFirstField(pts2_);


            if (dbg) {
                outs() << "\ndbg ----- spawnSite: "
                        << rcUtil::getSourceLoc(rp1.getSpawnSite()) << "\t";
                outs() << rp1.getReachableType() << "  "
                        << rp2.getReachableType() << "\n";

                outs() << rcUtil::getSourceLoc(p1) << "\n";
                ctx1.print();
                pts1_.print();
                outs() << "\n";

                outs() << rcUtil::getSourceLoc(p2) << "\n";
                ctx2.print();
                pts2_.print();
                outs() << "\n\n";
            }

            // Check alias
            if (pts1_.alias(pts2_)) {
                if (dbg) {
                    outs() << "They are aliases !!!!!!!!!\n";
                }

                return false;
            }
        }
    }

    if (dbg) {
        outs() << "REFINED !\n";
    }

    return true;
}


/*
 * Destructor
 */
StandardCSAA::~StandardCSAA() {
    delete mhpPathFinder;
    mhpPathFinder = NULL;
}

