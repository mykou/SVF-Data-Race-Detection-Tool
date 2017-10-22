/*
 * MhpAnalysis.h
 *
 *  Created on: 01/05/2015
 *      Author: dye
 */

#ifndef MHPANALYSIS_H_
#define MHPANALYSIS_H_

#include "InterproceduralAnalysis.h"

class ThreadCallGraph;

/*!
 * The SCC summary information of MhpAnalysis.
 */
class MhpSummary {
public:
    typedef CodeSet::InstSet InstSet;
    typedef std::map<const llvm::Instruction*, InstSet> Site2AffectedSitesMap;
    typedef Site2AffectedSitesMap::const_iterator const_iterator;
    typedef Site2AffectedSitesMap::iterator iterator;

    /// Iterators for spawn sites that have side effect to this SCC
    //@{
    inline const_iterator spawnBegin() const {
        return spawnSite2AffectedSitesMap.begin();
    }
    inline const_iterator spawnEnd() const {
        return spawnSite2AffectedSitesMap.end();
    }
    inline iterator spawnBegin() {
        return spawnSite2AffectedSitesMap.begin();
    }
    inline iterator spawnEnd() {
        return spawnSite2AffectedSitesMap.end();
    }
    //@}

    /*!
     * Add a call site Instruction which has a thread spawn effect
     * @param spawnSite the given thread spawn site
     * @param affectedSite the call site that has the side effect of spawnSite
     */
    inline void addAffectedSpawnSite(const llvm::Instruction *spawnSite,
            const llvm::Instruction *affectedSite) {
        spawnSite2AffectedSitesMap[spawnSite].insert(affectedSite);
    }

    /*!
     * Get the affected sites of a given thread spawn site.
     * @param spawnSite the input thread spawn site
     * @return a pointer to the set of the affected sites; NULL if there isn't any.
     */
    inline const InstSet *getAffectedSpawnSites(
            const llvm::Instruction *spawnSite) const {
        auto iter = spawnSite2AffectedSitesMap.find(spawnSite);
        if (iter == spawnSite2AffectedSitesMap.end())   return NULL;
        return &iter->second;
    }

    /// Iterators for join sites in this SCC
    //@{
    inline InstSet::const_iterator joinBegin() const {
        return joinSites.begin();
    }
    inline InstSet::const_iterator joinEnd() const {
        return joinSites.end();
    }
    inline InstSet::iterator joinBegin() {
        return joinSites.begin();
    }
    inline InstSet::iterator joinEnd() {
        return joinSites.end();
    }
    //@}

    /// Insert a join site
    inline void insertJoinSite(const llvm::Instruction *joinSite) {
        joinSites.insert(joinSite);
    }

private:
    /// Mapping from spawn site to all the callInsts
    /// which may have side effect of the spawn site in this SCC.
    Site2AffectedSitesMap spawnSite2AffectedSitesMap;

    /// Records of join sites in this SCC
    InstSet joinSites;
};


class PathRefinement;
class BarrierAnalysis;

/*!
 * Lightweight May-Happen-in-Parallel analysis.
 * This analysis is hybrid across the function-, basic block-,
 * and instruction-level granularities.
 * It only considers the "fork" operations (without handling the "join" operations).
 * The algorithm is context- and path-insensitive.
 */
class MhpAnalysis: public InterproceduralAnalysisBase<MhpAnalysis, MhpSummary> {
public:
    typedef InterproceduralAnalysisBase<MhpAnalysis, MhpSummary> SUPER;
    typedef CodeSet::FuncSet FuncSet;
    typedef CodeSet::BbSet BbSet;
    typedef CodeSet::InstSet InstSet;
    typedef CodeSet::ValSet ValSet;
    typedef std::map<const llvm::CallInst*, bool> ReachableSpawnSites;
    typedef std::map<const llvm::Function*, ReachableSpawnSites> FunctionReachableSpawnSites;

    friend SUPER;
    friend PathRefinement;

    /*!
     * The forward reachable code set from a spawn site.
     */
    class SpawnSiteReachableCodeSet {
    public:
        /// Constructor
        SpawnSiteReachableCodeSet() : spawnSite(NULL), recursive(false) {
        }

        /// Initializer
        inline void init(const llvm::Instruction *spawnSite) {
            this->spawnSite = spawnSite;
        }

        /// Get spawn site;
        inline const llvm::Instruction *getSpawnSite() const {
            return spawnSite;
        }

        /// Check if I is forward reachable from this->spawnSite
        //@{
        inline bool isTrunkReachable(const llvm::Instruction *I) const {
            return trunkReachable.covers(I);
        }
        inline bool isBranchReachable(const llvm::Instruction *I) const {
            return branchReachable.covers(I);
        }
        //@}

        /// Getter and Setter of this->recursive
        //@{
        inline bool isRecursive() const {
            return recursive;
        }
        inline void setRecursive(bool b) {
            recursive = b;
        }
        //@}

        /// Get the CodeSet
        //@{
        inline CodeSet &getTrunkReachableCodeSet() {
            return trunkReachable;
        }
        inline const CodeSet &getTrunkReachableCodeSet() const {
            return trunkReachable;
        }
        inline CodeSet &getBranchReachableCodeSet() {
            return branchReachable;
        }
        inline const CodeSet &getBranchReachableCodeSet() const {
            return branchReachable;
        }
        //@}

    private:
        /// Thread spawn site
        const llvm::Instruction *spawnSite;

        /// Whether the function of spawnSite is recursive
        bool recursive;

        /// Forward reachable from the trunk of spawnSite
        CodeSet trunkReachable;

        /// Forward reachable from the branch of spawnSite
        CodeSet branchReachable;
    };


    /// The types of code reachability from a spawn site
    enum ReachableType {
        REACHABLE_NOT = 0,      ///< Not reachable
        REACHABLE_TRUNK,        ///< Reachable from spawner
        REACHABLE_BRANCH,       ///< Reachable from spawnee
    };


    /*!
     * A reachable spawn site along with the type of reachability.
     */
    class ReachablePoint {
    public:
        /// Constructor
        //@{
        ReachablePoint() : spawnSite(NULL), type(REACHABLE_NOT) {
        }
        ReachablePoint(const llvm::Instruction *spawnSite, ReachableType type) :
                spawnSite(spawnSite), type(type) {
        }
        //@}

        /// Getter of this->spawnSite
        inline const llvm::Instruction *getSpawnSite() const {
            return spawnSite;
        }

        /// Getter of this->type
        inline ReachableType getReachableType() const {
            return type;
        }

        /// Operators
        //@{
        inline bool operator==(const ReachablePoint &rp) const {
            return spawnSite == rp.spawnSite && type == rp.type;
        }
        inline bool operator!=(const ReachablePoint &rp) const {
            return !((*this) == rp);
        }
        //@}

    private:
        const llvm::Instruction *spawnSite;
        ReachableType type;
    };

    /*!
     * To maintain a number of backward ReachablePoints.
     */
    class BackwardReachablePoints {
    public:
        typedef llvm::SmallVector<ReachablePoint, 4> ReachablePointVector;
        typedef ReachablePointVector::iterator iterator;
        typedef ReachablePointVector::const_iterator const_iterator;

        /**
         * Add a ReachablePoint
         * @param spawnSite the reachable spawn site
         * @param t the reachability type
         */
        inline void addReachablePoint(const llvm::Instruction *spawnSite,
                ReachableType t) {
            reachablePoints.push_back(ReachablePoint(spawnSite, t));
        }

        /// Union a bunch of ReachablePoints
        inline iterator insert(iterator pos, const_iterator first,
                const_iterator last) {
            return reachablePoints.insert(pos, first, last);
        }

        /// Dump the ReachablePoints information
        inline void dump() const {
            for (const_iterator it = begin(), ie = end(); it != ie; ++it) {
                const ReachablePoint &rp = *it;
                llvm::outs() << *rp.getSpawnSite() << "\n";
            }
        }

        /// Iterators over the ReachablePoints
        //@{
        inline const_iterator begin() const {
            return reachablePoints.begin();
        }
        inline const_iterator end() const {
            return reachablePoints.end();
        }
        inline iterator begin() {
            return reachablePoints.begin();
        }
        inline iterator end() {
            return reachablePoints.end();
        }
        //@}

        /// Get the number of ReachablePoints
        inline size_t size() const {
            return reachablePoints.size();
        }

    private:
        /// Records of all ReachablePoints
        ReachablePointVector reachablePoints;
    };

    typedef llvm::SmallVector<std::pair<ReachablePoint, ReachablePoint>, 4> ReachablePointPairs;


    /*!
     * Initialization
     * @param cg The callgraph.
     * @param tcg The thread callgraph.
     * @param pta The pointer analysis.
     */
    void init(PTACallGraph *cg, ThreadCallGraph *tcg, BVDataPTAImpl *pta);

    /*!
     * \brief Perform the may-happen-in-parallel analysis.
     * It contains two phases:
     * (1) a bottom-up phase, and
     * (2) a top-down phase.
     */
    void analyze();

    /// Perform refinement analysis
    void performRefinementAnalysis();

    /// Release
    void release();

    /// Destructor
    virtual ~MhpAnalysis() {
        release();
    }

    /// Interface to answer if two Instruction may happen in parallel.
    /// Return all the possible mhp spawn sites.
    /// Performance is low.
    InstSet mayHappenInParallel(const llvm::Instruction *I1,
            const llvm::Instruction *I2);

    /// Get the ReachablePoint pairs of two MHP Instructions
    ReachablePointPairs getReachablePointPairs(const llvm::Instruction *I1,
            const llvm::Instruction *I2);

    /// Check if the MHP relations between two given Instructions from a spawn
    /// site is refined (proven to be infeasible by refinement analysis' result).
    bool branchJoinRefined(const llvm::Instruction *spawnSite,
            const llvm::Instruction *I1,
            const llvm::Instruction *I2) const;

    /// Check if a given Instruction is sequential code,
    /// i.e., not backward reachable from any spawn site.
    inline bool isSequential(const llvm::Instruction *I) const {
        return backwardReachableInfo.isSequential(I);
    }

    /// Return the BackwardReachablePoints of a given Instruction.
    inline BackwardReachablePoints &getBackwardReachablePoints(
                    const llvm::Instruction *I) {
        return backwardReachableInfo.collectBackwardReachablePoints(I);
    }

    /// Get ThraedCallGraph
    inline ThreadCallGraph *getThreadCallGraph() const {
        return tcg;
    }

    /// Check the reachability of a given spawn site and an Instruction.
    //@{
    inline bool isTrunkReachable(const llvm::Instruction *spawnSite,
            const llvm::Instruction *I) const {
        return spawnSiteReachability.isTrunkReachable(spawnSite, I);
    }
    inline bool isBranchReachable(const llvm::Instruction *spawnSite,
            const llvm::Instruction *I) const {
        return spawnSiteReachability.isBranchReachable(spawnSite, I);
    }
    //@}

protected:
    /// Generic Function visitor.
    void visitFunction(const llvm::Function *F);

private:
    /*!
     * Records of reachability information of every spawn site.
     */
    class SpawnSiteReachability {
    public:
        typedef std::map<const llvm::Instruction*, SpawnSiteReachableCodeSet> SpawnSiteReachableCodeSetMap;
        typedef SpawnSiteReachableCodeSetMap::const_iterator const_iterator;
        typedef SpawnSiteReachableCodeSetMap::iterator iterator;

        /// Iterators
        //@{
        inline const_iterator begin() const {
            return spawnSiteReachableCodeSetMap.begin();
        }
        inline const_iterator end() const {
            return spawnSiteReachableCodeSetMap.end();
        }
        inline iterator begin() {
            return spawnSiteReachableCodeSetMap.begin();
        }
        inline iterator end() {
            return spawnSiteReachableCodeSetMap.end();
        }
        //@}

        /// Initialization of spawnSiteReachableCodeSetMap for a given spawn site.
        inline void addSpawnSite(const llvm::Instruction *spawnSite) {
            if (spawnSiteReachableCodeSetMap.find(spawnSite)
                    != spawnSiteReachableCodeSetMap.end()) {
                return;
            }
            spawnSiteReachableCodeSetMap[spawnSite].init(spawnSite);
        }

        /// Get the SpawnSiteReachableCodeSet of a given spawn site.
        //@{
        inline SpawnSiteReachableCodeSet &getReachableCodeSet(
                const llvm::Instruction *spawnSite) {
            auto it = spawnSiteReachableCodeSetMap.find(spawnSite);
            assert(it != spawnSiteReachableCodeSetMap.end()
                            && "spawnSiteReachableCodeSetMap must contain spawnSite!");
            return it->second;
        }
        inline const SpawnSiteReachableCodeSet &getReachableCodeSet(
                const llvm::Instruction *spawnSite) const {
            auto it = spawnSiteReachableCodeSetMap.find(spawnSite);
            assert(it != spawnSiteReachableCodeSetMap.end()
                            && "spawnSiteReachableCodeSetMap must contain spawnSite!");
            return it->second;
        }
        //@}

        /// Reachability check
        //@{
        inline bool isTrunkReachable(const llvm::Instruction *spawnSite,
                const llvm::Instruction *I) const {
            return getReachableCodeSet(spawnSite).isTrunkReachable(I);
        }
        inline bool isBranchReachable(const llvm::Instruction *spawnSite,
                const llvm::Instruction *I) const {
            return getReachableCodeSet(spawnSite).isBranchReachable(I);
        }
        //@}

    private:
        SpawnSiteReachableCodeSetMap spawnSiteReachableCodeSetMap;
    };



    /*!
     * To maintain the mapping from Functions, BasicBlocks, and Instructions
     * to their corresponding BackwardReachablePoints.
     */
    class BackwardReachableInfo {
    public:
        typedef std::map<const llvm::Instruction*, BackwardReachablePoints> Inst2Brp;
        typedef std::map<const llvm::BasicBlock*, BackwardReachablePoints> Bb2Brp;
        typedef std::map<const llvm::Function*, BackwardReachablePoints> Func2Brp;

        /// Check if a given Instruction is sequential code,
        /// i.e., not backward reachable from any spawn site.
        inline bool isSequential(const llvm::Instruction *I) const {
            const llvm::BasicBlock *BB = I->getParent();
            const llvm::Function *F = BB->getParent();
            if (funcReachable.count(F))     return false;
            if (bbReachable.count(BB))      return false;
            if (instReachable.count(I))     return false;
            return true;
        }

        /// Return the BackwardReachablePoints of a given Instruction.
        inline BackwardReachablePoints &collectBackwardReachablePoints(
                const llvm::Instruction *I) {
            Inst2Brp::iterator it = allInstReachable.find(I);
            if (it != allInstReachable.end())   return it->second;
            BackwardReachablePoints &brp = allInstReachable[I];
            computeBackwardReachablePoints(I, brp);
            return brp;
        }

        /// Compute the BackwardReachablePoints of a given Instruction.
        inline void computeBackwardReachablePoints(const llvm::Instruction *I,
                BackwardReachablePoints &brp) const {
            Inst2Brp::const_iterator it = instReachable.find(I);
            if (it != instReachable.end()) {
                brp.insert(brp.end(), it->second.begin(), it->second.end());
            }
            const llvm::BasicBlock *BB = I->getParent();
            Bb2Brp::const_iterator itt = bbReachable.find(BB);
            if (itt != bbReachable.end()) {
                brp.insert(brp.end(), itt->second.begin(), itt->second.end());
            }
            const llvm::Function *F = BB->getParent();
            Func2Brp::const_iterator ittt = funcReachable.find(F);
            if (ittt != funcReachable.end()) {
                brp.insert(brp.end(), ittt->second.begin(), ittt->second.end());
            }
        }

        /// Access to BackwardReachablePoints
        //@{
        inline BackwardReachablePoints &getOrAddBackwardReachablePoints(
                const llvm::Function *F) {
            return funcReachable[F];
        }
        inline BackwardReachablePoints &getOrAddBackwardReachablePoints(
                const llvm::BasicBlock *BB) {
            return bbReachable[BB];
        }
        inline BackwardReachablePoints &getOrAddBackwardReachablePoints(
                const llvm::Instruction *I) {
            return instReachable[I];
        }
        //@}

    private:
        Func2Brp funcReachable;     ///< Reachable Functions
        Bb2Brp bbReachable;         ///< Reachable BasicBlocks
        Inst2Brp instReachable;     ///< Reachable Instructions
        Inst2Brp allInstReachable;  ///< All Instruction to ReachablePoint mapping
    };


    /*!
     * The loop bound condition.
     */
    class BoundCond {
    public:
        /// Constructor
        BoundCond() :
                CI(0), indVar(0), bound(0), pred(llvm::CmpInst::BAD_ICMP_PREDICATE) {
        }

        /// Reset this instance with given data
        inline void reset(const llvm::CmpInst *CI, const llvm::Value *indVar,
                const llvm::Value *bound, llvm::CmpInst::Predicate pred) {
            this->CI = CI;
            this->indVar = indVar;
            this->bound = bound;
            this->pred = pred;
        }

        /// Get the CmpInst
        const llvm::CmpInst *getCmpInst() const {
            return CI;
        }

        /// Check if BoundCond is valid
        inline bool isValid() const {
            return CI != 0;
        }

        /// Check if this BoundCond equals a given one
        inline bool operator==(const BoundCond &bc) const {
            if (!isValid() || !bc.isValid())    return false;
            if (!rcUtil::areSignInsensitivelyEqualIcmpPredicate(pred, bc.pred))
                return false;
            if (!equalBounds(bound, bc.bound))  return false;
            return true;
        }

        /// Check if this BoundCond is not equal to a given one
        inline bool operator!=(const BoundCond &bc) const {
            return !((*this) == bc);
        }

        /// Print the BoundCond information
        inline void print() const;

    private:
        /// Check if two given bounds are equal
        bool equalBounds(const llvm::Value *b1, const llvm::Value *b2) const {
            // Strip the casts.
            b1 = rcUtil::stripAllCasts(b1);
            b2 = rcUtil::stripAllCasts(b2);

            // If there are identical, then return true.
            if (b1 == b2)   return true;

            // Do more thorough examination here.
            const llvm::Instruction *I1 = llvm::dyn_cast<llvm::Instruction>(b1);
            const llvm::Instruction *I2 = llvm::dyn_cast<llvm::Instruction>(b2);

            // We only handle Instructions at this moment.
            if (!I1 || !I2)     return false;

            // Skip the Instructions with different same opcodes.
            if (I1->getOpcode() != I2->getOpcode())     return false;

            // Compare all operands
            for (int i = 0, e = I1->getNumOperands(); i != e; ++i) {
                const llvm::Value *opnd1 = I1->getOperand(i);
                const llvm::Value *opnd2 = I2->getOperand(i);
                if (!equalBounds(opnd1, opnd2))  return false;
            }
            return true;
        }

        const llvm::CmpInst *CI;    ///< the source CmpInst
        const llvm::Value *indVar;  ///< the canonical loop induction variable (lhs)
        const llvm::Value *bound;   ///< the loop bounds (rhs)
        llvm::CmpInst::Predicate pred; ///< predicate of lhs and rhs
    };

    /*!
     * Information of a spawn/join site.
     */
    class SpawnJoinSiteInfo {
    public:
        /// Constructor
        SpawnJoinSiteInfo(MhpAnalysis *mhp, const llvm::Instruction *site,
                bool isSpawnSite) :
                mhp(mhp), site(site), matchingSite(0), isSpawnSite(isSpawnSite),
                tidPtr(0), tidPtrBase(0), L(0), canonicalIndVar(0),
                exitingBb(0), exitBb(0), offsetStart(0), offsetStride(0) {
        }

        /// Compute the information of a spawn/join site.
        void run();

        /// Compute the information of this spawn/join site.
        void print() const;

        /*!
         * Check if a given SpawnJoinSiteInfo matches this in terms of:
         * (1) equal loop BoundCond;
         * (2) same access pattern of tid pointers in loop.
         */
        bool match(const SpawnJoinSiteInfo *siteInfo) const;

        /// Class member accesses
        //@{
        inline const llvm::Instruction *getMatchingSpawnSite() const {
            assert(!isSpawnSite && "This must be a join site.");
            return matchingSite;
        }
        inline void setMatchingSpawnSite(const llvm::Instruction *spawnSite) {
            assert(!isSpawnSite && "This must be a join site.");
            matchingSite = spawnSite;
        }
        inline const llvm::Instruction *getSite() const {
            return site;
        }
        inline const llvm::Value *getTidPointer() const {
            return tidPtr;
        }
        inline const llvm::BasicBlock *getExitBasicBlock() const {
            return exitBb;
        }
        inline const llvm::Loop *getLoop() const {
            return L;
        }
        //@}

    private:
        /// Get the unique exiting and exit BasicBlocks
        /// if this spawn/join site is in a loop.
        std::pair<const llvm::BasicBlock*, const llvm::BasicBlock*>
        getUniqueExitBasicBlock();

        /// Check if a BasicBlock executes backedgeTakenCount times in a loop.
        static bool executeBackedgeTakenCountTimes(
                const llvm::BasicBlock *BB, const llvm::Loop *L);

        /// Get the base pointer from any GEP.
        static llvm::Value *getBasePtr(llvm::Value *v);

        /// Compute a SCEV that represents the subtraction of two given SCEVs.
        static const llvm::SCEV *getSCEVMinusExpr(const llvm::SCEV *s1,
                const llvm::SCEV *s2, llvm::ScalarEvolution *SE);

        /// MhpAnalysis
        MhpAnalysis *mhp;

        /// Spawn or join site
        const llvm::Instruction *site;

        /// The matching spawn site if this is a joinSiteInfo
        const llvm::Instruction *matchingSite;

        /// If this is a spawn site.
        bool isSpawnSite;

        /// Pointers of the tid object(s)
        //@{
        llvm::Value *tidPtr;
        llvm::Value *tidPtrBase;
        //@}

        /// Loop related information if this spawn/join site is in a loop
        //@{
        const llvm::Loop *L;
        const llvm::PHINode *canonicalIndVar;
        const llvm::BasicBlock *exitingBb;
        const llvm::BasicBlock *exitBb;
        const llvm::Value *offsetStart;
        const llvm::Value *offsetStride;
        BoundCond bc;
        //@}
    };


    /*!
     * Thread Join Analysis to identify the corresponding join Instruction
     * (BasicBlocks) that join the thread(s) created by "spawnSite".
     */
    class ThreadJoinAnalysis {
    public:
        typedef std::map<const llvm::Function*, ValSet> JoinInstsOrBbsMap;

        /*!
         * Record of a CodeSet that is blocked (by some join operations)
         * in the reachability from a spawn site in MhpAnalysis.
         */
        class BlockingCodeInfo {
        public:
            /*!
             * Reset BlockingCodeInfo to identify the blocked CodeSet with blockers.
             * @param blockers it contains the join site Instructions
             * or BasicBlocks that finish the thread join effect in a loop.
             */
            void reset(const ValSet &blockers);

            /// Check if a given BasicBlock contains any blocking Instruction
            inline bool isBlockingBasicBlock(const llvm::BasicBlock *BB) const {
                return blockingBbs.count(BB);
            }

            /// Check if a given Value is blocked by this BlockingCodeInfo.
            template<typename T>
            inline bool isBlocked(T v) const {
                return blockedCode.covers(v);
            }

            /// Check if the blocked CodeSet is empty.
            inline bool empty() const {
                return blockedCode.empty();
            }

            /// Dummy BlockingCodeInfo
            static BlockingCodeInfo emptyBlockingCodeInfo;

        private:
            /*!
             * Identify the CodeSet blocked by a given Value intra-procedurally.
             * @param v it can be either an Instruction or a BasicBlock
             */
            void resolveBlocker(const llvm::Value *v);

            /// CodeSet that must be blocked
            CodeSet blockedCode;

            /// BasicBlocks that contains a blocking Instruction
            BbSet blockingBbs;
        };

        /// Constructor
        ThreadJoinAnalysis(MhpAnalysis *mhp, const llvm::Instruction *spawnSite) :
                mhp(mhp),
                spawnSite(spawnSite),
                F(spawnSite->getParent()->getParent()),
                spawnSiteInfo(mhp->spawnSiteInfoMap[spawnSite]),
                SE(mhp->getSe(F)),
                LI(mhp->getLi(F)) {
        }

        /// Identify the corresponding must-join Instructions or BasicBlocks.
        void run();

        /// Return the set of Instructions (BasicBlocks) in a given
        /// Function that join the thread(s) created by "this->spawnSite".
        inline const ValSet &getJoinInstsOrBbs(const llvm::Function *F) {
            return joinInstsOrBbsMap[F];
        }

        /// Get the spawn site of the analysis.
        inline const llvm::Instruction *getSpawnSite() const {
            return spawnSite;
        }

        /// Check if a given Value is blocked.
        template<typename T>
        inline bool isBlocked(T v) const {
            for (auto it = blockingCodeInfoMap.begin(), ie =
                    blockingCodeInfoMap.end(); it != ie; ++it) {
                if (it->second.isBlocked(v))    return true;
            }
            return false;
        }

        /// Get tje BlockingCodeInfo of a given Function.
        inline const BlockingCodeInfo &getBlockingCodeInfo(
                const llvm::Function *F) const {
            auto it = blockingCodeInfoMap.find(F);
            if (it != blockingCodeInfoMap.end())    return it->second;
            return BlockingCodeInfo::emptyBlockingCodeInfo;
        }

        /// Get the BasicBlock of the failure branch of "this->spawnSite".
        /// Return NULL if it does not find any.
        static const llvm::BasicBlock *getSpawnFailureBranch(
                const llvm::Instruction *spawnSite);

    private:
        /// Compute the BlockingCodeInfo for each involved Functions.
        void initBlockingCodeInfo();

        /// Get the JoinInstsOrBbsMap.
        inline const JoinInstsOrBbsMap &getJoinInstsOrBbsMap() const {
            return joinInstsOrBbsMap;
        }

        /*!
         * Check if "joinSite" must join the threads created in
         * "this->spawnSite" in loops.
         * If so, return the Exiting BasicBlock of the loop where
         * "joinSite" is located; otherwise return NULL.
         */
        const llvm::BasicBlock *handleLoopJoin(
                const llvm::Instruction *joinSite);

        /// Check if the two SCEVExprs are two SCEVAddRecExpr with
        /// equivalent starts and step recurrences according to "SE".
        bool haveSameStartAndStep(const llvm::SCEV *se1,
                const llvm::SCEV *se2) const;

        /// Check if the SCEVExprs computed by SCEV for spawn and join sites
        /// have the same execution count.
        bool haveSameExecutionCount(const llvm::SCEV *spawnSiteExecutionCount,
                const llvm::SCEV *joinSiteExecutionCount,
                const llvm::Loop *spawnSiteLoop);

        /// Check if "BB" executes for all the iterations of loop "L".
        bool executeBackedgeTakenCountTimes(const llvm::BasicBlock *BB,
                llvm::Loop *L) const;

        /*!
         * Get the execution count of a BasicBlock in a loop.
         * @param BB the BasicBlock for execution count
         * @param L the loop
         * @param exact If true, return the exact count if possible;
         * otherwise, return the conservative count (ignoring the loop exit
         * caused by the failure of thread spawn).
         * @return the number of the execution count of BB in L
         */
        const llvm::SCEV *getExecutionCount(const llvm::BasicBlock *BB,
                llvm::Loop *L, bool exact);

        /// Get the known start value of a given SCEV.
        const llvm::SCEV *getStart(const llvm::SCEV *scev) const;

        MhpAnalysis *mhp;
        const llvm::Instruction *spawnSite;
        const llvm::Function *F;
        const SpawnJoinSiteInfo *spawnSiteInfo;
        llvm::ScalarEvolution &SE;
        llvm::LoopInfo *LI;
        JoinInstsOrBbsMap joinInstsOrBbsMap;
        /// Mapping from Function to BlockingCodeInfo
        std::map<const llvm::Function*, BlockingCodeInfo> blockingCodeInfoMap;
        /// SCEVs of PHINodes that start from a zero value
        std::set<const llvm::SCEV*> startFromZero;
    };


    /*!
     * Reachability analysis for spawn/join effects.
     * This is used in the top-down phase of MhpAnalysis
     */
    class ReachabilityAnalysis {
    public:
        typedef ThreadJoinAnalysis::BlockingCodeInfo BlockingCodeInfo;

        /// Constructor
        ReachabilityAnalysis(CodeSet &trunkCode, CodeSet &branchCode,
                ThreadJoinAnalysis *threadJoinAnalysis) :
                trunkReachable(trunkCode), branchReachable(branchCode),
                threadJoinAnalysis(threadJoinAnalysis) {
        }

        /*!
         * Perform reachability analysis for trunk reachable code
         */
        void solveTrunkReachability(const InstSet &startingPoints,
                const SCC &scc, const ThreadCallGraph *tcg,
                const llvm::Instruction *excludedSpawnSite = NULL);

        /*!
         * Perform reachability analysis for branch reachable code
         * for refinement analysis
         */
        void solveBranchReachability(const llvm::Instruction *spawnSite,
                const ThreadCallGraph *tcg,
                const ThreadJoinAnalysis *threadJoinAnalysis);

        /*!
         * Perform reachability analysis for branch reachable code
         * for regular analysis
         */
        static void solveBranchReachability(const llvm::Instruction *spawnSite,
                const ThreadCallGraph *tcg, CodeSet &branchReachable);

        /*!
         * Identify the forward reachable Functions from a set of starting Functions
         * @param start the Functions to start traversal
         * @param reachable the output to record the results of reachable Functions
         * @param cg the call graph used for traversal
         */
        static void identifyReachableFunctions(const FuncSet &start,
                CodeSet &reachable, const ThreadCallGraph *cg);

        /*!
         * Identify the forward reachable code from a given Instruction.
         * @param I the Instruction to start traversal (with itself included).
         * @param blockingCodeInfo the BlockingCodeInfo under which the traversal stops
         * @param reachable the output to record the reachable code.
         */
        static void identifyReachableInstructions(const llvm::Instruction *I,
                const BlockingCodeInfo &blockingCodeInfo, CodeSet &reachable);

        /*!
         * Perform intra-procedural reachability analysis.
         * @param rootInst the root Instruction to start traversal;
         * it is excluded before the traversal happens,
         * but it may be included later if there is a cycle.
         * @param blockingCodeInfo the BlockingCodeInfo under which the traversal stops
         * @param reachable the output to record the results of reachable code.
         */
        static void identifyReachableInstsAndBBs(const llvm::Instruction *rootInst,
                const BlockingCodeInfo &blockingCodeInfo, CodeSet &reachable);

    private:
        /// CodeSet
        CodeSet &trunkReachable;
        CodeSet &branchReachable;

        /// Trunk reachable Functions that contains blocking Instructions to the spawn site
        FuncSet trunkPartialReachableFuncs;

        /// ThreadJoinAnalysis and its property references
        ThreadJoinAnalysis *threadJoinAnalysis;
    };

    typedef std::map<const llvm::Instruction*, ThreadJoinAnalysis*> ThreadJoinAnalysisMap;
    typedef std::map<const llvm::Instruction*, SpawnJoinSiteInfo*> SpawnJoinSiteInfoMap;
    typedef std::map<const llvm::Instruction*,
            std::map<const llvm::Instruction*, SpawnSiteReachableCodeSet> > RefinedSpawnSiteReachableCodeSetMap;
    typedef std::map<const llvm::Instruction*, CodeSet> CodeSetMap;

    /// Initialize refinement analysis.
    void initRefinementAnalysis();

    /// Perform parallel-for MHP analysis
    void performParForAnalysis();

    /// Identify concurrent staffs.
    //@{
    void identifySpawnJoinSites();
    void identifyReachable(const SCC *scc, const llvm::Instruction *spawnSite,
            const MhpSummary::InstSet &sideEffectSites);
    //@}

    /// Check if "F" may have side effect of "spawnSite".
    bool mayHaveSideEffect(const llvm::Instruction *spawnSite,
            const llvm::Function *F);

    /*!
     * Compute the backward reachability information that tells
     * which spawn site(s) can be backwards reached from a given
     * Instruction/BasicBlock/Function.
     */
    void computeBackwardReachableInfo();

    /// Inter-procedural analysis for each Function
    //@{
    void bottomUpFunctionVisitor(const llvm::Function *F);
    void topDownFunctionVisitor(const llvm::Function *F);
    //@}

    /**
     * Add a BackwardReachablePoint into the "backwardReachableInfo" record.
     * @param value the value to be added;
     * it can be Function*, BasicBlock*, or Instruction*.
     * @param spawnSite the reachable spawn site
     * @param t the reachable type
     */
    template<typename ValueType>
    inline void addReachablePoint(ValueType value,
            const llvm::Instruction *spawnSite, ReachableType t) {
        BackwardReachablePoints &brp =
                backwardReachableInfo.getOrAddBackwardReachablePoints(value);
        brp.addReachablePoint(spawnSite, t);
    }

    /// Determine if a reachable pair (with reachable types t1 and t2)
    /// may happen in parallel.
    static bool mhpForReachablePair(ReachableType t1, ReachableType t2);

    /// Check if a given input Instruction is an interested one.
    static inline bool isInterstedInst(const llvm::Instruction *I) {
        return llvm::isa<llvm::LoadInst>(I) || llvm::isa<llvm::StoreInst>(I)
                || llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I);
    }


    ThreadCallGraph *tcg;
    SpawnSiteReachability spawnSiteReachability;
    BackwardReachableInfo backwardReachableInfo;
    ThreadJoinAnalysisMap threadJoinAnalysisMap;
    SpawnJoinSiteInfoMap spawnSiteInfoMap;
    SpawnJoinSiteInfoMap joinSiteInfoMap;
    CodeSetMap parForReachabilityMap;

    /// Class members for refinement analysis
    //@{
    bool refinementMode;
    RefinedSpawnSiteReachableCodeSetMap refinedSpawnSiteReachableCodeSetMap;
    //@}
};


#endif /* MHPANALYSIS_H_ */
