/*
 * CflReachabilityAnalysis.h
 *
 *  Created on: 08/02/2016
 *      Author: dye
 */

#ifndef CFLREACHABILITYANALYSIS_H_
#define CFLREACHABILITYANALYSIS_H_

#include "MemoryPartitioning.h"
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <iomanip>


#define QUERY_BUDGET 1000

/*!
 * Class to perform context-sensitive CFL-reachability
 * based pointer analysis.
 */
template<typename Pts>
class CflReachabilityAnalysis {
public:
    typedef Pts Fts;
    typedef typename Pts::Ctx Ctx;
    typedef typename Pts::CtxSet CtxSet;
    typedef llvm::SmallSet<NodeID, 16> NodeSet;

    /// Constructor
    CflReachabilityAnalysis() :
            pta(NULL), pag(NULL), mp(NULL), mhp(NULL),
            dbg(false), worklistBased(true), useCache(true),
            budget(0), oc(OperationCollector::getInstance()) {
    }

    /// Destructor
    virtual ~CflReachabilityAnalysis() {
    }

    /// Initialization
    void init(RCMemoryPartitioning *mp, MhpAnalysis *mhp);

    /// Reset the analysis
    void reset();

    /// Dump CflReachability information
    void print() const;


protected:
    /*!
     * The class of the cache statistics.
     */
    class CacheStat {
    public:
        /// Constructor
        CacheStat() :
                cacheHitCount(0), cacheMissCount(0) {
        }

        /// Get cache statistics
        //@{
        inline int getCacheHitCount() const {
            return cacheHitCount;
        }
        inline int getCacheMissCount() const {
            return cacheMissCount;
        }
        inline int getTotalCacheReadCount() const {
            return cacheHitCount + cacheMissCount;
        }
        inline double getCacheHitPercentage() const {
            return 100.0 * (double) getCacheHitCount()
                    / (double) getTotalCacheReadCount();
        }
        inline double getCacheMissPercantage() const {
            return 100.0 * (double) getCacheMissCount()
                    / (double) getTotalCacheReadCount();
        }
        //@}

        /// Reset the statistics
        inline void reset() {
            cacheHitCount = 0;
            cacheMissCount = 0;
        }

    protected:
        /// Cache statistics operations
        //@{
        inline void incrementCacheHitCount() {
            ++cacheHitCount;
        }
        inline void incrementCacheMissCount() {
            ++cacheMissCount;
        }
        //@}

    private:
        /// Statistics
        //@{
        int cacheHitCount;
        int cacheMissCount;
        //@}
    };


    /*!
     * Cache for Pts.
     */
    class PtsCache : public CacheStat {
    public:
        /// Get the Pts from cache,
        /// Return a fresh Pts object if the cache missed.
        inline Pts &getPts(NodeID n, const Ctx &ctx) {
            static Query query;
            query.reset(n, ctx);
            Pts &ret = cache[query];

            // Set statistics
            if (ret.isFresh()) {
                CacheStat::incrementCacheMissCount();
            } else {
                CacheStat::incrementCacheHitCount();
            }

            return ret;
        }

        /// Get the size of the cache.
        inline size_t size() const {
            return cache.size();
        }

        /// Reset the cache
        inline void reset() {
            cache.clear();
            CacheStat::reset();
        }

        /// Print cache information
        inline void print() const {
            // Set format
            std::cout << std::fixed << std::setprecision(2);

            // Print the statistics
            std::cout << " == Pts Cache ==\n";
            std::cout << " size: " << size() << "\n";

            std::cout << " hit:  " << CacheStat::getCacheHitCount()
                    << " / " << CacheStat::getTotalCacheReadCount();
            if (CacheStat::getTotalCacheReadCount()) {
                std::cout << " (" << CacheStat::getCacheHitPercentage() << "%)";
            }
            std::cout << "\n";

            std::cout << " miss: " << CacheStat::getCacheMissCount()
                    << " / " << CacheStat::getTotalCacheReadCount();
            if (CacheStat::getTotalCacheReadCount()) {
                std::cout << " (" << CacheStat::getCacheMissPercantage() << "%)";
            }
            std::cout << "\n";
        }

    private:
        /*!
         * A Pts query contains two parts:
         * (1) a pointer node, and
         * (2) a Ctx.
         */
        struct Query {
            /// Reset the Pts query.
            inline void reset(NodeID n, const Ctx &ctx) {
                this->n = n;
                this->ctx = ctx;
            }

            /// Compute the hash value of the Pts query.
            inline size_t hashValue() const {
                return (size_t)(n) ^ ctx.hashValue();
            }

            /// Check if two Pts queries are equal.
            inline bool operator==(const Query &query) const {
                return n == query.n && ctx == query.ctx;
            }

            /// Dump the query information
            inline void print() const {
                llvm::outs() << "Query for points-to set: \n";
                llvm::outs() << " - Node: #" << n << "\t";
                llvm::outs() << rcUtil::getSourceLoc(n) << "\n";
                llvm::outs() << " - Context:\n";
                ctx.print();
            }

        private:
            NodeID n;
            Ctx ctx;
        };

        /// Define the hash function for unordered_map.
        struct HashValue {
           size_t operator()(const Query& query) const {
               return query.hashValue();
           }
        };

        typedef std::unordered_map<Query, Pts, HashValue> Query2PtsMap;
        Query2PtsMap cache;
    };


    /*!
     * Cache for Fts.
     */
    class FtsCache : public CacheStat {
    public:
        /// Get the Fts from cache,
        /// Return a fresh Fts object if the cache missed.
        inline Fts &getFts(NodeID n, int offset, const Ctx &ctx) {
            static Query query;
            query.reset(n, offset, ctx);
            Fts &ret = cache[query];

            // Set statistics
            if (ret.isFresh()) {
                CacheStat::incrementCacheMissCount();
            } else {
                CacheStat::incrementCacheHitCount();
            }

            return ret;
        }

        /// Get the size of the cache.
        inline size_t size() const {
            return cache.size();
        }

        /// Reset the cache
        inline void reset() {
            cache.clear();
            CacheStat::reset();
        }

        /// Print cache information
        inline void print() const {
            // Set format
            std::cout << std::fixed << std::setprecision(2);

            // Print the statistics
            std::cout << " == Fts Cache ==\n";
            std::cout << " size: " << size() << "\n";

            std::cout << " hit:  " << CacheStat::getCacheHitCount()
                    << " / " << CacheStat::getTotalCacheReadCount();
            if (CacheStat::getTotalCacheReadCount()) {
                std::cout << " (" << CacheStat::getCacheHitPercentage() << "%)";
            }
            std::cout << "\n";

            std::cout << " miss: " << CacheStat::getCacheMissCount()
                    << " / " << CacheStat::getTotalCacheReadCount();
            if (CacheStat::getTotalCacheReadCount()) {
                std::cout << " (" << CacheStat::getCacheMissPercantage() << "%)";
            }
            std::cout << "\n";
        }

    private:
        /*!
         * A Fts query contains three parts:
         * (1) a field-entry object node,
         * (2) an offset from the field-entry object node, and
         * (3) a Ctx.
         */
        struct Query {
            /// Reset the Fts query.
            inline void reset(NodeID n, int offset, const Ctx &ctx) {
                this->n = n;
                this->offset = offset;
                this->ctx = ctx;
            }

            /// Compute the hash value of the Fts query.
            inline size_t hashValue() const {
                return (size_t)n ^ (size_t)offset ^ ctx.hashValue();
            }

            /// Check if two Fts queries are equal.
            inline bool operator==(const Query &query) const {
                return n == query.n && ctx == query.ctx;
            }

            /// Dump the query information
            inline void print() const {
                llvm::outs() << "Query for flows-to set: \n";
                NodeID o = n;
                if (offset >= 0) {
                    PAG *pag = PAG::getPAG();
                    LocationSet ls(offset);
                    o = pag->getGepObjNode(n, ls);
                }
                llvm::outs() << " - Node: #" << o << "\t";
                llvm::outs() << rcUtil::getSourceLoc(o) << "\n";
                llvm::outs() << " - Context:\n";
                ctx.print();
            }

        private:
            NodeID n;
            int offset;
            Ctx ctx;
        };

        /// Define the hash function for unordered_map.
        struct HashValue {
           size_t operator()(const Query& query) const {
               return query.hashValue();
           }
        };

        typedef std::unordered_map<Query, Fts, HashValue> Query2FtsMap;
        Query2FtsMap cache;
    };


    class CtxPool {
    public:
        inline const Ctx *getOrAdd(const Ctx &ctx) {
            typename CtxSet::iterator iter = ctxSet.insert(ctx).first;
            return &*iter;
        }
    private:
        CtxSet ctxSet;
    };

    typedef struct Work {
        Work(NodeID n, const Ctx *ctx) :
                n(n), ctx(ctx) {
        }

        inline bool operator==(const Work &w) const {
            return n == w.n && ctx == w.ctx;
        }
        inline size_t hashValue() const {
            return (size_t) n ^ ctx->hashValue();
        }
        NodeID n;
        const Ctx *ctx;
    } Work;

    struct HashValue {
       size_t operator()(const Work& w) const {
           return w.hashValue();
       }
    };

    typedef std::unordered_set<Work, HashValue> WorkSet;
    typedef struct {
        NodeID root;
        WorkSet ptsVisited;
        WorkSet ftsVisited;
    } VisitedWorkSets;



    /*!
     * Get the context-sensitive points-to set of a given query.
     * @param n the node of the input query
     * @param ctx the context of the input query
     * @param pts the output points-to set
     * @return whether the query is solved successfully
     */
    bool getPts(NodeID n, Ctx &ctx, Pts &pts);

    /*!
     * Worklist-based implementation to compute the context-sensitive
     * points-to set of a given query.
     * @param x the node of the input query
     * @param ctx the context of the input query
     * @param pts the output points-to set
     * @param visitedWorkSets the visited Work sets of pts and fts
     * @return whether the query is solved successfully
     */
    bool pointsToWorklistBased(NodeID x, const Ctx &ctx, Pts &pts,
            VisitedWorkSets &visitedWorkSets);

    /*!
     * Worklist-based implementation to compute the context-sensitive
     * flows-to set of a given query.
     * @param o the field-entry object node of the input query
     * @param offset the field offset of the input query
     * @param ctx the context of the input query
     * @param pts the output flows-to set
     * @param visitedWorkSets the visited Work sets of pts and fts
     * @return whether the query is solved successfully
     */
    bool flowsToWorklistBased(NodeID o, int offset, const Ctx &ctx, Fts &fts,
            VisitedWorkSets &visitedWorkSets);

    /*!
     * Compute the pointers, who point-to aliases context-sensitively
     * with a given query.
     * @param p the pointer of the input query
     * @param ctx the context of the input query
     * @param aliasPointers the output set of the alias pointers and contexts
     * @return whether the query is solved successfully
     */
    //@{
    bool getAliasPointers(NodeID p, const Ctx &ctx, Pts &aliasPointers);
    bool getAliasPointersImpl(NodeID p, const Ctx &ctx, Pts &aliasPointers);
    //@}

    /// Include the struct base object of any/first fields from
    /// the input pts into itself.
    //@{
    void includeFIObjForAnyField(Pts &pts) const;
    void includeFIObjForFirstField(Pts &pts) const;
    //@}


    /// ###### Deprecated! ######
    /// Recursion-based points-to and flows-to computation.
    //@{
    /*!
     * Recursion-based implementation to compute the context-sensitive
     * points-to set of a given query.
     * @param x the node of the input query
     * @param ctx the context of the input query
     * @param pts the output points-to set
     * @return whether the query is solved successfully
     */
    bool pointsToRecursionBased(NodeID x, Ctx &ctx, Pts &pts);

    /*!
     * Recursion-based implementation to compute the context-sensitive
     * flows-to set of a given query.
     * @param o the field-entry object node of the input query
     * @param offset the field offset of the input query
     * @param ctx the context of the input query
     * @param pts the output flows-to set
     * @return whether the query is solved successfully
     */
    bool flowsToRecursionBased(NodeID o, int offset, Ctx &ctx, Fts &fts);
    //@}

    BVDataPTAImpl *pta;
    PAG *pag;
    RCMemoryPartitioning *mp;
    MhpAnalysis *mhp;

    /// Analysis settings
    //@{
    bool dbg;
    bool worklistBased;
    bool useCache;
    //@}

    /// The remaining number of calls on pointsTo/flowsTo functions.
    int budget;

    /// Caches for Pts and Fts
    //@{
    PtsCache ptsCache;
    FtsCache ftsCache;
    //@}

    CtxPool ctxPool;

    OperationCollector *oc;

    static constexpr int initBudget = QUERY_BUDGET;
};



/*
 * Initialization
 */
template<typename Pts>
void CflReachabilityAnalysis<Pts>::init(RCMemoryPartitioning *mp,
        MhpAnalysis *mhp) {
    this->mhp = mhp;
    this->mp = mp;
    pta = mp->getPta();
    pag = pta->getPAG();
}



/*
 * Reset the analysis.
 */
template<typename Pts>
void CflReachabilityAnalysis<Pts>::reset() {
    // Reset the caches
    ptsCache.reset();
    ftsCache.reset();
}



/*
 * Dump CflReachability information
 */
template<typename Pts>
void CflReachabilityAnalysis<Pts>::print() const {
    std::cout << analysisUtil::pasMsg(" --- Cfl-Reachability Analysis ---\n");

    // Print the cache information
    ptsCache.print();
    ftsCache.print();

    std::cout << "\n";
}



/*
 * Get the context-sensitive points-to set of a given query.
 * @param n the node of the input query
 * @param ctx the context of the input query
 * @param pts the output points-to set
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::getPts(NodeID n, Ctx &ctx, Pts &pts) {

    // Reset budget
    budget = initBudget;

    // Perform the analysis
    bool ret = false;
    if (worklistBased) {
        VisitedWorkSets visitedWorkSets;
        visitedWorkSets.root = 0;
        ret = pointsToWorklistBased(n, ctx, pts, visitedWorkSets);
    } else {
        ret = pointsToRecursionBased(n, ctx, pts);
    }

    return ret;
}



/*
 * Worklist-based implementation to compute the context-sensitive
 * points-to set of a given query.
 * @param n the node of the input query
 * @param ctx the context of the input query
 * @param pts the output points-to set
 * @param visitedWorkSets the visited Work sets of pts and fts
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::pointsToWorklistBased(NodeID n,
        const Ctx &ctx, Pts &pts, VisitedWorkSets &visitedWorkSets) {

    if (dbg) {
        llvm::outs() << "\npointsTo():\t" << n << "\t" << &n << "\n";
        llvm::outs() << rcUtil::getSourceLoc(n) << "\n";
        ctx.print();
        if (useCache) {
            pts.print();
        }
        llvm::outs() << "\n";
    }

    // If cache is used, check if the query has been cached.
    if (useCache) {
        if (pts.isFailedToSolve()) {
            return false;
        }
        else if (pts.isSolved()) {
            return true;
        }
        else if (pts.isInProcess()) {
            if (0 == visitedWorkSets.root)
                return true;
        }
        pts.setInProcess();
    }

    // Check the budget.
    if (--budget <= 0) {
        pts.setOutOfBudget();
        return false;
    }

    // Perform the analysis.
    const Ctx *ctxPtr = ctxPool.getOrAdd(ctx);
    std::stack<Work> worklist;
    worklist.push(Work(n, ctxPtr));
    WorkSet &visited = visitedWorkSets.ptsVisited;

    while (!worklist.empty()) {
        Work work = worklist.top();
        worklist.pop();

        // Check if we have already visited this node
        if (visited.count(work))    continue;
        visited.insert(work);

        NodeID id = work.n;
        const Ctx *ctxPtr = work.ctx;

        // Backward traversal on the PAG
        PAGNode *N = pag->getPAGNode(id);
        const PAGNode::GEdgeSetTy &inEdges = N->getInEdges();
        for (auto it = inEdges.begin(), ie = inEdges.end(); it != ie; ++it) {
            PAGEdge *edge = *it;
            NodeID src = edge->getSrcID();
            const llvm::Instruction *I = NULL;

            switch (edge->getEdgeKind()) {
            // Handle addr edge
            case PAGEdge::Addr: {
                PAGNode *N = pag->getPAGNode(src);
                if (PAGNode::DummyObjNode == N->getNodeKind())  break;
                const llvm::Value *v = N->getValue();
                if (v && llvm::isa<llvm::GlobalValue>(v)) {
                    Ctx ctx_;
                    pts.addCsPts(src, ctx_);
                } else {
                    pts.addCsPts(src, *ctxPtr);
                }
                break;
            }

            // Handle copy edge
            case PAGEdge::Copy: {
                worklist.push(Work(src, ctxPtr));
                break;
            }

            // Handle call edge
            case PAGEdge::Call: {
                CallPE *callPe = llvm::cast<CallPE>(edge);
                I = callPe->getCallInst();
            }
            case PAGEdge::ThreadFork: {
                if (!I) {
                    TDForkPE *tdForkPe = llvm::cast<TDForkPE>(edge);
                    I = tdForkPe->getCallInst();
                }
            }
            {
                Ctx ctx_ = *ctxPtr;
                OperationCollector::CsID csId = oc->getCsID(I);
                bool matchCtx = ctx_.pop(csId);
                if (matchCtx) {
                    const Ctx *ctxPtr = ctxPool.getOrAdd(ctx_);
                    worklist.push(Work(src, ctxPtr));
                }
                break;
            }

            // Handle ret edge
            case PAGEdge::Ret: {
                RetPE *retPe = llvm::cast<RetPE>(edge);
                I = retPe->getCallInst();
            }
            case PAGEdge::ThreadJoin: {
                if (!I) {
                    TDJoinPE *tdJoinPe = llvm::cast<TDJoinPE>(edge);
                    I = tdJoinPe->getCallInst();
                }
            }
            {
                Ctx ctx_ = *ctxPtr;
                OperationCollector::CsID csId = oc->getCsID(I);
                if (ctx_.hasCycle(csId)) {
                    break;
                }
                ctx_.push(csId);
                const Ctx *ctxPtr = ctxPool.getOrAdd(ctx_);
                worklist.push(Work(src, ctxPtr));
                break;
            }

            // Handle load edge
            case PAGEdge::Load: {
                Pts aliasPts;
                if (!getAliasPointers(src, *ctxPtr, aliasPts)) {
                    pts.setStatus(aliasPts.getStatus());
                    return false;
                }

                for (auto it = aliasPts.begin(), ie = aliasPts.end(); it != ie; ++it) {
                    NodeID q = it->first;
                    CtxSet &ctxSet = it->second;

                    PAGNode *N = pag->getPAGNode(q);
                    const PAGNode::GEdgeSetTy &inEdges = N->getInEdges();
                    for (auto it = inEdges.begin(), ie = inEdges.end(); it != ie; ++it) {
                        const PAGEdge *edge = *it;
                        if (PAGEdge::Store != edge->getEdgeKind())  continue;

                        NodeID y = edge->getSrcID();
                        for (auto it = ctxSet.begin(), ie = ctxSet.end(); it != ie;
                                ++it) {
                            const Ctx *ctxPtr = ctxPool.getOrAdd(*it);
                            worklist.push(Work(y, ctxPtr));
                        }
                    }
                }
                break;
            }

            // Handle store edge
            case PAGEdge::Store: {
                // We need to do nothing here.
                break;
            }

            // Handle normalGep edge
            case PAGEdge::NormalGep: {
                if (visited.count(Work(src, ctxPtr))) {
                    break;
                }

                NormalGepPE *normalGepPe = llvm::cast<NormalGepPE>(edge);
                const LocationSet &ls = normalGepPe->getLocationSet();

                Pts pts_;
                Pts &gepPts = useCache ? ptsCache.getPts(src, *ctxPtr) : pts_;
                if (!pointsToWorklistBased(src, *ctxPtr, gepPts, visitedWorkSets)) {
                    pts.setStatus(gepPts.getStatus());
                    return false;
                }

                for (auto it = gepPts.begin(), ie = gepPts.end(); it != ie; ++it) {
                    NodeID o = it->first;
                    CtxSet &ctxSet = it->second;
                    NodeID gepObj = rcUtil::getGepObjNode(o, ls, pag);
                    if (gepObj) {
                        pts.addCsPts(gepObj, ctxSet);
                    }
                }
                break;
            }

            // Handle variantGep edge
            case PAGEdge::VariantGep: {
                if (visited.count(Work(src, ctxPtr))) {
                    break;
                }

                Pts pts_;
                Pts &gepPts = useCache ? ptsCache.getPts(src, *ctxPtr) : pts_;
                if (!pointsToWorklistBased(src, *ctxPtr, gepPts, visitedWorkSets)) {
                    pts.setStatus(gepPts.getStatus());
                    return false;
                }

                for (auto it = gepPts.begin(), ie = gepPts.end(); it != ie; ++it) {
                    NodeID o = it->first;
                    CtxSet &ctxSet = it->second;

                    auto &fields = pag->getAllFieldsObjNode(o);
                    for (auto it = fields.begin(), ie = fields.end(); it != ie; ++it) {
                        NodeID fieldObj = *it;
                        pts.addCsPts(fieldObj, ctxSet);
                    }
                }
                break;
            }

            // Default
            default: {
                llvm::outs() << "pointsToWorklistBased(): edge not handled " << edge->getEdgeKind() << "\n";
                pts.setOtherFailureStatus();
                return false;
                break;
            }
            }
        }
    }

    pts.setSolved();
    return true;
}



/*
 * Worklist-based implementation to compute the context-sensitive
 * flows-to set of a given query.
 * @param n the field-entry object node of the input query
 * @param offset the field offset of the input query
 * @param ctx the context of the input query
 * @param pts the output flows-to set
 * @param visitedWorkSets the visited Work sets of pts and fts
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::flowsToWorklistBased(NodeID n, int offset,
        const Ctx &ctx, Fts &fts, VisitedWorkSets &visitedWorkSets) {

    if (dbg) {
        llvm::outs() << "\nflowsTo():\t" << n << "\toffset(" << offset << ")\t"<< &n << "\n";
        ctx.print();
        if (useCache) {
            fts.print();
        }
        llvm::outs() << "\n";
    }

    // If cache is used, check if the query has been cached.
    if (useCache) {
        if (fts.isFailedToSolve()) {
            return false;
        }
        else if (fts.isSolved()) {
            return true;
        }
        else if (fts.isInProcess()) {
            if (0 == visitedWorkSets.root)
                return true;
        }
        fts.setInProcess();
    }

    // Check the budget.
    if (--budget <= 0) {
        fts.setOutOfBudget();
        return false;
    }

    // Perform the analysis.
    const Ctx *ctxPtr = ctxPool.getOrAdd(ctx);
    std::stack<Work> worklist;
    worklist.push(Work(n, ctxPtr));
    WorkSet &visited = visitedWorkSets.ftsVisited;

    while (!worklist.empty()) {
        Work work = worklist.top();
        worklist.pop();

        // Check if we have already visited this node
        if (visited.count(work))    continue;
        visited.insert(work);

        NodeID q = work.n;
        const Ctx *ctxPtr = work.ctx;
        PAGNode *N = pag->getPAGNode(q);

        // If there is no offset.
        if (offset < 0) {
            // Check if the value flows to the pointer of a load/store.
            if (N->hasOutgoingEdges(PAGEdge::Load)
                    || N->hasIncomingEdges(PAGEdge::Store)) {
                fts.addCsPts(q, *ctxPtr);
            }
        }

        // If there is variantGep edge of the base node of the node being traversed,
        // push it into the worklist.
        if (llvm::isa<ObjPN>(N)) {
            NodeID base = pag->getBaseObjNode(q);
            PAGNode *baseN = pag->getPAGNode(base);
            auto &variantGepEdges = baseN->getOutgoingEdges(PAGEdge::VariantGep);
            for (auto it = variantGepEdges.begin(), ie = variantGepEdges.end(); it != ie; ++it) {
                PAGEdge *edge = *it;
                NodeID dst = edge->getDstID();
                worklist.push(Work(dst, ctxPtr));
            }
        }

        // Traverse successors
        const PAGNode::GEdgeSetTy &outEdges = N->getOutEdges();
        for (auto it = outEdges.begin(), ie = outEdges.end(); it != ie; ++it) {
            PAGEdge *edge = *it;
            NodeID dst = edge->getDstID();
            const llvm::Instruction *I = NULL;

            switch (edge->getEdgeKind()) {
            // Handle Addr and Copy edge
            case PAGEdge::Addr:
            case PAGEdge::Copy: {
                worklist.push(Work(dst, ctxPtr));
                break;
            }

            // Handle call edge
            case PAGEdge::Call: {
                CallPE *callPe = llvm::cast<CallPE>(edge);
                I = callPe->getCallInst();
            }
            case PAGEdge::ThreadFork: {
                if (!I) {
                    TDForkPE *tdForkPe = llvm::cast<TDForkPE>(edge);
                    I = tdForkPe->getCallInst();
                }
            }
            {
                Ctx ctx_ = *ctxPtr;
                OperationCollector::CsID csId = oc->getCsID(I);
                if (ctx_.hasCycle(csId)) {
                    break;
                }
                ctx_.push(csId);
                const Ctx *ctxPtr = ctxPool.getOrAdd(ctx_);
                worklist.push(Work(dst, ctxPtr));
                break;
            }

            // Handle ret edge
            case PAGEdge::Ret: {
                RetPE *retPe = llvm::cast<RetPE>(edge);
                I = retPe->getCallInst();
            }
            case PAGEdge::ThreadJoin: {
                if (!I) {
                    TDJoinPE *tdJoinPe = llvm::cast<TDJoinPE>(edge);
                    I = tdJoinPe->getCallInst();
                }
            }
            {
                Ctx ctx_ = *ctxPtr;
                OperationCollector::CsID csId = oc->getCsID(I);
                bool matchCtx = ctx_.pop(csId);
                if (matchCtx) {
                    const Ctx *ctxPtr = ctxPool.getOrAdd(ctx_);
                    worklist.push(Work(dst, ctxPtr));
                }
                break;
            }

            // Handle Store edge
            case PAGEdge::Store: {
                Pts aliasPts;
                if (!getAliasPointers(dst, *ctxPtr, aliasPts)) {
                    fts.setStatus(aliasPts.getStatus());
                    return false;
                }

                for (auto it = aliasPts.begin(), ie = aliasPts.end(); it != ie; ++it) {
                    NodeID p = it->first;
                    CtxSet &ctxSet = it->second;

                    PAGNode *N = pag->getPAGNode(p);
                    const PAGNode::GEdgeSetTy &outEdges = N->getOutEdges();
                    for (auto it = outEdges.begin(), ie = outEdges.end(); it != ie; ++it) {
                        const PAGEdge *edge = *it;
                        if (PAGEdge::Load != edge->getEdgeKind())  continue;

                        NodeID x = edge->getDstID();
                        for (auto it = ctxSet.begin(), ie = ctxSet.end(); it != ie;
                                ++it) {
                            const Ctx *ctxPtr = ctxPool.getOrAdd(*it);
                            worklist.push(Work(x, ctxPtr));
                        }
                    }
                }
                break;
            }

            // Handle Load edge
            case PAGEdge::Load: {
                // We need to do nothing here.
                break;
            }

            // Handle NormalGep edge
            case PAGEdge::NormalGep: {
                NormalGepPE *normalGepPe = llvm::cast<NormalGepPE>(edge);
                int gepOffset = normalGepPe->getLocationSet().getOffset();

                if (offset < 0 && gepOffset > 0)     break;

                if (offset != gepOffset) {
                    // If Gep has zero offset in this case, then handle this Gep as Copy
                    if (gepOffset == 0) {
                        worklist.push(Work(dst, ctxPtr));
                    }
                    break;
                }

                Fts fts_;
                Fts &gepFts = useCache ? ftsCache.getFts(dst, -1, *ctxPtr) : fts_;
                if (!flowsToWorklistBased(dst, -1, *ctxPtr, gepFts, visitedWorkSets)) {
                    fts.setStatus(gepFts.getStatus());
                    return false;
                }

                for (auto it = gepFts.begin(), ie = gepFts.end();
                        it != ie; ++it) {
                    NodeID q = it->first;
                    CtxSet &ctxSet = it->second;
                    fts.addCsPts(q, ctxSet);
                }
                break;
            }

            // Handle VariantGep edge
            case PAGEdge::VariantGep: {
                Fts fts_;
                Fts &gepFts = useCache ? ftsCache.getFts(dst, -1, *ctxPtr) : fts_;
                if (!flowsToWorklistBased(dst, -1, *ctxPtr, gepFts, visitedWorkSets)) {
                    fts.setStatus(gepFts.getStatus());
                    return false;
                }

                for (auto it = gepFts.begin(), ie = gepFts.end();
                        it != ie; ++it) {
                    NodeID q = it->first;
                    CtxSet &ctxSet = it->second;
                    fts.addCsPts(q, ctxSet);
                }
                break;
            }

            // Default
            default: {
                llvm::outs() << "flowsToWorklistBased(): edge not handled " << edge->getEdgeKind() << "\n";
                fts.setOtherFailureStatus();
                return false;
                break;
            }
            }
        }

    }

    fts.setSolved();
    return true;
}


/*
 * The wrapper function to compute the pointers,
 * who point-to aliases context-sensitively with a given query.
 * @param p the pointer of the input query
 * @param ctx the context of the input query
 * @param aliasPointers the output set of the alias pointers and contexts
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::getAliasPointers(NodeID p, const Ctx &ctx,
        Pts &aliasPointers) {

    // The record of the work being processed
    static WorkSet visiting;

    // Skip the cyclic work
    Work w(p, &ctx);
    if (visiting.count(w)) {
        aliasPointers.setInProcess();
        return true;
    }

    // Add the work into the visiting list
    typename WorkSet::iterator iter = visiting.insert(w).first;

    // Perform the alias analysis
    bool ret = getAliasPointersImpl(p, ctx, aliasPointers);

    // Remove the work from the visiting list
    visiting.erase(iter);

    return ret;
}


/*
 * The implementation function to compute the pointers,
 * who point-to aliases context-sensitively with a given query.
 * @param p the pointer of the input query
 * @param ctx the context of the input query
 * @param aliasPointers the output set of the alias pointers and contexts
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::getAliasPointersImpl(NodeID p, const Ctx &ctx,
        Pts &aliasPointers) {

    if (dbg) {
        llvm::outs() << "\nalias():\t" << p << "\t" << &p << "\n";
        ctx.print();
        llvm::outs() << "\n";
    }

    // Compute points-to set
    VisitedWorkSets visitedWorkSets;
    visitedWorkSets.root = p;
    Ctx ctx_ = ctx;
    Pts pts_;
    Pts &pts = useCache ? ptsCache.getPts(p, ctx_) : pts_;

    if (worklistBased) {
        if (!pointsToWorklistBased(p, ctx_, pts, visitedWorkSets)) {
            aliasPointers.setStatus(pts.getStatus());
            return false;
        }
        if (useCache) {
            if (pts.isInProcess()) {
                aliasPointers.setStatus(pts.getStatus());
                return true;
            }
        }
    } else {
        if (!pointsToRecursionBased(p, ctx_, pts)) {
            aliasPointers.setStatus(pts.getStatus());
            return false;
        }
    }

    // Compute the relevant flows-to set and then obtain aliases.
    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
        NodeID o = it->first;
        CtxSet &ctxSet = it->second;

        // Skip dummy object node.
        PAGNode *N = pag->getPAGNode(o);
        if (PAGNode::DummyObjNode == N->getNodeKind())  continue;

        // Check if o is a global object.
        const llvm::Value *v = N->getValue();
        bool isGlobal = v && llvm::isa<llvm::GlobalValue>(v);

        // Check if o is a gep object.
        // The flowsTo edges from any object must start at the entry object.
        int offset = rcUtil::getFieldObjOffset(o, pag);
        bool isGepObj = offset >= 0;
        NodeID baseObj = isGepObj ? pag->getBaseObjNode(o) : o;

        for (auto it = ctxSet.begin(), ie = ctxSet.end(); it != ie; ++it) {
            Ctx emptyCtx;
            Ctx existingCtx = *it;
            Ctx &ctx = isGlobal ? emptyCtx : existingCtx;
            Fts fts_;
            Fts &fts = useCache ? ftsCache.getFts(baseObj, offset, ctx) : fts_;

            if (worklistBased) {
                if (!flowsToWorklistBased(baseObj, offset, ctx, fts, visitedWorkSets)) {
                    aliasPointers.setStatus(pts.getStatus());
                    return false;
                }
                if (useCache) {
                    if (fts.isInProcess()) {
                        continue;
                    }
                }
            }
            else {
                if (!flowsToRecursionBased(baseObj, offset, ctx, fts)) {
                    aliasPointers.setStatus(pts.getStatus());
                    return false;
                }
            }

            for (auto it = fts.begin(), ie = fts.end(); it != ie; ++it) {
                NodeID q = it->first;
                CtxSet &ctxSet = it->second;
                aliasPointers.addCsPts(q, ctxSet);
            }
        }
    }

    aliasPointers.setSolved();

    if (dbg) {
        llvm::outs() << "\nalias() solved:\t" << p << "\t" << &p << "\n";
        aliasPointers.print();
        llvm::outs() << "\n";
    }
    return true;
}


/*
 * Include the struct base object for any field object from the
 * input pts into itself.
 */
template<typename Pts>
void CflReachabilityAnalysis<Pts>::includeFIObjForAnyField(
        Pts &pts) const {
    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
        NodeID n = it->first;
        NodeID baseObj = pag->getBaseObjNode(n);
        if (baseObj == n)   continue;

        pts.addCsPts(baseObj, it->second);
    }
}


/*
 * Include the struct base object for any first field object
 * (i.e., with zero offset) from the input pts into itself.
 */
template<typename Pts>
void CflReachabilityAnalysis<Pts>::includeFIObjForFirstField(
        Pts &pts) const {
    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
        NodeID n = it->first;
        NodeID baseObj = pag->getBaseObjNode(n);
        if (baseObj == n)   continue;

        if (rcUtil::isFirstFieldObj(n, pag)) {
            pts.addCsPts(baseObj, it->second);
        }
    }
}


/*
 * Recursion-based implementation to compute the context-sensitive
 * points-to set of a given query.
 * @param n the node of the input query
 * @param ctx the context of the input query
 * @param pts the output points-to set
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::pointsToRecursionBased(NodeID n, Ctx &ctx,
        Pts &pts) {
    assert(false && "Obsolete code");
    return false;
}



/*
 * Recursion-based implementation to compute the context-sensitive
 * flows-to set of a given query.
 * @param n the field-entry object node of the input query
 * @param offset the field offset of the input query
 * @param ctx the context of the input query
 * @param pts the output flows-to set
 * @return whether the query is solved successfully
 */
template<typename Pts>
bool CflReachabilityAnalysis<Pts>::flowsToRecursionBased(NodeID n, int offset,
        Ctx &ctx, Fts &fts) {
    assert(false && "Obsolete code");
    return false;
}



#endif /* CFLREACHABILITYANALYSIS_H_ */
