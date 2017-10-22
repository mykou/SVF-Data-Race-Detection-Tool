/*
 * InterproceduralAnalysis.h
 *
 *  Created on: 18/05/2015
 *      Author: dye
 */

#ifndef INTERPROCEDURALANALYSIS_H_
#define INTERPROCEDURALANALYSIS_H_

#include "RCUtil.h"
#include "MemoryModel/PointerAnalysis.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/AssumptionCache.h>

/*!
 * Storage of FunctionPass that can be reused by inter-procedural analysis.
 */
class FunctionPassPool {
public:
    /// Get DominatorTree for Function F.
    static llvm::DominatorTree &getDt(const llvm::Function *F);

    /// Get DomFrontier for Function F.
    static DomFrontier &getDf(const llvm::Function *F);

    /// Get PostDominatorTree for Function F.
    static llvm::PostDominatorTree &getPdt(const llvm::Function *F);

    /// Get ScalarEvolution for Function F.
    static llvm::ScalarEvolution &getSe(const llvm::Function *F);

    /// Get LoopInfo for Function F.
    static llvm::LoopInfo *getLi(const llvm::Function *F);

    /// Get ReturnInst for Function F. Return NULL if it does not exist.
    static const llvm::ReturnInst *getRet(const llvm::Function *F);

    /// Set ModulePass
    static void setModulePass(llvm::ModulePass *modulePass);

    /// Clear all saved analysis and release the memory
    static void clear();

    /// Dump the Functions whose pass exists in this pool
    static void print();

private:
    /// Mapping from Function to the desired analysis result.
    //@{
    static std::map<const llvm::Function*, llvm::DominatorTree> func2DtMap;
    static std::map<const llvm::Function*, DomFrontier> func2DfMap;
    static std::map<const llvm::Function*, llvm::PostDominatorTree> func2PdtMap;
    static std::map<const llvm::Function*, llvm::LoopInfo*> func2LiMap;
    static std::map<const llvm::Function*, const llvm::ReturnInst*> func2RetMap;
    //@}

    /// Cache of relevant FuncionPass
    //@{
    static std::pair<const llvm::Function*, llvm::ScalarEvolution*> func2ScevCache;
    static std::pair<const llvm::Function*, llvm::LoopInfo*> func2LiCache;
    //@}

    static llvm::ModulePass *modulePass;
};


/*!
 * Topological order of the SCC call graph.
 */
class TopoSccOrder {
public:
    /*!
     * Strongly connected component in the call graph.
     */
    class SCC {
    public:
        typedef llvm::SmallVector<const llvm::Function*, 8> FuncVector;
        typedef FuncVector::const_iterator const_iterator;
        typedef FuncVector::iterator iterator;

        /*!
         * Constructor with explicit parameters
         * @param rep the representation Function of this SCC
         * @param recursive whether this SCC is recursive
         */
        SCC(const llvm::Function *rep, bool recursive) :
                rep(rep), recursive(recursive) {
        }

        /// Iterators
        //@{
        inline const_iterator begin() const {
            return scc.begin();
        }
        inline const_iterator end() const {
            return scc.end();
        }
        inline iterator begin() {
            return scc.begin();
        }
        inline iterator end() {
            return scc.end();
        }
        //@}

        /// Size
        inline size_t size() const {
            return scc.size();
        }

        /// Member variable accesses
        //@{
        inline void setRep(const llvm::Function *F) {
            rep = F;
        }
        inline const llvm::Function *getRep() const {
            return rep;
        }
        inline void setRecursive(bool b) {
            recursive = b;
        }
        inline bool isRecursive() const {
            return recursive;
        }
        inline void addFunction(const llvm::Function *F) {
            scc.push_back(F);
        }
        //@}

        /// Collect all call site Instructions of all Functions in SCC.
        template<typename T>
        void collectCallSites(T &csInsts) const {
            for (SCC::const_iterator it = begin(), ie = end(); it != ie; ++it) {
                const llvm::Function *F = *it;
                for (llvm::const_inst_iterator it = inst_begin(F), ie = inst_end(F);
                        it != ie; ++it) {
                    const llvm::Instruction *I = &*it;
                    if (llvm::isa<llvm::CallInst>(I)
                            || llvm::isa<llvm::InvokeInst>(I))
                        csInsts.insert(I);
                }
            }
        }

        /// Dump the SCC information
        void print() const;

    protected:
        FuncVector scc;
        const llvm::Function *rep;
        bool recursive;
    };

    typedef std::vector<SCC*> SCCS;
    typedef SCCS::iterator iterator;
    typedef SCCS::reverse_iterator reverse_iterator;
    typedef llvm::DenseMap<const llvm::Function*,  const SCC*> Func2SccMap;


    /// Constructor
    TopoSccOrder() : cg(NULL) {
    }

    /*!
     * Initialization
     * @param cg The callgraph used to compute the SCCs.
     * @param conservativeCg This is used to identify the dead Functions.
     */
    void init(PTACallGraph *cg, const PTACallGraph *conservativeCg);

    /// Release resources
    void release();

    /// Iterators
    //@{
    inline iterator begin() {
        return topoSccs.begin();
    }
    inline iterator end() {
        return topoSccs.end();
    }
    inline reverse_iterator rbegin() {
        return topoSccs.rbegin();
    }
    inline reverse_iterator rend() {
        return topoSccs.rend();
    }
    //@}

    /// Get the PTACallGraph
    //@{
    inline PTACallGraph *getCallGraph() {
        return cg;
    }
    inline const PTACallGraph *getCallGraph() const {
        return cg;
    }
    //@}

    /// Get SCC of a given Function
    inline const SCC *getScc(const llvm::Function *F) const {
        typename Func2SccMap::const_iterator it = func2SccMap.find(F);
        assert(it != func2SccMap.end() && it->second
                && "Every Function must correspond to a SCC.");
        return it->second;
    }

    /// Check if a given Function is a dead Function.
    inline bool isDeadFunction(const llvm::Function *F) const {
        return !mainReachableFuncs.count(F);
    }

private:
    PTACallGraph *cg;
    SCCS topoSccs;
    Func2SccMap func2SccMap;
    llvm::DenseSet<const llvm::Function*> mainReachableFuncs;
};


/*!
 * Container class of a static members of the
 * InterproceduralAnalysisBase class.
 */
class InterproceduralAnalysisBaseStaticContainer {
public:
    /// Get PTA
    static inline BVDataPTAImpl *getPTA() {
        return pta;
    }

    /// Check if a Function is a dead function (i.e., not reachable from main)
    static bool isDeadFunction(const llvm::Function *F) {
        assert (pta &&
                "InterproceduralAnalysisBaseStaticContainer should have been initialized.\n");
        return topoSccOrder.isDeadFunction(F);
    }

protected:
    static TopoSccOrder topoSccOrder;
    static BVDataPTAImpl *pta;
};


/*!
 * The base class for inter-procedural program analysis.
 * This class should be inherited when used, along with another
 * class for the summary of each SCC in the call graph.
 */
template<typename SubClass, typename SccSummary>
class InterproceduralAnalysisBase: public FunctionPassPool,
        public InterproceduralAnalysisBaseStaticContainer {
public:
    /// The order of function visiting in the inter-procedural analysis.
    enum AnalysisOrder {
        AO_DEFAULT,
        AO_BOTTOM_UP,
        AO_TOP_DOWN,
    };

    typedef TopoSccOrder::SCC SCC;
    typedef std::map<const SCC*, SccSummary> Scc2SummaryMap;

    /*!
     * Initialization
     * @param cg The callgraph used for the inter-procedural analysis.
     * @param conservativeCg The callgraph used to identify the dead Functions.
     * @param pta The pointer analysis.
     */
    void init(PTACallGraph *cg, const PTACallGraph *conservativeCg,
            BVDataPTAImpl *pta) {
        // Initialize topoSccOrder if it has not been done yet.
        if (NULL == this->pta) {
            this->pta = pta;
            topoSccOrder.init(cg, conservativeCg);
        }
        currentScc = NULL;
    }

    /// Perform analysis
    void analyze(AnalysisOrder order){
        setCurrentAnalysisOrder(order);

        // Bottom-up analysis
        switch(order) {
        case AO_BOTTOM_UP: {
            for (auto it = topoSccOrder.rbegin(), ie = topoSccOrder.rend();
                    it != ie; ++it) {
                const SCC *scc = *it;
                setCurrentScc(scc);
                visitScc(*scc);
            }
            break;
        }
        case AO_TOP_DOWN: {
            for (auto it = topoSccOrder.begin(), ie = topoSccOrder.end();
                    it != ie; ++it) {
                const SCC *scc = *it;
                setCurrentScc(scc);
                visitScc(*scc);
            }
            break;
        }
        default:
            break;
        }
    }

    /// Release resources
    void release() {
        topoSccOrder.release();
    }

    /// Destructor
    virtual ~InterproceduralAnalysisBase() {
        release();
    }

    /// Some wrappers for topoSccOrder
    //@{
    inline PTACallGraph *getCallGraph() {
        return topoSccOrder.getCallGraph();
    }
    inline const PTACallGraph *getCallGraph() const {
        return topoSccOrder.getCallGraph();
    }
    inline bool isDeadFunction(const llvm::Function *F) const {
        return topoSccOrder.isDeadFunction(F);
    }
    inline const SCC *getScc(const llvm::Function *F) const {
        return topoSccOrder.getScc(F);
    }
    //@}

    /// Get the SCC's summary information.
    //@{
    inline SccSummary &getOrAddSccSummary(const SCC *scc) {
        return scc2SummaryMap[scc];
    }
    inline SccSummary &getOrAddSccSummary(const llvm::Function *F) {
        return scc2SummaryMap[getScc(F)];
    }
    inline const SccSummary *getSccSummary(const SCC* scc) const {
        typename Scc2SummaryMap::const_iterator it = scc2SummaryMap.find(scc);
        if (it != scc2SummaryMap.end())  return &it->second;
        return NULL;
    }
    inline const SccSummary *getSccSummary(const llvm::Function *F) const {
        return getSccSummary(getScc(F));
    }
    inline SccSummary *getSccSummary(const SCC* scc) {
        typename Scc2SummaryMap::iterator it = scc2SummaryMap.find(scc);
        if (it != scc2SummaryMap.end())  return &it->second;
        return NULL;
    }
    inline SccSummary *getSccSummary(const llvm::Function *F) {
        return getSccSummary(getScc(F));
    }
    //@}

protected:
    /// Basic SCC visitor
    void visitScc(const SCC &scc) {
        for (typename SCC::const_iterator it = scc.begin(), ie = scc.end(); it != ie;
                ++it) {
            const llvm::Function *F = *it;
            if (F->isDeclaration()) continue;
            static_cast<SubClass*>(this)->visitFunction(F);
        }
    }

    /// Get the SCC's information.
    //@{
    inline const SCC *getCurrentScc() const {
        return currentScc;
    }
    inline void setCurrentScc(const SCC *scc) {
        currentScc = scc;
    }
    inline const llvm::Function *getCurrentSccRep() const {
        return currentScc->getRep();
    }
    //@}

    /// Inter-procedural analysis order
    //@{
    inline AnalysisOrder getCurrentAnalysisOrder() const {
        return currentAnalysisOrder;
    }
    inline void setCurrentAnalysisOrder(const AnalysisOrder order) {
        currentAnalysisOrder = order;
    }
    //@}

    /// Function visitor to perform the detailed analysis.
    /// SubClass should overload this method.
    void visitFunction(const llvm::Function *F) {
    }

private:
    Scc2SummaryMap scc2SummaryMap;
    const SCC* currentScc;
    AnalysisOrder currentAnalysisOrder;
};



#endif /* INTERPROCEDURALANALYSIS_H_ */
