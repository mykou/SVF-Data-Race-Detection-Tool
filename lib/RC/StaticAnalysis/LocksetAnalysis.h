/*
 * LocksetAnalysis.h
 *
 *  Created on: 08/05/2015
 *      Author: dye
 */

#ifndef LOCKSETANALYSIS_H_
#define LOCKSETANALYSIS_H_

#include "WPA/Andersen.h"
#include "InterproceduralAnalysis.h"
#include "PathCorrelationAnalysis.h"
#include <llvm/ADT/SmallSet.h>
#include <llvm/IR/Dominators.h>

class LocksetAnalysis;


/*!
 * SCC summary for LocksetAnalysis.
 */
class LocksetSummary {
public:
    typedef llvm::SmallSet<const llvm::Value*, 4> LockSet;
    typedef CodeSet::InstSet InstSet;
    typedef CodeSet::FuncSet FuncSet;
    typedef std::map<const llvm::Instruction*, LockSet> Inst2LocksetMap;

    /// Add information to summary
    //@{
    inline void addMemoryAccess(const llvm::Instruction *I) {
        accessInsts.insert(I);
    }
    inline void addReachableFunction(const llvm::Function *F) {
        reachableFuncs.insert(F);
    }
    template<typename ITER>
    inline void addReachableFunction(ITER begin, ITER end) {
        reachableFuncs.insert(begin, end);
    }
    template<typename ITER>
    inline void addAcquire(ITER begin, ITER end) {
        acquiredLocks.insert(begin, end);
    }
    template<typename ITER>
    inline void addRelease(ITER begin, ITER end) {
        releasedLocks.insert(begin, end);
    }
    //@}

    /// Iterators
    //@{
    inline InstSet::iterator access_begin() {
        return accessInsts.begin();
    }
    inline InstSet::iterator access_end() {
        return accessInsts.end();
    }
    inline InstSet::const_iterator access_begin() const {
        return accessInsts.begin();
    }
    inline InstSet::const_iterator access_end() const {
        return accessInsts.end();
    }
    inline FuncSet::iterator reachableFuncBegin() {
        return reachableFuncs.begin();
    }
    inline FuncSet::iterator reachableFuncEnd() {
        return reachableFuncs.end();
    }
    inline FuncSet::const_iterator reachableFuncBegin() const {
        return reachableFuncs.begin();
    }
    inline FuncSet::const_iterator reachableFuncEnd() const {
        return reachableFuncs.end();
    }
    inline LockSet::iterator acquire_begin() {
        return acquiredLocks.begin();
    }
    inline LockSet::iterator acquire_end() {
        return acquiredLocks.end();
    }
    inline LockSet::const_iterator acquire_begin() const {
        return acquiredLocks.begin();
    }
    inline LockSet::const_iterator acquire_end() const {
        return acquiredLocks.end();
    }
    inline LockSet::iterator release_begin() {
        return releasedLocks.begin();
    }
    inline LockSet::iterator release_end() {
        return releasedLocks.end();
    }
    inline LockSet::const_iterator release_begin() const {
        return releasedLocks.begin();
    }
    inline LockSet::const_iterator release_end() const {
        return releasedLocks.end();
    }
    //@}

    /// Check if a given Function is reachable
    inline bool isReachable(const llvm::Function *F) const {
        return reachableFuncs.count(F);
    }

    /// Get the lockset that protects a given Instruction.
    //@{
    static inline const LockSet *getProtectingLocks(
            const llvm::Instruction *I) {
        Inst2LocksetMap::const_iterator it = inst2LocksetMap.find(I);
        if (inst2LocksetMap.end() == it)    return NULL;
        return &it->second;
    }
    static inline LockSet &getOrAddProtectingLocks(
            const llvm::Instruction *I) {
        return inst2LocksetMap[I];
    }
    //@}

private:
    /// Summary for inter-procedural analysis
    //@{
    LockSet acquiredLocks;       ///< Must-acquired locks
    LockSet releasedLocks;       ///< May-released locks
    FuncSet reachableFuncs;     ///< Reachable Functions
    //@}

    /// Summary for intra-procedural analysis
    //@{
    InstSet accessInsts;    ///< Instructions of memory accesses
    //@}

    /// Mapping from Instruction to the lockset that protects it
    static Inst2LocksetMap inst2LocksetMap;
};


/*!
 * \brief Intra-procedural lockset analysis for a single Function.
 *
 * This analysis is very lightweight race detection,
 * where the lock/unlock side effects across functions are not considered.
 * This algorithm is not sound when a callee is called by multiple callers,
 * some of which have the call sites guarded but others don't.
 */
class IntraproceduralLocksetAnalyzer {
public:
    typedef CodeSet::InstSet InstSet;
    typedef LocksetSummary::LockSet LockSet;
    typedef LocksetSummary::Inst2LocksetMap Inst2LocksetMap;

    /// Constructor
    IntraproceduralLocksetAnalyzer(LocksetAnalysis *lsa) :
            lsa(lsa), cg(NULL), pta(NULL) {
    }

    /// Initialization
    void init();

    /// Intra-procedural lockset analysis for Function "F".
    void run(const llvm::Function *F);

    /// Summarize the memory accesses of Function "F" and its callees recursively.
    void bottomUpSummarize(const llvm::Function *F, LocksetSummary &summary);

private:
    /// Dump the information of every protected Instruction with its lockset.
    void dump() const;

    LocksetAnalysis *lsa;
    PTACallGraph *cg;
    BVDataPTAImpl *pta;
    Inst2LocksetMap inst2LocksetMap;
};


/*!
 * Lightweight Lockset Analysis.
 * Either intra- or inter-procedural lockset analysis is performed.
 */
class LocksetAnalysis: public InterproceduralAnalysisBase<LocksetAnalysis,
            LocksetSummary> {
public:
    typedef InterproceduralAnalysisBase<LocksetAnalysis, LocksetSummary> SUPER;
    typedef CodeSet::FuncSet FuncSet;
    typedef CodeSet::BbSet BbSet;
    typedef CodeSet::InstSet InstSet;
    typedef CodeSet::ValSet ValSet;
    typedef LocksetSummary::LockSet LockSet;

    friend SUPER;

    /// Constructor
    LocksetAnalysis() : intraproceduralLocksetAnalyzer(NULL), tcg(NULL) {
    }

    /*!
     * Initialization
     * @param cg The callgraph.
     * @param tcg the ThreadCallGraph
     * @param pta the pointer analysis
     * @param useIntraproceduralAnalysis whether to perform the unsound
     * but more efficient intra-procedural lockset analysis instead of
     * the inter-procedural one
     */
    void init(PTACallGraph *cg, ThreadCallGraph *tcg, BVDataPTAImpl *pta,
            bool useIntraproceduralAnalysis);

    /*!
     * \brief Perform the lockset analysis.
     * It contains two phases:
     * (1) a bottom-up phase, and (2) a top-down phase.
     */
    void analyze();

    /// Release resource
    void release();

    /// Destructor
    virtual ~LocksetAnalysis() {
    }

    /// Get ThreadCallGraph
    inline ThreadCallGraph *getThreadCallGraph() {
        return tcg;
    }

    /// Get PathCorrelationAnaysis
    inline PathCorrelationAnaysis &getPathCorrelationAnaysis() {
        return pca;
    }

    /// Check if two Instructions are protected by the identical lockset
    inline bool protectedBySameLockset(const llvm::Instruction *I1,
            const llvm::Instruction *I2) {
        const LockSet *lockset1 = getProtectingLocks(I1);
        const LockSet *lockset2 = getProtectingLocks(I2);
        return sameLockSet(lockset1, lockset2);
    }

    /// Check if two Instructions are protected by any common lock
    inline bool protectedByCommonLocks(const llvm::Instruction *I1,
            const llvm::Instruction *I2) {
        const LockSet *lockset1 = getProtectingLocks(I1);
        const LockSet *lockset2 = getProtectingLocks(I2);
        return hasCommonLock(lockset1, lockset2);
    }

    /**
     * Get all memory access operations in a Function and its callee(s) recursively.
     * @param F the given Function
     * @param accesses the record of the results
     */
    inline void getMemoryAccesses(const llvm::Function *F,
            InstSet &accesses) const {
        const LocksetSummary *summary = getSccSummary(F);
        if (!summary)   return;
        accesses.insert(summary->access_begin(), summary->access_end());
    }

protected:
    /// Main Function visitor for inter-procedural analysis.
    void visitFunction(const llvm::Function *F);

private:
    /*!
     * Base class when analyzing a single Function in an inter-procedural
     * analysis phase.
     */
    class FunctionAnalyzerBase {
    protected:
        typedef LocksetSummary::Inst2LocksetMap Inst2LocksetMap;
        typedef std::map<const llvm::Function*, Inst2LocksetMap> UniversalInst2LocksetMap;

        /// Constructor
        FunctionAnalyzerBase(LocksetAnalysis *lsa,
                const llvm::Function *F) :
                lsa(lsa), F(F), lockSites(universalLockSites[F]),
                unlockSites(universalUnlockSites[F]) {
        }

        /*!
         * Identify all unlock sites that may release any lock
         * in lockset in Function "this->F".
         * @param lockset the input lockset
         * @param correspondingUnlockSites the output that records the
         * unlock sites that may unlock the input lockset
         */
        void identifyCorrespondingUnlockSites(const LockSet &lockset,
                InstSet &correspondingUnlockSites) const;

        LocksetAnalysis *lsa;
        const llvm::Function *F;
        Inst2LocksetMap &lockSites;
        Inst2LocksetMap &unlockSites;

        /// Records of the inter-procedural analysis results
        //@{
        static UniversalInst2LocksetMap universalLockSites;
        static UniversalInst2LocksetMap universalUnlockSites;
        //@}
    };

    /*!
     * Single Function analyzer for inter-procedural bottom-up phase.
     */
    class BottomUpAnalyzer : public FunctionAnalyzerBase {
    public:
        /// Constructor
        BottomUpAnalyzer(LocksetAnalysis *lsa, const llvm::Function *F) :
            FunctionAnalyzerBase(lsa, F), retInst(NULL) {
        }

        /// Access to "mustAcquireLockset" and "mayReleaseLockset"
        //@{
        inline const ValSet &getMustAcquireLockset() const {
            return mustAcquiredLockset;
        }
        inline const ValSet &getMayReleaseLockset() const {
            return mayReleasedLockset;
        }
        //@}

        /// Perform analysis on the single Function.
        void run();

    private:
        /// Identify interesting stuffs including ReturnInst,
        /// lock/unlock from csInsts, and reachable Functions.
        //@{
        void identifyInterestingStuffsForFunction();
        void identifyInterestingStuffsForScc();
        //@}

        /// Match the lock acquire/release sites intra-procedurally.
        void matchAcquireReleaseLockSites();

        /// Get the matching lock acquire/release sites.
        //@{
        inline const InstSet &getMatchingUnlockSites(
                const llvm::Instruction *lockSite) const {
            auto it = matchingUnlockSites.find(lockSite);
            if (it == matchingUnlockSites.end())     return emptyInstSet;
            return it->second;
        }
        inline const InstSet &getMatchingLockSites(
                const llvm::Instruction *unlockSite) const {
            auto it = matchingLockSites.find(unlockSite);
            if (it == matchingLockSites.end())     return emptyInstSet;
            return it->second;
        }
        //@}

        /// Compute the must-acquired and may-released locksets.
        //@{
        void computeMustAcquiredLockset();
        void computeMayReleasedLockset();
        //@}

        /*!
         * Check if a given lock site is not blocked for every path to the
         * return site in the CFG.
         * @param lockSite the given lock site
         * @param correspondingUnlockSites the unlock sites that may release
         * the acquired locks in lockSite
         */
        bool hasAcquireLockSideEffect(
                const llvm::Instruction *lockSite,
                const InstSet &correspondingUnlockSites) const;

        /// The unique return site of the Function
        const llvm::Instruction *retInst;

        /// The must-acquired and may-released locksets
        //@{
        ValSet mustAcquiredLockset;
        ValSet mayReleasedLockset;
        //@}

        /// Match the lock acquire/release sites.
        //@{
        std::map<const llvm::Instruction*, InstSet> matchingLockSites;
        std::map<const llvm::Instruction*, InstSet> matchingUnlockSites;
        static InstSet emptyInstSet;
        //@}
    };

    /*!
     * Single Function analyzer for inter-procedural top-down phase.
     */
    class TopDownAnalyzer : public FunctionAnalyzerBase {
    public:
        /// Constructor
        TopDownAnalyzer(LocksetAnalysis *lsa, const llvm::Function *F) :
                FunctionAnalyzerBase(lsa, F) {
        }

        /// Perform the analysis on the single Function.
        void run();

    private:
        /*!
         * Identify the side effects of acquired lockset from callers.
         * We only consider must-acquire lockset, which contains
         * the acquired lock from all call sites that "this-F" is called.
         */
        void identifyEntryLockset();

        /// Perform reachability analysis to identify protected Instructions
        void performReachabilityAnalysis();

        /*!
         * Identify protected Instructions of a given lockset from a relevant acquire site.
         * This algorithm has some path sensitivity.
         * @param root the root Instruction (usually the immediate
         * following Instruction of a lock acquire site) to start traversal
         * @param lockset the lockset acquired
         */
        void identifyProtectedInstructions(const llvm::Instruction *lockSite,
                const LockSet &lockset);

        /*!
         * Identify the protected Instructions .
         * @param root the root Instruction to start traversal
         * @param lockset the protecting lockset
         * @param unlockSites the corresponding lock release sites
         * @param dt DominatorTree
         * @param cg CallGraph
         * @param visited the visited Instructions
         * @param protectedFrontiers the Instructions becoming unprotected
         */
        void protecting(const llvm::Instruction *root, const LockSet &lockset,
                const InstSet &unlockSites, llvm::DominatorTree &dt,
                PTACallGraph *cg, InstSet &visited,
                InstSet &protectedFrontiers);

        /// Record of lockset that must be acquired from all
        /// call sites that "this-F" is called.
        LockSet entryLockset;
    };

    /// Inter-procedural analysis Function visitors.
    //@{
    void bottomUpFunctionVisitor(const llvm::Function *F);
    void topDownFunctionVisitor(const llvm::Function *F);
    //@}

    /// Check if two locksets are identical.
    inline bool sameLockSet(const LockSet *lockset1, const LockSet *lockset2) {
        if (!lockset1 || !lockset2) return false;
        PointerAnalysis *pta = getPTA();
        for (LockSet::const_iterator it = lockset1->begin(), ie =
                lockset1->end(); it != ie; ++it) {
            const llvm::Value *p1 = *it;
            bool hasMatchedLock = false;
            for (LockSet::const_iterator it = lockset2->begin(), ie =
                    lockset2->end(); it != ie; ++it) {
                const llvm::Value *p2 = *it;
                if (rcUtil::alias(p1, p2, pta)) {
                    hasMatchedLock = true;
                    break;
                }
            }
            if (false == hasMatchedLock)    return false;
        }
        return true;
    }

    /// Check if two locksets share any common lock.
    //@{
    inline bool hasCommonLock(const LockSet *lockset1, const LockSet *lockset2) {
        if (!lockset1 || !lockset2) return false;
        return hasCommonLock(*lockset1, *lockset2);
    }
    inline bool hasCommonLock(const LockSet &lockset1, const LockSet &lockset2) {
        PointerAnalysis *pta = getPTA();
        for (ValSet::const_iterator it = lockset1.begin(), ie =
                lockset1.end(); it != ie; ++it) {
            const llvm::Value *p1 = *it;
            for (ValSet::const_iterator it = lockset2.begin(), ie =
                    lockset2.end(); it != ie; ++it) {
                const llvm::Value *p2 = *it;
                if (rcUtil::alias(p1, p2, pta))     return true;
            }
        }
        return false;
    }
    //@}

    /// Get the lockset that protects the Instruction "I".
    inline const LockSet *getProtectingLocks(const llvm::Instruction *I) const {
        return LocksetSummary::getProtectingLocks(I);
    }

    /// If not NULL, the intra-procedural analysis will be performed
    /// instead of the inter-procedural one.
    IntraproceduralLocksetAnalyzer *intraproceduralLocksetAnalyzer;

    ThreadCallGraph *tcg;
    PathCorrelationAnaysis pca;
};


#endif /* LOCKSETANALYSIS_H_ */
