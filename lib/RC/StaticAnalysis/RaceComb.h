/*
 * RaceComb.h
 *
 *  Created on: 20/10/2014
 *      Author: dye
 */

#ifndef RACECOMB_H_
#define RACECOMB_H_

#include "RCUtil.h"
#include "MemoryPartitioning.h"


class ThreadCallGraph;
class ThreadEscapeAnalysis;
class MhpAnalysis;
class LocksetAnalysis;
class BarrierAnalysis;
class ThreadJoinRefinement;
class PathRefinement;
class HeapRefinement;
class ContextSensitiveAliasAnalysis;
class RCStat;
class RCStaticAnalysisResult;


/*!
 * RaceComb analysis main class
 */
class RaceComb {
public:
    /// Constructor
    RaceComb() :
            M(NULL), rcPass(NULL),
            pag(NULL), cg(NULL), tcg(NULL), pta(NULL),
            oc(NULL), mp(NULL), tea(NULL),
            mhp(NULL), lsa(NULL), ba(NULL),
            joinRefine(NULL), pathRefine(NULL), heapRefine(NULL),
            csaa(NULL), stat(NULL), raceResult(NULL) {
    }

    /// Initialize RaceComb with Module M
    void init(llvm::Module *M, llvm::ModulePass *rcPass);

    /// Perform RaceComb analysis
    void analyze();

    /// Release resource
    void release();


protected:
    /// Collect interested operations at the beginning of the analysis.
    void collectOperations();

    /// Partition memory objects into access equivalent parts.
    void partitionMemory();

    /// Major analysis
    //@{
    void threadEscapeAnalysis();
    void mhpAnalysis();
    void locksetAnalysis();
    void barrierAnalysis();
    void furtherRefinement();
    void contextSensitiveRefinement();
    //@}

    /// Organize the results of the data race detection.
    void settleDataRaceResults();

    /// Perform annotations for the later instrumentation pass.
    void annotateDataraceChecks();

    /// Validation for the test cases if applicable.
    void validateResults();

private:
    llvm::Module *M;
    llvm::ModulePass *rcPass;
    PAG *pag;
    PTACallGraph *cg;
    ThreadCallGraph *tcg;
    BVDataPTAImpl *pta;
    OperationCollector *oc;
    RCMemoryPartitioning *mp;
    ThreadEscapeAnalysis *tea;
    MhpAnalysis *mhp;
    LocksetAnalysis *lsa;
    BarrierAnalysis *ba;
    ThreadJoinRefinement *joinRefine;
    PathRefinement *pathRefine;
    HeapRefinement *heapRefine;
    ContextSensitiveAliasAnalysis *csaa;
    RCStat *stat;
    RCStaticAnalysisResult *raceResult;
};


#endif /* RACECOMB_H_ */
