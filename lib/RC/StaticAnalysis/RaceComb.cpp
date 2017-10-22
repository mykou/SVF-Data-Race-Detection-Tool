/*
 * RaceComb.cpp
 *
 *  Created on: 20/10/2014
 *      Author: dye
 */


#include "RaceComb.h"
#include "ThreadEscapeAnalysis.h"
#include "MhpAnalysis.h"
#include "LocksetAnalysis.h"
#include "BarrierAnalysis.h"
#include "ThreadJoinRefinement.h"
#include "PathRefinement.h"
#include "HeapRefinement.h"
#include "ContextSensitiveAliasAnalysis.h"
#include "ResultValidator.h"
#include "RCStat.h"
#include "RCResult.h"
#include "RC/RCAnnotaton.h"
#include "Util/ThreadCallGraph.h"
#include "Util/RaceAnnotator.h"
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace std;
using namespace analysisUtil;
using namespace rcUtil;


cl::opt<bool> RcAnno("rc-anno", cl::init(false),
        cl::desc("IR annotation with static analysis results for instrumentation"));

cl::opt<bool> RcDetail("rc-detail", cl::init(false),
                           cl::desc("Print detailed results of RaceComb"));

static cl::opt<bool> RcStat("rc-stat", cl::init(false),
                           cl::desc("Statistics of RaceComb"));

static cl::opt<bool> RcIntraproceduralLocksetAnalysis(
        "rc-intraprocedural-lsa", cl::init(false),
        cl::desc("Use lightweight intra-procedural lockset analysis."));

static cl::opt<string> RcContextSensitivity(
        "rc-csaa", cl::init(""),
        cl::desc("Please specifi the context-sensitive alias analysis to be applied for the demand-driven refinement analysis. "
                "Default is the standard context-sensitive alias analysis. "
                "\"hybrid\": use lightweight hybrid-context-sensitive alias analysis. "
                "\"no\": not to use context-sensitive alias analysis"
                ));

static cl::opt<bool> RcVGep("rc-vgep", cl::init(false),
                           cl::desc("Handle VariantGEP edges."));

cl::opt<bool> RcHandleFree("rc-handle-free", cl::init(false),
                           cl::desc("Handle free() operation."));


/*
 * Collect interested operations
 */
void RaceComb::collectOperations() {

    oc->init(M);

    oc->analyze();

    if (RcStat) {
        oc->print();
    }
}


/*
 * Partition memory objects according to their access equivalence
 */
void RaceComb::partitionMemory() {

    mp->init(pta);

    // Add the memory accesses collected from oc to mp.
    mp->addMemoryAccesses(oc->getReads());
    mp->addMemoryAccesses(oc->getWrites());

    mp->run();

    if (RcStat) {
        mp->print();
    }
}


/*
 * Print the data races detected
 */
void RaceComb::settleDataRaceResults() {

    // Init raceResult
    raceResult->init(pta);

    // Prune the non-risky memory accesses
    mp->pruneNonRiskyMemoryAccess();

    // Add data race warnings into results
    vector<RCMemoryPartitioning::Partition> &parts = mp->getParts();
    SmallSet<MemoryPartitioning::AccessID, 16> riskyIds;
    for (int i = 0, e = parts.size(); i != e; ++i) {
        // Check if there is any risky access for this memory partition
        bool hasRiskyAccess = false;
        const MemoryPartitioning::AccessIdVector &accessIds = parts[i].accessIds;
        for (int ii = 0, ee = accessIds.size(); ii != ee; ++ii) {
            MemoryPartitioning::AccessID id = accessIds[ii];
            if (MemoryPartitioning::isPrunedAccess(id))  continue;
            hasRiskyAccess = true;
            break;
        }
        if (!hasRiskyAccess)    continue;

        // Add a new warning to the results
        RCMemoryPartitioning::RiskyInstructionSet &riskyInstSet =
                mp->getRiskyInstSet(i);
        RCStaticAnalysisResult::DataRaceWarning &warning =
                raceResult->addWarning(i, &riskyInstSet);
        for (PointsTo::iterator it = mp->getPartObjs(i).begin(), ie =
                mp->getPartObjs(i).end(); it != ie; ++it) {
            NodeID obj = *it;
            warning.addObj(obj);
        }

        for (int ii = 0, ee = accessIds.size(); ii != ee; ++ii) {
            MemoryPartitioning::AccessID id = accessIds[ii];
            if (MemoryPartitioning::isPrunedAccess(id))  continue;
            const Instruction *I = MemoryPartitioning::getInstruction(id);
            warning.addInstruction(I);
        }
    }

    // Sort the results
    raceResult->sort();

    // Print the results
    raceResult->print();
}


/*
 * Perform annotations for the later instrumentation pass.
 */
void RaceComb::annotateDataraceChecks() {
    // Skip if annotation is not required.
    if (!RcAnno)    return;

    // Perform annotation with RCStaticAnalysisResult.
    RCAnnotator anno;
    anno.init(M, raceResult);
    anno.run();

    if (RcStat) {
        anno.print();
    }
}


/*
 * Perform May-Happen-in-Parallel analysis.
 */
void RaceComb::mhpAnalysis() {

    // Basic MhpAnalysis
    mhp->init(cg, tcg, pta);

    mhp->analyze();

    // Context-sensitive alias analysis
    if (csaa) {
        csaa->init(mp, mhp);
    }
    mp->applyAnalysis(mhp, tea, csaa);

}


/*
 * Perform lockset analysis.
 */
void RaceComb::locksetAnalysis() {

    lsa->init(cg, tcg, pta, RcIntraproceduralLocksetAnalysis.getValue());

    lsa->analyze();

    mp->applyAnalysis(lsa);

}


/*
 * Perform barrier analysis.
 */
void RaceComb::barrierAnalysis() {

    ba->init(cg, tcg, pta);

    ba->analyze();

    mp->applyAnalysis(ba);

}


/*
 * Perform thread escape analysis.
 */
void RaceComb::threadEscapeAnalysis() {

    tea->init(tcg, pta, mp);

    tea->analyze();

    mp->applyAnalysis(tea);

}


/*
 * Perform further refinement.
 */
void RaceComb::furtherRefinement() {

    // HeapRefinement
    heapRefine->init(mhp, lsa, tea);
    mp->applyAnalysis(heapRefine);

    // ThreadJoinRefinement and PathRefinement
    joinRefine->init(mhp);
    pathRefine->init(mhp, mp);
    mp->applyAnalysis(joinRefine, pathRefine);

}


/*
 * Perform context-sensitive refinement.
 */
void RaceComb::contextSensitiveRefinement() {

    if (!csaa)  return;

    // Context-sensitive CFL-reachability analysis
    mp->applyAnalysis(csaa);

    if (RcDetail && RcStat) {
        csaa->print();
    }
}


/*
 * Validate the analysis results for test case.
 */
void RaceComb::validateResults() {
    ResultValidator validator(mp, tea, mhp, lsa, ba, joinRefine, pathRefine,
            heapRefine, csaa);

    // Initialize the validator and perform validation.
    validator.init(M);
    validator.analyze();
}


/*
 * RaceComb initialization.
 */
void RaceComb::init(Module *M, ModulePass *rcPass) {
    stat = new RCStat();
    stat->startTiming(RCStat::Stat_TotalAnalysis);

    this->M = M;
    this->rcPass = rcPass;

    // Pass "this->rcPass" to InterproceduralAnalysisBase
    FunctionPassPool::setModulePass(rcPass);

    // Initialize analysis instances
    oc = OperationCollector::getInstance();

    mp = new RCMemoryPartitioning();

    tea = new ThreadEscapeAnalysis();

    mhp = new MhpAnalysis();

    lsa = new LocksetAnalysis();

    ba = new BarrierAnalysis();

    joinRefine = new ThreadJoinRefinement();

    pathRefine = new PathRefinement();

    heapRefine = new HeapRefinement();

    raceResult = new RCStaticAnalysisResult();

    if (RcContextSensitivity == "no" || RcContextSensitivity == "false") {
        csaa = NULL;
    } else if (RcContextSensitivity == "hybrid") {
        csaa = new HybridCtxBasedCSAA();
    } else {
        csaa = new StandardCSAA();
    }

    // Perform pre-analysis
    stat->startTiming(RCStat::Stat_PreAnalysis);

    // Get PAG
    PAG::handleVGep(RcVGep);
    pag = PAG::getPAG();

    // Perform Andersen's Pointer Analysis
    pta = new AndersenWaveDiff();
    pta->disablePrintStat();
    pta->analyze(*M);

    // Create PTACallGraph and ThreadCallGraph,
    // and update them via pointer analysis result.
    cg = new PTACallGraph(pta->getModule());
    rcUtil::updateCallGraph(cg, pta);
    tcg = new ThreadCallGraph(M);
    tcg->updateCallGraph(pta);

    // Update PAG via pointer analysis result.
    rcUtil::updatePAG(pag, tcg);

    stat->endTiming(RCStat::Stat_PreAnalysis);
}


/*
 * Perform RaceComb analysis.
 */
void RaceComb::analyze() {

    stat->startTiming(RCStat::Stat_RcAnalysis);

    stat->startTiming(RCStat::Stat_OpCollection);
    collectOperations();
    stat->endTiming(RCStat::Stat_OpCollection);

    stat->startTiming(RCStat::Stat_MemPart);
    partitionMemory();
    stat->endTiming(RCStat::Stat_MemPart);

    stat->startTiming(RCStat::Stat_EscapeAnalysis);
    threadEscapeAnalysis();
    stat->endTiming(RCStat::Stat_EscapeAnalysis);

    stat->startTiming(RCStat::Stat_MhpAnalysis);
    mhpAnalysis();
    stat->endTiming(RCStat::Stat_MhpAnalysis);

    stat->startTiming(RCStat::Stat_LocksetAnalysis);
    locksetAnalysis();
    stat->endTiming(RCStat::Stat_LocksetAnalysis);

    stat->startTiming(RCStat::Stat_BarrierAnalysis);
    barrierAnalysis();
    stat->endTiming(RCStat::Stat_BarrierAnalysis);

    stat->startTiming(RCStat::Stat_FurtherRefinement);
    furtherRefinement();
    stat->endTiming(RCStat::Stat_FurtherRefinement);

    stat->startTiming(RCStat::Stat_CsRefinement);
    contextSensitiveRefinement();
    stat->endTiming(RCStat::Stat_CsRefinement);

    stat->endTiming(RCStat::Stat_RcAnalysis);
    stat->endTiming(RCStat::Stat_TotalAnalysis);

    outs().flush();

    if (RcStat) {
        stat->print();
    }

    settleDataRaceResults();

    validateResults();

    annotateDataraceChecks();

}


/*
 * Release resource.
 */
void RaceComb::release() {
    delete tcg;
    tcg = NULL;

    delete oc;
    oc = NULL;

    delete mp;
    mp = NULL;

    delete tea;
    tea = NULL;

    delete mhp;
    mhp = NULL;

    delete lsa;
    lsa = NULL;

    delete pathRefine;
    pathRefine = NULL;

    delete heapRefine;
    heapRefine = NULL;

    delete stat;
    stat = NULL;

    delete raceResult;
    raceResult = NULL;

    FunctionPassPool::clear();

//    AndersenWaveDiff::releaseAndersenWaveDiff();

}

