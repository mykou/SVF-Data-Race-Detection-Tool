/*
 * ContextSensitiveAliasAnalysis.h
 *
 *  Created on: 11/04/2016
 *      Author: dye
 */

#ifndef CONTEXTSENSITIVEALIASANALYSIS_H_
#define CONTEXTSENSITIVEALIASANALYSIS_H_

#include "CflReachabilityAnalysis.h"
#include <unordered_set>


/*!
 * The base class to represent a context, which consists of
 * a sequence of context items.
 * A context item, as specified by the template variable C,
 * can typically be a call site.
 */
template<typename C>
class CtxBase {
public:
    typedef typename std::vector<C>::iterator iterator;
    typedef typename std::vector<C>::const_iterator const_iterator;
    typedef typename std::vector<C>::reverse_iterator reverse_iterator;
    typedef typename std::vector<C>::const_reverse_iterator const_reverse_iterator;

    /// Iterators
    //@{
    inline iterator begin() {
        return context.begin();
    }
    inline iterator end() {
        return context.end();
    }
    inline const_iterator begin() const {
        return context.begin();
    }
    inline const_iterator end() const {
        return context.end();
    }
    inline reverse_iterator rbegin() {
        return context.rbegin();
    }
    inline reverse_iterator rend() {
        return context.rend();
    }
    inline const_reverse_iterator rbegin() const {
        return context.rbegin();
    }
    inline const_reverse_iterator rend() const {
        return context.rend();
    }
    //@}

    /// Detect if there's a cycle in the context
    /// after pushing "c" onto the stack.
    inline bool hasCycle(C c) const {
        for (int i = 0, e = context.size(); i != e; ++i) {
            if (context[i] == c)   return true;
        }
        return false;
    }

    /// Push context to the top of the stack
    inline void push(C c) {
        context.push_back(c);
    }

    /// Try popping matched context from the top of the stack.
    /// If the stack top matches the input, then pop the stack
    /// and return true; otherwise return false.
    inline bool pop(C c) {
        if (context.empty())   return true;
        if (context.back() == c) {
            context.pop_back();
            return true;
        }
        return false;
    }

    /// Unconditionally pop the stack.
    inline void pop() {
        if (!context.empty()) {
            context.pop_back();
        }
    }

    /// Get the top element
    inline const C top() const {
        return context.back();
    }

    /// Get the context size.
    inline size_t size() const {
        return context.size();
    }

    /// Reserve a size for the context.
    inline void reserve(size_t sz) {
        context.reserve(sz);
    }

    /// Check if the context is empty (epsilon).
    inline bool empty() const {
        return 0 == context.size();
    }

    /// Clear the context.
    inline void clear() {
        context.clear();
    }

    /// Get the i-th context item.
    inline const C &operator[](size_t i) const {
        return context[i];
    }

    /// Check if two contexts are equal or not.
    //@{
    inline bool operator==(const CtxBase<C> &ctx) const {
        return context == ctx.context;
    }
    inline bool operator!=(const CtxBase<C> &ctx) const {
        return !(*this == ctx);
    }
    //@}

    /// Check if two contexts are matched.
    /// If the shorter context equals the upper part
    /// of the longer one, then they are matched.
    inline bool matches(const CtxBase<C> &ctx) const {
        int e1 = context.size();
        int e2 = ctx.context.size();
        int e = std::min(e1, e2);
        for (int i = e; i != 0; --i) {
            if (context[e1 - i] != ctx.context[e2 - i]) {
                return false;
            }
        }
        return true;
    }

    /// Compute the hash value of the context.
    inline size_t hashValue() const {
        std::hash<size_t> hasher;
        size_t seed = 0;
        for (const C &c : context) {
            size_t i = (size_t) c;
            seed ^= hasher(i) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }
        return seed;
    }

    /// Dump the CtxBase information
    void print() const;

private:
    std::vector<C> context;
};



/*!
 * This class defines a special context, consisting of a
 * root and an abstract set (not sequence) of context items (call sites).
 *
 * The root context item (similar to the bottom of the stack) is
 * a spawn site, while the rest context items are abstracted by the
 * reachable code from the root, as indicated by MhpAnalysis.
 *
 * "push" operations do not modify the abstract context.
 *
 * When trying to do a "pop" operation:
 * (1) if the input context item matches the root, then
 * the entire abstract context is popped;
 * (2) if the input context item matches the abstract set
 * rather than the root, the abstract context remains unmodified.
 */
class AbstractCtx {
public:

    /// Constructor
    //@{
    AbstractCtx() : valid(false) {
    }
    AbstractCtx(const MhpAnalysis::ReachablePoint &rp) :
            rp(rp), valid(true) {
    }
    //@}

    /// Update the ReachablePoint.
    inline void setReachablePoint(const MhpAnalysis::ReachablePoint &rp) {
        this->rp = rp;
        valid = true;
    }

    /// Check if the given context item matches the root.
    //@{
    inline bool matchesRoot(const llvm::Instruction *I) {
        if (MhpAnalysis::REACHABLE_BRANCH == rp.getReachableType()) {
            return I == rp.getSpawnSite();
        }
        return false;
    }
    inline bool matchesRoot(OperationCollector::CsID csID) {
        const llvm::Instruction *I = OperationCollector::getInstance()->getCsInst(csID);
        return matchesRoot(I);
    }
    //@}

    /// Check if the given context item matches the abstract context set.
    //@{
    inline bool matchesRest(const llvm::Instruction *I) const {
        const llvm::Instruction *spawnSite = rp.getSpawnSite();
        MhpAnalysis::ReachableType t = rp.getReachableType();
        if (MhpAnalysis::REACHABLE_TRUNK == t) {
            return mhp->isTrunkReachable(spawnSite, I);
        } else {
            return mhp->isBranchReachable(spawnSite, I);
        }
    }
    inline bool matchesRest(OperationCollector::CsID csID) const {
        const llvm::Instruction *I = OperationCollector::getInstance()->getCsInst(csID);
        return matchesRest(I);
    }
    //@}

    /// Get the spawn site of the root context item.
    inline const llvm::Instruction *getSpawnSite() const {
        return rp.getSpawnSite();
    }

    /// Get the ReachableType from the spawn site.
    inline MhpAnalysis::ReachableType getReachableType() const {
        return rp.getReachableType();
    }

    /// Check if the context is valid/invalid.
    //@{
    inline bool isValid() const {
        return valid;
    }
    inline bool isInvalid() const {
        return !valid;
    }
    //@}

    /// Set the context to be valid/invalid.
    //@{
    inline void setValid() {
        valid = true;
    }
    inline void setInvalid() {
        valid = false;
    }
    //@}

    /// Initialize static members
    static void init(MhpAnalysis *mhp) {
        AbstractCtx::mhp = mhp;
    }


private:
    MhpAnalysis::ReachablePoint rp;
    bool valid;

    static MhpAnalysis *mhp;
};



/*!
 * This class defines a hybrid context.
 * It contains an AbstractCtx, and a ConcreteCtx of
 * type C as its context item.
 * The ConcreteCtx is at the top of the AbstractCtx
 * on stack.
 */
template<typename C>
class HybridCtx {
public:
    typedef CtxBase<C> ConcreteCtx;

    /// Constructors
    //@{
    HybridCtx<C>() {
    }

    HybridCtx<C>(AbstractCtx &aCtx) :
            abstractCtx(aCtx) {
    }

    HybridCtx<C>(const HybridCtx<C> &ctx) :
            concreteCtx(ctx.concreteCtx),
            abstractCtx(ctx.abstractCtx) {
    }
    //@}

    /// Detect if there's a cycle in the context
    /// after pushing "c" onto the stack.
    inline bool hasCycle(C c) const {
        return concreteCtx.hasCycle(c);
    }

    /// Push context to the top of the stack
    inline void push(C c) {
        concreteCtx.push(c);
    }

    /// Try popping matched context from the top of the stack.
    /// If the stack top matches the input, then pop the stack
    /// and return true; otherwise return false.
    inline bool pop(C c) {
        if (!concreteCtx.empty()) {
            return concreteCtx.pop(c);
        } else if (abstractCtx.isValid()) {
            if (abstractCtx.matchesRoot(c)) {
                abstractCtx.setInvalid();
                return true;
            } else {
                return abstractCtx.matchesRest(c);
            }
        }
        return false;
    }

    /// Clear the context.
    inline void clear() {
        concreteCtx.clear();
        abstractCtx.setInvalid();
    }

    /// Get the context size.
    inline size_t size() const {
        size_t sz = concreteCtx.size();
        if (abstractCtx.isValid()) {
            ++sz;
        }
        return sz;
    }

    /// Get the reference of ConcreteCtx/AbstractCtx.
    //@{
    inline const ConcreteCtx &getConcreteCtx() const {
        return concreteCtx;
    }
    inline const AbstractCtx &getAbstractCtx() const {
        return abstractCtx;
    }
    //@}

    /// Set the AbstractCtx
    inline void setAbstractCtx(AbstractCtx &aCtx) {
        abstractCtx = aCtx;
    }

    /// Check if two contexts are matched.
    /// If the shorter context equals the upper part
    /// of the longer one, then they are matched.
    bool matches(const HybridCtx<C> &ctx) const {
        int e1 = concreteCtx.size();
        int e2 = ctx.concreteCtx.size();

        // Compare the ConcreteCtx
        int e = std::min(e1, e2);
        for (int i = e; i != 0; --i) {
            if (concreteCtx[e1 - i] != ctx.concreteCtx[e2 - i]) {
                return false;
            }
        }

        // Compare the AbstractCtx
        if (e1 < e2) {
            return abstractCtx.isInvalid();
        } else {
            return ctx.abstractCtx.isInvalid();
        }
    }

    /// Dump the information of the context.
    void print() const {
        if (abstractCtx.isValid()) {
            const llvm::Instruction *spawnSite = abstractCtx.getSpawnSite();
            llvm::outs() << "Abstract ctx: " << rcUtil::getSourceLoc(spawnSite)
               << "\t" << abstractCtx.getReachableType() << "\n";
        } else {
            llvm::outs() << "Abstract ctx: NULL\n";
        }
        llvm::outs() << "Concrete ctx size: " << concreteCtx.size() << "\n";
    }

    /// Compute the hash value of the context.
    inline size_t hashValue() const {
        if (abstractCtx.isValid()) {
            return (size_t) (abstractCtx.getSpawnSite())
                    ^ concreteCtx.hashValue();
        }
        return concreteCtx.hashValue();
    }

    /// Define the operators
    //@{
    inline bool operator==(const HybridCtx<C> &hCtx) const {
        if (concreteCtx != hCtx.concreteCtx) {
            return false;
        }
        if (!abstractCtx.isValid() && !hCtx.abstractCtx.isValid()) {
            return true;
        }
        if (abstractCtx.isValid() && hCtx.abstractCtx.isValid()) {
            return abstractCtx.getReachableType()
                    == hCtx.abstractCtx.getReachableType()
                    && abstractCtx.getSpawnSite()
                            == hCtx.abstractCtx.getSpawnSite();
        }
        return false;
    }

    inline bool operator!=(const HybridCtx<C> &hCtx) const {
        return !((*this) == hCtx);
    }
    //@}

protected:
    ConcreteCtx concreteCtx;
    AbstractCtx abstractCtx;
};



/*!
 * The class of a context-sensitive points-to set,
 * used for the context-sensitive CFL-reachability-based
 * alias analysis.
 *
 * Conceptually, it maps node to a list of contexts,
 * under which the node would be pointed-to.
 * It also maintains a status of the points-to set,
 * indicating, for example, if the points-to set is
 * properly solved by the analyzer.
 *
 * This class also can be used to describe a context-sensitive
 * flow-to set.
 */
template<typename CtxImpl>
class CsPts {
public:
    /// Define the hash function for unordered_map.
    struct HashValue {
       size_t operator()(const CtxImpl& ctxImpl) const {
           return ctxImpl.hashValue();
       }
    };

    typedef CtxImpl Ctx;
    typedef typename std::unordered_set<CtxImpl, HashValue> CtxSet;
    typedef typename std::map<NodeID, CtxSet>::iterator iterator;
    typedef typename std::map<NodeID, CtxSet>::const_iterator const_iterator;

    /// Constructor
    CsPts<CtxImpl>() : status(initStatus) {
    }

    /// Iterators
    //@{
    inline iterator begin() {
        return pts.begin();
    }
    inline iterator end() {
        return pts.end();
    }
    inline const_iterator begin() const {
        return pts.begin();
    }
    inline const_iterator end() const {
        return pts.end();
    }
    //@}

    /// Get the number of nodes in the points-to set.
    inline size_t size() const {
        return pts.size();
    }

    /// Add context-sensitive points-to set
    //@{
    inline void addCsPts(NodeID o, const CtxImpl &ctx) {
        pts[o].insert(ctx);
    }
    inline void addCsPts(NodeID o, const CtxSet &ctxSet) {
        CtxSet &ctxSet_ = pts[o];
        ctxSet_.insert(ctxSet.begin(), ctxSet.end());
    }
    //@}

    /// Check if two CsPts are aliases
    bool alias(CsPts<CtxImpl> &pts) const {
        // The iterators of points-to sets follow the order by
        // the key (i.e., NodeID) of the map
        const_iterator it1 = begin();
        const_iterator ie1 = end();
        const_iterator it2 = pts.begin();
        const_iterator ie2 = pts.end();

        // Starting from the heads of the two pts, try to find out the
        // matched NodeIDs as well as the contexts.
        while (it1 != ie1 && it2 != ie2) {
            NodeID n1 = it1->first;
            NodeID n2 = it2->first;
            if (n1 < n2) {
                ++it1;
                continue;
            }
            if (n1 > n2) {
                ++it2;
                continue;
            }
            const CtxSet &ctxSet1 = it1++->second;
            const CtxSet &ctxSet2 = it2++->second;
            for (auto iit1 = ctxSet1.begin(), iie1 = ctxSet1.end();
                    iit1 != iie1; ++iit1) {
                for (auto iit2 = ctxSet2.begin(), iie2 = ctxSet2.end();
                        iit2 != iie2; ++iit2)  {
                    const auto &ctx1 = *iit1;
                    const auto &ctx2 = *iit2;
                   if (ctx1.matches(ctx2)) {
                       return true;
                   }
               }
           }
        }
        return false;
    }

    /// Dump the CsPts information.
    void print() const;

    /// Set the status of the CsPts.
    //@{
    inline void setStatus(int s) {
        status = s;
    }
    inline void setOutOfBudget() {
        status = outOfBudgetStatus;
    }
    inline void setInProcess() {
        status = inProcessStatus;
    }
    inline void setSolved() {
        status = solvedStatus;
    }
    inline void setOtherFailureStatus() {
        status = otherFailureStatus;
    }
    //@}

    /// Check the status of the CsPts.
    //@{
    inline int getStatus() const {
        return status;
    }
    inline bool isFresh() const {
        return status == initStatus;
    }
    inline bool isInProcess() const {
        return status == inProcessStatus;
    }
    inline bool isSolved() const {
        return status == solvedStatus;
    }
    inline bool isFailedToSolve() const {
        return status < initStatus;
    }
    inline bool isOutOfBudget() const {
         return status == outOfBudgetStatus;
    }
    //@}


private:
    std::map<NodeID, CtxSet> pts;
    int status;

    /// Status definitions
    //@{
    static constexpr int initStatus = 0;
    static constexpr int solvedStatus = 1;
    static constexpr int inProcessStatus = 2;
    static constexpr int outOfBudgetStatus = -1;
    static constexpr int otherFailureStatus = -2;
    //@}
};



/*!
 * \brief The base class of context-sensitive alias analysis.
 *
 * This class defines the interface to perform demand-driven
 * context-sensitive alias analysis.
 *
 * It is used to answer if two memory accesses may access aliases;
 * and if an access may access a memory partition.
 *
 * It also implements the relevant statistics.
 */
class ContextSensitiveAliasAnalysis {
public:
    typedef MemoryPartitioning::PartID PartID;
    typedef MemoryPartitioning::AccessID AccessID;

    /// Constructor
    ContextSensitiveAliasAnalysis() :
            refinedPairCount(0), unrefinedPairCount(0) {
    }

    /// Initialization
    virtual void init(RCMemoryPartitioning *mp, MhpAnalysis *mhp) = 0;

    /// Reset the analysis
    virtual void reset() = 0;

    /// Check if two memory accesses may/mustn't access aliases
    //@{
    bool mustNotAccessAliases(AccessID a1, AccessID a2);
    inline bool mayAccessAliases(AccessID a1, AccessID a2) {
        return !mustNotAccessAliases(a1, a2);
    }
    //@}

    /*!
     * Check if a memory access may/mustn't access a given memory partition.
     * @param accessId the memory access id
     * @param rp the ReachablePoint of the access
     * @param partId the memory partition id
     */
    //@{
    virtual bool mustNotAccessMemoryPartition(AccessID accessId,
            const MhpAnalysis::ReachablePoint &rp, PartID partId) = 0;
    inline bool mayAccessMemoryPartition(AccessID accessId,
            const MhpAnalysis::ReachablePoint &rp, PartID partId) {
        return !mustNotAccessMemoryPartition(accessId, rp, partId);
    }
    //@}

    /// Dump the context-sensitive alias analysis information
    virtual void print() const = 0;


protected:
    /// Destructor
    virtual ~ContextSensitiveAliasAnalysis() {
    }

    /// Check if two memory accesses must not access aliases
    virtual bool performRefinement(AccessID a1, AccessID a2) = 0;

    /// Statistics
    //@{
    inline void resetStat() {
        refinedPairCount = 0;
        unrefinedPairCount = 0;
    }
    inline void incrementRefinedPairCount() {
        ++refinedPairCount;
    }
    inline void incrementUnrefinedPairCount() {
        ++unrefinedPairCount;
    }
    inline int getRefinedPairCount() const {
        return refinedPairCount;
    }
    inline int getUnrefinedPairCount() const {
        return unrefinedPairCount;
    }
    inline int getTotalQueryPairCount() const {
        return refinedPairCount + unrefinedPairCount;
    }
    inline double getRefinementPercentage() const {
        return 100.0 * (double) refinedPairCount
                / (double) getTotalQueryPairCount();
    }
    void printStat() const;
    //@}


private:
    /// Statistics
    //@{
    int refinedPairCount;
    int unrefinedPairCount;
    //@}
};



/*!
 * \brief This class defines a hybrid context-based context-sensitive
 * alias analysis via CFL-based reachability analysis.
 *
 */
class HybridCtxBasedCSAA: public ContextSensitiveAliasAnalysis,
        public CflReachabilityAnalysis<
                CsPts<HybridCtx<OperationCollector::CsID> > > {
public:
    typedef CsPts<HybridCtx<OperationCollector::CsID> > Pts;
    typedef Pts::Ctx Ctx;
    typedef CflReachabilityAnalysis<Pts> CFL;

    /// Initialization
    virtual void init(RCMemoryPartitioning *mp, MhpAnalysis *mhp);

    /// Reset the analysis
    virtual void reset();

    /// Check if two memory accesses must not access aliases
    virtual bool mustNotAccessMemoryPartition(AccessID accessId,
            const MhpAnalysis::ReachablePoint &rp, PartID partId);

    /// Dump the analysis information
    virtual void print() const;


protected:
    /// Destructor
    virtual ~HybridCtxBasedCSAA() {
    }

    /// Get the context-sensitive access set of a given pointer
    /// from a memory access along with a ReachablePoint.
    const Pts &getCsAccessSet(AccessID id,
            const MhpAnalysis::ReachablePoint &rp);

    /// Check if two memory accesses must not access aliases
    virtual bool performRefinement(AccessID a1, AccessID a2);

};



class MhpPathFinder;

typedef CtxBase<OperationCollector::CsID> RC_CTX;

/*!
 * Standard context-based context-sensitive alias analysis
 * via CFL-based reachability analysis.
 */
class StandardCSAA: public ContextSensitiveAliasAnalysis,
        public CflReachabilityAnalysis<
                CsPts<RC_CTX> > {
public:
    typedef CsPts<RC_CTX> Pts;
    typedef Pts::Ctx Ctx;
    typedef CflReachabilityAnalysis<Pts> CFL;

    /// Constructor
    StandardCSAA() : mhpPathFinder(0) {
    }

    /// Initialization
    virtual void init(RCMemoryPartitioning *mp, MhpAnalysis *mhp);

    /// Reset the analysis
    virtual void reset();

    /// Check if two memory accesses must not access aliases
    virtual bool mustNotAccessMemoryPartition(AccessID accessId,
            const MhpAnalysis::ReachablePoint &rp, PartID partId);

    /// Dump the analysis information
    virtual void print() const;


protected:
    /// Check if two memory accesses must not access aliases
    virtual bool performRefinement(AccessID a1, AccessID a2);

    /// Destructor
    virtual ~StandardCSAA();

private:
    MhpPathFinder *mhpPathFinder;
};

#endif /* CONTEXTSENSITIVEALIASANALYSIS_H_ */
