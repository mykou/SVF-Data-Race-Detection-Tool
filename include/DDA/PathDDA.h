/*
 * PathDDA.h
 *
 *  Created on: Jul 7, 2014
 *      Author: Yulei Sui, Sen Ye
 */

#ifndef PathDDA_H_
#define PathDDA_H_

#include "MemoryModel/PointerAnalysis.h"
#include "DDA/DDAVFSolver.h"
#include "Util/DPItem.h"
#include "DDA/DDAPathAllocator.h"

class ContextDDA;
class DDAClient;
typedef PathStmtDPItem<SVFGNode> PathDPItem;

/*!
 * Context- and flow- sensitive demand-driven points-to analysis using guarded value-flow analysis
 */
class PathDDA : public CondPTAImpl<VFPathCond>, public DDAVFSolver<VFPathVar,VFPathPtSet,PathDPItem> {

public:
    typedef PathCondAllocator::Condition Condition;
    typedef std::map<const SVFGEdge *, Condition*> SVFGEdgeToCondMapTy;
    /// Constructor
    PathDDA(llvm::Module* m, DDAClient* client);
    /// Destructor
    virtual ~PathDDA();

    /// Initialization of the analysis
    virtual void initialize(llvm::Module& module);

    /// Finalize analysis
    virtual void finalize() {
        CondPTAImpl<VFPathCond>::finalize();
    }

    /// dummy analyze method
    virtual void analyze(llvm::Module& mod) {}

    /// Compute points-to set for an unconditional pointer
    void computeDDAPts(NodeID id);

    /// Compute points-to set for a path-sensitive pointer
    const VFPathPtSet& computeDDAPts(const VFPathVar& pathVar);

    /// Handle out-of-budget query
    void handleOutOfBudgetDpm(const PathDPItem& dpm);

    /// Override parent method
    inline VFPathPtSet getConservativeCPts(const PathDPItem& dpm) {
        const PointsTo& pts =  getAndersenAnalysis()->getPts(dpm.getCurNodeID());
        VFPathPtSet tmpCPts;
        VFPathCond vfpath;
        for (PointsTo::iterator piter = pts.begin(); piter != pts.end(); ++piter) {
            VFPathVar var(vfpath,*piter);
            tmpCPts.set(var);
        }
        return tmpCPts;
    }

    /// Override parent method
    virtual inline NodeID getPtrNodeID(const VFPathVar& var) const {
        return var.get_id();
    }
    /// Return a dpm with old context but new path condition
    virtual inline PathDPItem getDPImWithOldCond(const PathDPItem& oldDpm,const VFPathVar& var, const SVFGNode* loc) {
        PathDPItem dpm(oldDpm);
        dpm.setLocVar(loc,var.get_id());
        /// TODO: decide whether we may copy the old path information to the new DPM
        dpm.getCond().setPaths(var.get_cond().getPaths(),var.get_cond().getVFEdges());
        if(llvm::isa<StoreSVFGNode>(loc))
            addLoadDpmAndCVar(dpm,getLoadDpm(oldDpm),var);

        if(llvm::isa<LoadSVFGNode>(loc))
            addLoadDpmAndCVar(dpm,oldDpm,var);

        DOSTAT(ddaStat->_NumOfDPM++);
        return dpm;
    }

    /// Handle condition for context or path analysis
    virtual bool handleBKCondition(PathDPItem& dpm, const SVFGEdge* edge);

    /// refine indirect call edge
    bool testIndCallReachability(PathDPItem& dpm, const llvm::Function* callee, llvm::CallSite cs);

    /// get callsite id from call, return false if it is a spurious call edge
    CallSiteID getCSIDAtCall(PathDPItem& dpm, const SVFGEdge* edge);

    /// get callsite id from return, return false if it is a spurious return edge
    CallSiteID getCSIDAtRet(PathDPItem& dpm, const SVFGEdge* edge);


    /// Pop recursive callsites
    inline virtual void popRecursiveCallSites(PathDPItem& dpm) {
        CallStrCxt& cxt = dpm.getCond().getContexts();
        while(!cxt.empty() && isEdgeInRecursion(cxt.back())) {
            cxt.pop_back();
        }
    }
    /// Whether call/return inside recursion
    inline virtual bool isEdgeInRecursion(CallSiteID csId) {
        const llvm::Function* caller = getPTACallGraph()->getCallerOfCallSite(csId);
        const llvm::Function* callee = getPTACallGraph()->getCalleeOfCallSite(csId);
        return inSameCallGraphSCC(caller, callee);
    }
    /// Update call graph.
    //@{
    void updateCallGraphAndSVFG(const PathDPItem& dpm,llvm::CallSite cs,SVFGEdgeSet& svfgEdges)
    {
        CallEdgeMap newEdges;
        resolveIndCalls(cs, getBVPointsTo(getCachedPointsTo(dpm)), newEdges);
        for (CallEdgeMap::const_iterator iter = newEdges.begin(),eiter = newEdges.end(); iter != eiter; iter++) {
            llvm::CallSite newcs = iter->first;
            const FunctionSet & functions = iter->second;
            for (FunctionSet::const_iterator func_iter = functions.begin(); func_iter != functions.end(); func_iter++) {
                const llvm::Function * func = *func_iter;
                getSVFG()->connectCallerAndCallee(newcs, func, svfgEdges);
            }
        }
    }
    //@}

    /// processGep node
    VFPathPtSet processGepPts(const GepSVFGNode* gep, const VFPathPtSet& srcPts);

    /// Handle Address SVFGNode to add proper conditional points-to
    inline void handleAddr(VFPathPtSet& pts,const PathDPItem& dpm,const AddrSVFGNode* addr) {
        NodeID srcID = addr->getPAGSrcNodeID();
        /// whether this object is set field-insensitive during pre-analysis
        if (isFieldInsensitive(srcID))
            srcID = getFIObjNode(srcID);

        VFPathVar var(dpm.getCond(),srcID);
        addDDAPts(pts,var);
        DBOUT(DDDA, llvm::outs() << "\t add points-to target " << var << " to dpm ");
        DBOUT(DDDA, dpm.dump());
    }

    /// Propagate along indirect value-flow if two objects of load and store are same
    virtual inline bool propagateViaObj(const VFPathVar& storeObj, const VFPathVar& loadObj) {
        return isSameVar(storeObj,loadObj);
    }
    /// Whether load and store are aliased or not
//	virtual bool isMustAlias(const PathDPItem& loadDpm, const PathDPItem& storeDpm) {
//		return mustAlias(loadDpm.getCondVar(),storeDpm.getCondVar());
//	}

    /// Whether two conditions are compatible
    bool isCondCompatible(const VFPathCond& cond1, const VFPathCond& cond2, bool singleton) const;

    /// Whether two call string contexts are compatible which may represent the same memory object
    /// compare with call strings from last few callsite ids (most recent ids to objects):
    /// compatible : (e.g., 123 == 123, 123 == 23). not compatible (e.g., 123 != 423)
    inline bool isCxtsCompatible (const CallStrCxt& largeCxt, const CallStrCxt& smallCxt) const {
        for(int i = smallCxt.size() - 1; i >= 0; i--) {
            if(largeCxt[i] != smallCxt[i])
                return false;
        }
        return true;
    }

    /// Get pathCondition according to a value-flow edge
    Condition* getVFCond(const SVFGEdge* edge);
    /// Get complement BB of a SSA phi
    //@{
    const llvm::BasicBlock* getComplementBBOfMPHI(const IntraMSSAPHISVFGNode* mphi,const SVFGEdge* edge);
    const llvm::BasicBlock* getComplementBBOfNPHI(const IntraPHISVFGNode* phi, u32_t pos);
    //@}

    /// Condition operations
    //@{
    inline Condition* condAnd(Condition* lhs, Condition* rhs) {
        return pathCondAllocator->condAnd(lhs,rhs);
    }
    inline Condition* condOr(Condition* lhs, Condition* rhs) {
        return pathCondAllocator->condOr(lhs,rhs);
    }
    //@}

    /// Callsite related methods
    //@{
    inline llvm::CallSite getCallSite(const SVFGEdge* edge) const {
        assert(edge->isCallVFGEdge() && "not a call svfg edge?");
        if(const CallDirSVFGEdge* callEdge = llvm::dyn_cast<CallDirSVFGEdge>(edge))
            return getSVFG()->getCallSite(callEdge->getCallSiteId());
        else
            return getSVFG()->getCallSite(llvm::cast<CallIndSVFGEdge>(edge)->getCallSiteId());
    }
    inline llvm::CallSite getRetSite(const SVFGEdge* edge) const {
        assert(edge->isRetVFGEdge() && "not a return svfg edge?");
        if(const RetDirSVFGEdge* callEdge = llvm::dyn_cast<RetDirSVFGEdge>(edge))
            return getSVFG()->getCallSite(callEdge->getCallSiteId());
        else
            return getSVFG()->getCallSite(llvm::cast<RetIndSVFGEdge>(edge)->getCallSiteId());
    }
    //@}
    /// Whether this edge is treated context-insensitively
    bool isInsensitiveCallRet(const SVFGEdge* edge) {
        return insensitveEdges.find(edge) != insensitveEdges.end();
    }
    /// Return insensitive edge set
    inline ConstSVFGEdgeSet& getInsensitiveEdgeSet() {
        return insensitveEdges;
    }
    /// dump context call strings
    virtual inline void dumpContexts(const ContextCond& cxts) {
        llvm::outs() << cxts.toString() << "\n";
    }

    virtual const std::string PTAName() const {
        return "PathSensitive DDA";
    }

private:
    SVFGEdgeToCondMapTy edgeToCondMap;
    DDAPathAllocator* pathCondAllocator;
    ConstSVFGEdgeSet insensitveEdges;
    ContextDDA* contextDDA;
    DDAClient* _client;				///< DDA client
};

#endif /* PathDDA_H_ */
