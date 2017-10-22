/*
 * BarrierAnalysis.h
 *
 *  Created on: 05/07/2016
 *      Author: dye
 */

#ifndef BARRIERANALYSIS_H_
#define BARRIERANALYSIS_H_

#include "InterproceduralAnalysis.h"

class ThreadCallGraph;

/*!
 * The SCC summary information of BarrierAnalysis.
 */
class BarrierSummary {
public:
    typedef CodeSet::ValSet ValSet;

    /*!
     * Data structure to record the code that must affected
     * before/after any waiting operation of a barrier object.
     */
    struct BarrierEffect {
        CodeSet beforeCode;
        CodeSet afterCode;
    };

    typedef std::map<const llvm::Value*, BarrierEffect> Barrier2EffectMap;

    /// Constructor
    BarrierSummary() {
    }

    /// Iterators for barriers waited in this SCC
    //@{
    inline ValSet::const_iterator barrierBegin() const {
        return barriers.begin();
    }
    inline ValSet::const_iterator barrierEnd() const {
        return barriers.end();
    }
    inline ValSet::iterator barrierBegin() {
        return barriers.begin();
    }
    inline ValSet::iterator barrierEnd() {
        return barriers.end();
    }
    //@}

    /// Add barriers
    //@{
    inline void addBarriers(const llvm::Value *bar) {
         barriers.insert(bar);
    }
    inline void addBarriers(const ValSet &bars) {
         barriers.insert(bars.begin(), bars.end());
    }
    //@}

    /// Get the Barrier2EffectMap
    static inline const Barrier2EffectMap &getBarrier2EffectMap() {
        return barrier2EffectMap;
    }

    /// Get the BarrierEffect of a given barrier.
    //@{
    static inline const BarrierEffect *getBarrierEffect(
            const llvm::Value *barrier) {
        auto it = barrier2EffectMap.find(barrier);
        if (barrier2EffectMap.end() == it)      return NULL;
        return &it->second;
    }
    static inline BarrierEffect &getOrAddBarrierEffect(
            const llvm::Value *barrier) {
        return barrier2EffectMap[barrier];
    }
    //@}

private:

    /// Records of the barrier objects in this SCC
    ValSet barriers;

    /// Mapping from barrier object to the affected code
    static Barrier2EffectMap barrier2EffectMap;
};


/*!
 * Barrier Analysis.
 * This algorithm is not sound, since it applies an aggressive strategy
 * to deal with barriers in loops.
 */
class BarrierAnalysis: public InterproceduralAnalysisBase<BarrierAnalysis,
            BarrierSummary> {
public:
    typedef InterproceduralAnalysisBase<BarrierAnalysis, BarrierSummary> SUPER;
    typedef CodeSet::FuncSet FuncSet;
    typedef CodeSet::BbSet BbSet;
    typedef CodeSet::InstSet InstSet;
    typedef CodeSet::ValSet ValSet;
    typedef BarrierSummary::BarrierEffect BarrierEffect;

    friend SUPER;

    /// Constructor
    BarrierAnalysis() : cg(NULL), tcg(NULL), pta(NULL) {
    }

    /*!
     * Initialization
     * @param cg The callgraph.
     * @param tcg The thread callgraph.
     * @param pta The pointer analysis.
     */
    void init(PTACallGraph *cg, ThreadCallGraph *tcg, BVDataPTAImpl *pta);

    /*!
     * \brief Perform the barrier analysis.
     * It contains two phases:
     * (1) a bottom-up phase, and
     * (2) a top-down phase.
     */
    void analyze();

    /// Release the resources
    void release();

    /// Destructor
    virtual ~BarrierAnalysis() {
        release();
    }

    /*!
     * The interface of the analysis to answer if two given
     * Instructions must be separated by any barrier.
     */
    bool separatedByBarrier(const llvm::Instruction *I1,
            const llvm::Instruction *I2) const;

protected:
    /// Generic Function visitor of inter-procedural analysis
    void visitFunction(const llvm::Function *F);

private:
    /*!
     * Base class when analyzing a single Function in an inter-procedural
     * analysis phase.
     */
    class FunctionAnalyzerBase {
    protected:
        typedef std::map<const llvm::Instruction*, ValSet> Inst2BarriersMap;
        typedef std::map<const llvm::Function*, Inst2BarriersMap> UniversalInst2BarriersMap;

        /// Constructor
        FunctionAnalyzerBase(BarrierAnalysis *ba, const llvm::Function *F) :
                ba(ba), F(F), barrierSites(universalBarrierSites[F]) {
        }

        BarrierAnalysis *ba;
        const llvm::Function *F;
        Inst2BarriersMap &barrierSites;

        /// Records of the inter-procedural analysis results
        static UniversalInst2BarriersMap universalBarrierSites;
    };

    /*!
     * Single Function analyzer for inter-procedural bottom-up phase.
     */
    class BottomUpAnalyzer : public FunctionAnalyzerBase {
    public:
        BottomUpAnalyzer(BarrierAnalysis *ba, const llvm::Function *F) :
            FunctionAnalyzerBase(ba, F), retInst(NULL) {
        }

        /// Perform the bottom-up analysis.
        void run();

        /// Access to "mustWaitedBarriers"
        inline const ValSet &getMustWaitedBarriers() const {
            return mustWaitedBarriers;
        }

    private:
        /// Identify some stuffs including return Instruction and
        /// barrier-waiting effects for later analysis.
        void identifyInterestingStuffs();

        /// Compute the must-waited barriers of the current Function.
        void computeMustWaitedBarriers();

        const llvm::Instruction *retInst;
        ValSet mustWaitedBarriers;
    };


    /*!
     * Single Function analyzer for inter-procedural top-down phase.
     */
    class TopDownAnalyzer : public FunctionAnalyzerBase {
    public:
        /// Constructor
        TopDownAnalyzer(BarrierAnalysis *ba, const llvm::Function *F) :
            FunctionAnalyzerBase(ba, F) {
        }

        /// Perform the top-down analysis.
        void run();

    private:
        /// Compute BarrierEffect for relevant external/internal barriers.
        //@{
        void processExternalBarriers();
        void processInternalBarriers();
        //@}

        /*!
         * Compute the BarrierEffect for a barrier object at a wait site.
         * The result is recorded in BarrierSummary.
         * @param barrier the pointer to the barrier object
         * @param waitSite the barrier-waiting site
         */
        void processBarrierEffect(const llvm::Value *barrier,
                const llvm::Instruction *waitSite);

        /*!
         * Perform intra-procedural dominant analysis.
         * @param rootInst the root Instruction to start traversal;
         * it is excluded before the traversal happens,
         * but it may be included later if there is a cycle.
         * @param dt the DominatorTree
         * @param dominatee the output of the dominated code.
         */
        static void identifyDominatedInstsAndBBs(
                const llvm::Instruction *rootInst, llvm::DominatorTree &dt,
                CodeSet &dominatee);

        /*!
         * Perform intra-procedural post-dominant analysis.
         * @param rootInst the root Instruction to start traversal;
         * it is excluded before the traversal happens,
         * but it may be included later if there is a cycle.
         * @param pdt the PostDominatorTree
         * @param postDominatee the output of the post-dominated code.
         */
        static void identifyPostDominatedInstsAndBBs(
                const llvm::Instruction *rootInst, llvm::PostDominatorTree &pdt,
                CodeSet &postDominatee);

        /// Identify intra-procedural forward/backward reachable code.
        //@{
        static void identifyForwardReachableInstructions(
                const llvm::Instruction *I, CodeSet &reachable);
        static void identifyBackwardReachableInstructions(
                const llvm::Instruction *I, CodeSet &reachable);
        //@}
    };

    /// The bottom-up and top-down visitors
    //@{
    /// In the bottom-up phase, the visitor summarizes the
    /// barrier objects that must be waited by the Function.
    void buttomUpVisitor(const llvm::Function *F);

    /// In the top-down phase, the visitor compute the BarrierEffect
    /// for all barrier objects involved.
    void topDownVisitor(const llvm::Function *F);
    //@}

    /// Check if a given input Instruction is an interested one.
    static inline bool isInterstedInst(const llvm::Instruction *I) {
        return llvm::isa<llvm::LoadInst>(I) || llvm::isa<llvm::StoreInst>(I)
                || llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I);
    }

    PTACallGraph *cg;
    ThreadCallGraph *tcg;
    BVDataPTAImpl *pta;
};


#endif /* BARRIERANALYSIS_H_ */
