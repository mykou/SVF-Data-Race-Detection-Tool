/*
 * RCUtil.h
 *
 *  Created on: 20/10/2014
 *      Author: dye
 */

#ifndef RCUTIL_H_
#define RCUTIL_H_

#include "Util/BasicTypes.h"
#include "Util/PTACallGraph.h"
#include "MemoryModel/PAG.h"
#include "MemoryModel/PointerAnalysis.h"
#include "llvm/ADT/SmallSet.h"


class ThreadCallGraph;


/*!
 * Memory access operation
 */
class MemoryAccess {
public:

    /// Constructors
    //@{
    MemoryAccess() :
            I(0), p(0), isWrite(false) {
    }

    /**
     * Constructors
     * @param I the Instruction of the memory access
     * @param p the pointer of the memory object(s) which may be accessed
     * @param isWrite denotes if it is a write access
     */
    MemoryAccess(const llvm::Instruction *I, const llvm::Value *p, bool isWrite) :
            I(I), p(p), isWrite(isWrite) {
    }
    //@}

    /// Reset the MemoryAccess
    inline void reset(const llvm::Instruction *I, const llvm::Value *p,
            bool isWrite) {
        this->I = I;
        this->p = p;
        this->isWrite = isWrite;
    }

    /// Get member accesses
    //@{
    inline const llvm::Instruction *getInstruction() const {
        return I;
    }
    inline const llvm::Value *getPointer() const {
        return p;
    }
    inline bool isWriteAccess() const {
        return isWrite;
    }
    //@}

    /// Dump MemoryAccess information
    void print() const;

private:
    const llvm::Instruction *I;     ///< the Instruction of the memory access
    const llvm::Value *p;   ///< the pointer of the memory object(s) which may be accessed
    bool isWrite;           ///< denotes if it is a write access
};



/*!
 * Basic code set, including the sets of Functions,
 * BasicBlocks, and Instructions.
 */
class CodeSet {
public:
    typedef llvm::SmallSet<const llvm::Function*, 16> FuncSet;
    typedef llvm::SmallSet<const llvm::BasicBlock*, 16> BbSet;
    typedef llvm::SmallSet<const llvm::Instruction*, 16> InstSet;
    typedef llvm::SmallSet<const llvm::Value*, 16> ValSet;

    typedef FuncSet::iterator func_iterator;
    typedef FuncSet::const_iterator const_func_iterator;
    typedef BbSet::iterator bb_iterator;
    typedef BbSet::const_iterator const_bb_iterator;
    typedef InstSet::iterator inst_iterator;
    typedef InstSet::const_iterator const_inst_iterator;

    /// Iterators for FuncSet
    //@{
    inline func_iterator func_begin() {
        return funcs.begin();
    }
    inline func_iterator func_end() {
        return funcs.end();
    }
    inline const_func_iterator func_begin() const {
        return funcs.begin();
    }
    inline const_func_iterator func_end() const {
        return funcs.end();
    }
    //@}

    /// Iterators for BbSet
    //@{
    inline bb_iterator bb_begin() {
        return bbs.begin();
    }
    inline bb_iterator bb_end() {
        return bbs.end();
    }
    inline const_bb_iterator bb_begin() const {
        return bbs.begin();
    }
    inline const_bb_iterator bb_end() const {
        return bbs.end();
    }
    //@}

    /// Iterators for InstSet
    //@{
    inline inst_iterator inst_begin() {
        return insts.begin();
    }
    inline inst_iterator inst_end() {
        return insts.end();
    }
    inline const_inst_iterator inst_begin() const {
        return insts.begin();
    }
    inline const_inst_iterator inst_end() const {
        return insts.end();
    }
    //@}

    /// Insert operations
    //@{
    inline void insert(const llvm::Instruction *I) {
        insts.insert(I);
    }
    inline void insert(InstSet::const_iterator begin,
            const InstSet::const_iterator end) {
        insts.insert(begin, end);
    }
    inline void insert(const llvm::BasicBlock *BB) {
        bbs.insert(BB);
    }
    inline void insert(BbSet::const_iterator begin,
            const BbSet::const_iterator end) {
        bbs.insert(begin, end);
    }
    inline void insert(const llvm::Function *F) {
        funcs.insert(F);
    }
    inline void insert(FuncSet::const_iterator begin,
            const FuncSet::const_iterator end) {
        funcs.insert(begin, end);
    }
    //@}

    /// Count operations
    //@{
    inline InstSet::size_type count(const llvm::Instruction *I) const {
        return insts.count(I);
    }
    inline BbSet::size_type count(const llvm::BasicBlock *BB) const {
        return bbs.count(BB);
    }
    inline FuncSet::size_type count(const llvm::Function *F) const {
        return funcs.count(F);
    }
    //@}

    /// Check if the CodeSet is empty
    inline bool empty() const {
        return insts.empty() && bbs.empty() && funcs.empty();
    }

    /// Clear the CodeSet
    inline void clear() {
        insts.clear();
        bbs.clear();
        funcs.clear();
    }

    /// Check if the code set covers a given Instruction
    inline bool covers(const llvm::Instruction *I) const {
        const llvm::BasicBlock *BB = I->getParent();
        const llvm::Function *F = BB->getParent();
        return funcs.count(F) || bbs.count(BB) || insts.count(I);
    }

    /// Check if the code set covers a given BasicBlock
    inline bool covers(const llvm::BasicBlock *BB) const {
        const llvm::Function *F = BB->getParent();
        return funcs.count(F) || bbs.count(BB);
    }

    /// Remove redundant code.
    inline void removeRedundamcy() {
        for (auto it = insts.begin(), ie = insts.end(); it != ie; ++it) {
            const llvm::Instruction *I = *it;
            if (bbs.count(I->getParent())) {
                insts.erase(I);
            }
        }
        for (auto it = bbs.begin(), ie = bbs.end(); it != ie; ++it) {
            const llvm::BasicBlock *bb = *it;
            if (funcs.count(bb->getParent())) {
                bbs.erase(bb);
            }
        }
    }

private:
    FuncSet funcs;  ///< A set of Functions
    BbSet bbs;      ///< A set of BasicBlocks
    InstSet insts;  ///< A set of Instructions
};



/*!
 * Program operation collection for later analysis.
 * It collects read, write, lock/unlock, and spawn.
 */
class OperationCollector {
public:
    typedef unsigned CsID;

    static OperationCollector *getInstance() {
        if (!singleton) {
            singleton = new OperationCollector();
        }
        return singleton;
    }

    /// Constructor
    OperationCollector(): M(0), csIdCounter(0) {
    }

    /// Initialization
    void init(llvm::Module *M);

    /// Collect interested operations
    void analyze();

    /// Release resource
    void release();

    /// Get collected instructions
    //@{
    inline const std::vector<MemoryAccess> &getReads() const {
        return reads;
    }
    inline const std::vector<MemoryAccess> &getWrites() const {
        return writes;
    }
    inline const std::vector<const llvm::Instruction*> &getLocksUnlocks() const {
        return locksUnlocks;
    }
    inline const std::vector<const llvm::Instruction*> &getSpawnsJoins() const {
        return spawnsJoins;
    }
    inline const std::vector<const llvm::Instruction*> &getBarriers() const {
        return barriers;
    }
    inline const std::vector<const llvm::Instruction*> &getHareParFors() const {
        return hareParFors;
    }
    //@}

    /// Mapping between CsInst and CsID
    //@{
    inline const llvm::Instruction *getCsInst(CsID id) const {
        if (id < csInstVec.size()) {
            return csInstVec[id];
        }
        assert(false && "CallSite Instruction should have been recorded.");
        return csInstVec[0];
    }
    inline CsID getCsID(const llvm::Instruction *I) const {
        auto it = csInst2csIdMap.find(I);
        if (it != csInst2csIdMap.end()) {
            return it->second;
        }
        assert(false && "CallSite Instruction should have been recorded.");
        return 0;
    }
    inline void addCsInst(const llvm::Instruction *I) {
        csInst2csIdMap[I] = csIdCounter++;
        csInstVec.push_back(I);
    }
    //@}

    /// Dump the information of collected operations
    void print() const;

private:
    static OperationCollector *singleton;

    llvm::Module *M;
    std::vector<MemoryAccess> reads;
    std::vector<MemoryAccess> writes;
    std::vector<const llvm::Instruction*> locksUnlocks;
    std::vector<const llvm::Instruction*> spawnsJoins;
    std::vector<const llvm::Instruction*> barriers;
    std::vector<const llvm::Instruction*> hareParFors;
    llvm::DenseMap<const llvm::Instruction*, CsID> csInst2csIdMap;
    std::vector<const llvm::Instruction*> csInstVec;
    CsID csIdCounter;
};


/*!
 * A duplication of llvm::DominanceFrontier class.
 */
class DomFrontier {
public:
    typedef llvm::DominanceFrontierBase<llvm::BasicBlock>::iterator iterator;
    typedef llvm::DominanceFrontierBase<llvm::BasicBlock>::const_iterator const_iterator;

    /// Initialization
    void init(const llvm::Function *F);

    /// Tell if this class if for post dominance frontier calculation.
    inline bool isPostDominator() const {
        return Base.isPostDominator();
    }

    /// Iterators and lookup functions
    //@{
    inline iterator begin() {
        return Base.begin();
    }
    inline const_iterator begin() const {
        return Base.begin();
    }
    inline iterator end() {
        return Base.end();
    }
    inline const_iterator end() const {
        return Base.end();
    }
    inline iterator find(llvm::BasicBlock *B) {
        return Base.find(B);
    }
    inline const_iterator find(llvm::BasicBlock *B) const {
        return Base.find(B);
    }
    //@}

private:
    llvm::ForwardDominanceFrontierBase<llvm::BasicBlock> Base;
};



/*!
 * Namespace for the utilities used for RaceComb.
 */
namespace rcUtil {

/// Get the meta data here for line number and file name.
std::string getSourceLoc(const llvm::Value* v);


/// Get the meta data here for line number and file name.
std::string getSourceLoc(NodeID objId);


/*!
 * Split a string into substrings
 * @param s the input string
 * @param delim the char used for splitting
 * @param elems the output substrings
 * @return a reference to the output substrings
 */
std::vector<std::string> &split(const std::string &s, char delim,
        std::vector<std::string> &elems);


/// Get the intersection of two ValSets.
template<typename ValSet>
inline ValSet getIntersection(const ValSet &set1, const ValSet &set2) {
    ValSet ret;
    for (typename ValSet::const_iterator it = set1.begin(), ie = set1.end();
            it != ie; ++it) {
        const llvm::Value *I = *it;
        if (set2.count(I))  ret.insert(I);
    }
    return ret;
}


/// Strip all casts
inline const llvm::Value *stripAllCasts(const llvm::Value *val) {
    do {
        if (const llvm::CastInst *CI = llvm::dyn_cast<const llvm::CastInst>(
                val)) {
            val = CI->getOperand(0);
        } else if (const llvm::ConstantExpr *ce = llvm::dyn_cast<
                const llvm::ConstantExpr>(val)) {
            if (ce->isCast())
                val = ce->getOperand(0);
            else if (ce->isGEPWithNoNotionalOverIndexing()){
                val = ce->getOperand(0);
            }
        } else {
            return val;
        }
    } while (true);
    return NULL;
}


/// Update PAG by applying pta to solve indirect calls.
void updatePAG(PAG *pag, PointerAnalysis *pta);


/// Update PAG by applying ThreadCallGraph to solve indirect calls.
void updatePAG(PAG *pag, ThreadCallGraph *tcg);


/// Update PTACallGraph by applying pta to solve indirect calls.
void updateCallGraph(PTACallGraph *cg, PointerAnalysis *pta);


/// Check if a Function is a dead function (i.e. not reachable from main).
bool isDeadFunction(const llvm::Function *F);


/*!
 * Collect all dir/indir callees of a call site
 * @param cg the input call graph to look up from
 * @param csInst the input call site Instruction
 * @param callees the output callees
 */
template<typename FuncSet>
void getCallees(const PTACallGraph *cg, const llvm::Instruction *csInst,
        FuncSet &callees) {
    if (!cg->hasCallGraphEdge(csInst))    return;
    for (PTACallGraph::CallGraphEdgeSet::const_iterator it =
            cg->getCallEdgeBegin(csInst), ie = cg->getCallEdgeEnd(csInst);
            it != ie; ++it) {
        const PTACallGraphEdge* edge = *it;
        const llvm::Function* callee = edge->getDstNode()->getFunction();
        callees.insert(callee);
    }
}


/*!
 * Collect all valid call sites (i.e., not in dead functions) that invoke callee
 * @param cg the input call graph to look up from
 * @param callee the input callee
 * @param csInsts the output call sites Instruction
 */
template<typename InstSet>
void getAllValidCallSitesInvokingCallee(const PTACallGraph *cg,
        const llvm::Function *callee, InstSet &csInsts) {
    PTACallGraphNode *calleeCgNode = cg->getCallGraphNode(callee);
    for (PTACallGraphNode::iterator it = calleeCgNode->InEdgeBegin(), eit =
            calleeCgNode->InEdgeEnd(); it != eit; ++it) {
        PTACallGraphEdge *edge = *it;

        // Skip invalid edges (i.e., those from dead functions)
        PTACallGraphNode *callerCgNode = edge->getSrcNode();
        const llvm::Function *caller = callerCgNode->getFunction();
        if (isDeadFunction(caller))     continue;

        // Collect call sites into output
        for (PTACallGraphEdge::CallInstSet::iterator cit =
                edge->directCallsBegin(), ecit = edge->directCallsEnd();
                cit != ecit; ++cit) {
            csInsts.insert((*cit));
        }
        for (PTACallGraphEdge::CallInstSet::iterator cit =
                edge->indirectCallsBegin(), ecit = edge->indirectCallsEnd();
                cit != ecit; ++cit) {
            csInsts.insert((*cit));
        }
    }
}


/*!
 * Collect all valid call sites (i.e., not in dead functions) that invoke callee
 * @param cg the input call graph to look up from
 * @param callee the input callee
 * @param csIds the output CsID set
 */
template<typename InstSet>
void getAllValidCsIdsInvokingCallee(const PTACallGraph *cg,
        const llvm::Function *callee, InstSet &csIds) {
    OperationCollector *oc = OperationCollector::getInstance();
    PTACallGraphNode *calleeCgNode = cg->getCallGraphNode(callee);
    for (PTACallGraphNode::iterator it = calleeCgNode->InEdgeBegin(), eit =
            calleeCgNode->InEdgeEnd(); it != eit; ++it) {
        PTACallGraphEdge *edge = *it;

        // Skip invalid edges (i.e., those from dead functions)
        PTACallGraphNode *callerCgNode = edge->getSrcNode();
        const llvm::Function *caller = callerCgNode->getFunction();
        if (isDeadFunction(caller))     continue;

        // Collect call sites into output
        for (PTACallGraphEdge::CallInstSet::iterator cit =
                edge->directCallsBegin(), ecit = edge->directCallsEnd();
                cit != ecit; ++cit) {
            OperationCollector::CsID csId = oc->getCsID(*cit);
            csIds.insert(csId);
        }
        for (PTACallGraphEdge::CallInstSet::iterator cit =
                edge->indirectCallsBegin(), ecit = edge->indirectCallsEnd();
                cit != ecit; ++cit) {
            OperationCollector::CsID csId = oc->getCsID(*cit);
            csIds.insert(csId);
        }
    }
}


/*!
 * Identify all the Functions that are reachable from a given root.
 * @param cg the input CallGraph
 * @param root the input root Function
 * @param funcs the output Function set
 */
template<typename FuncSet>
void identifyReachableCallGraphNodes(const PTACallGraph *cg,
        const llvm::Function *root, FuncSet &funcs) {
    if (!root)  return;
    std::stack<const PTACallGraphNode *> worklist;
    worklist.push(cg->getCallGraphNode(root));
    while (!worklist.empty()) {
        const PTACallGraphNode *cgNode = worklist.top();
        worklist.pop();
        const llvm::Function *F = cgNode->getFunction();
        if (funcs.count(F))     continue;
        funcs.insert(F);

        for (auto it = cgNode->OutEdgeBegin(), ie = cgNode->OutEdgeEnd();
                it != ie; ++it) {
            PTACallGraphNode::EdgeType *edge = (*it);
            const PTACallGraphNode *dstNode = edge->getDstNode();
            const llvm::Function *callee = dstNode->getFunction();
            if (funcs.count(callee))    continue;
            worklist.push(dstNode);
        }
    }
}


/*!
 * Identify all the Functions that are backward reachable from a given root.
 * @param cg the input CallGraph
 * @param root the input root Function
 * @param funcs the output Function set
 */
template<typename FuncSet>
void identifyBackwardReachableCallGraphNodes(const PTACallGraph *cg,
        const llvm::Function *F, FuncSet &funcs) {
    if (!F)  return;
    std::stack<const PTACallGraphNode *> worklist;
    worklist.push(cg->getCallGraphNode(F));
    while (!worklist.empty()) {
        const PTACallGraphNode *cgNode = worklist.top();
        worklist.pop();
        const llvm::Function *F = cgNode->getFunction();
        if (funcs.count(F))     continue;
        funcs.insert(F);

        for (auto it = cgNode->InEdgeBegin(), ie = cgNode->InEdgeEnd();
                it != ie; ++it) {
            PTACallGraphNode::EdgeType *edge = (*it);
            const PTACallGraphNode *srcNode = edge->getSrcNode();
            const llvm::Function *caller = srcNode->getFunction();
            if (funcs.count(caller))    continue;
            worklist.push(srcNode);
        }
    }
}


/// Get the pointer operand of a LoadInst or a StoreInst
inline const llvm::Value *getPointerValueOfLoadStore(
        const llvm::Instruction *I) {
    const llvm::Value *p = NULL;
    if (const llvm::LoadInst *LI = llvm::dyn_cast<llvm::LoadInst>(I)) {
        p = LI->getPointerOperand();
    } else if (const llvm::StoreInst *SI = llvm::dyn_cast<llvm::StoreInst>(I)) {
        p = SI->getPointerOperand();
    }
    return p;
}


/// Check if Function "F" is memset
inline bool isMemset(const llvm::Function *F) {
    return F && (F->getName().find("llvm.memset") == 0);
}


/// Check if Function "F" is  free
inline bool isFree(const llvm::Function *F) {
    return F && (F->getName().find("free") == 0);
}


/// Check if Function "F" is memcpy
inline bool isMemcpy(const llvm::Function *F) {
    return F && ExtAPI::EFT_L_A0__A0R_A1R == ExtAPI::getExtAPI()->get_type(F);
}


/// Determine if Instruction "I" is a memory access operation.
bool isMemoryAccess(const llvm::Instruction *I);


/// Get the i-th argument of a CallSite
inline const llvm::Value *getArgument(const llvm::ImmutableCallSite cs,
        unsigned int i) {
    assert(cs.getNumArgOperands() > i);
    const llvm::Value *V = cs.getArgOperand(i);
    return stripAllCasts(V);
}


/// Get lockPtr for Instruction "I", where "I" must be a lock/unlock call
inline const llvm::Value *getLockPtr(const llvm::Instruction *I) {
    const llvm::ImmutableCallSite cs(I);
    if (!cs)    return NULL;
    return getArgument(cs, 0);
}


/// Get barrierPtr for Instruction "I", where "I" must be a barrier_wait call
inline const llvm::Value *getBarrierPtr(const llvm::Instruction *I) {
    const llvm::ImmutableCallSite cs(I);
    if (!cs)    return NULL;
    return getArgument(cs, 0);
}


/// Check if Function "F" is a pthread API.
inline bool isPthreadApi(const llvm::Function *F) {
    return F && F->getName().find("pthread_") == 0;
}


/// Check if Function "F" is a hare API.
inline bool isHareApi(const llvm::Function *F) {
    return F && F->getName().find("hare_") == 0;
}


/// Check if it is not an interested thread API.
bool isNotInterestedThreadApi(const llvm::Function *F);


/// Check if it is a thread API call.
inline bool isMissHandledThreadApiCall(const llvm::Instruction *I) {
    const llvm::Function *F = analysisUtil::getCallee(I);
    return (isPthreadApi(F) || isHareApi(F)) && !isNotInterestedThreadApi(F);
}


/// Check if "p" points to a singleton memory object.
inline bool isSingletonMemObj(const llvm::Value *p, PointerAnalysis *pta) {
    PAG *pag = pta->getPAG();
    NodeID n = pag->getValueNode(p);
    PointsTo &pts = pta->getPts(n);
    // "p" must points to exactly one object.
    if (pts.count() != 1)   return false;

    // The object must not be array or heap or local variable in a recursive function.
    NodeID objId = pts.find_first();
    if (pta->isArrayMemObj(objId))  return false;
    if (pta->isHeapMemObj(objId))   return false;
    if (pta->isLocalVarInRecursiveFun(objId))   return false;
    return true;
}


/// Check if an object is a gep object
inline bool isGepObj(NodeID obj, PAG *pag) {
    PAGNode *pn = pag->getPAGNode(obj);
    return llvm::dyn_cast<GepObjPN>(pn);
}


/// Check if an object is a Function object
inline bool isFuncObj(NodeID obj, PAG *pag) {
    PAGNode *pn = pag->getPAGNode(obj);
    const llvm::Value *v = pn->getValue();
    return llvm::dyn_cast<llvm::Function>(v);
}


/// Check if an object is a dummy object
inline bool isDummyObj(NodeID obj, PAG *pag) {
    PAGNode *pn = pag->getPAGNode(obj);
    return pn->getNodeKind() == PAGNode::DummyObjNode;
}


/// Check if an object is the first field of a struct.
inline bool isFirstFieldObj(NodeID obj, PAG *pag) {
    // Check if the object is the entire struct object.
    if (pag->getBaseObjNode(obj) == obj)    return true;;

    // Check if the object is exactly the first field.
    PAGNode *pn = pag->getPAGNode(obj);
    assert(pn && "pagNode must not be NULL.");
    GepObjPN *gepObjPN = llvm::dyn_cast<GepObjPN>(pn);
    assert(gepObjPN && "This must be a gepObj");
    Size_t offset = gepObjPN->getLocationSet().getOffset();
    if (0 == offset)    return true;
    return false;
}


/// Get the field offset of a GepObjNode.
/// Return 0 if n is not a GepObjNode.
inline int getFieldObjOffset(NodeID n, PAG *pag) {
    // Check if the object is exactly the first field.
    PAGNode *pn = pag->getPAGNode(n);
    assert(pn && "pagNode must not be NULL.");
    GepObjPN *gepObjPN = llvm::dyn_cast<GepObjPN>(pn);
    if (!gepObjPN)  return -1;
    Size_t offset = gepObjPN->getLocationSet().getOffset();
    return offset;
}


/// Given an object node, find its field object node
inline NodeID getGepObjNode(NodeID id, const LocationSet& ls, PAG *pag) {
    PAGNode* node = pag->getPAGNode(id);
    if (GepObjPN* gepNode = llvm::dyn_cast<GepObjPN>(node)) {
        const MemObj* memObj = gepNode->getMemObj();
        return pag->getGepObjNode(memObj, gepNode->getLocationSet() + ls);
    } else if (FIObjPN* baseNode = llvm::dyn_cast<FIObjPN>(node)) {
        return pag->getGepObjNode(baseNode->getMemObj(), ls);
    } else {
        return 0;
    }
}


/// Get the transformed predicate of a given one.
/// @param p the input predicate
/// @return the transformed predicate
//@{
llvm::CmpInst::Predicate getMirrorPredicate(llvm::CmpInst::Predicate p);
llvm::CmpInst::Predicate getInversePredicate(llvm::CmpInst::Predicate p);
//@}


/// Get the string from a given predicate.
std::string getPredicateString(llvm::CmpInst::Predicate p);


/// Check if two ICMP predicate are sign-insensitively equal.
bool areSignInsensitivelyEqualIcmpPredicate(llvm::CmpInst::Predicate p1,
        llvm::CmpInst::Predicate p2);


/// Get all field-insensitive field objects of every input object.
/// @param pag the input PAG
/// @param inPts the input objects
/// @param outPts the output objects containing all field-insensitive fields of the input
inline void expandFIObjects(PAG *pag, PointsTo &inPts, PointsTo &outPts) {
    outPts = inPts;
    for (PointsTo::iterator it = inPts.begin(), ie = inPts.end(); it != ie;
            ++it) {
        if (pag->getBaseObjNode(*it) == *it) {
            outPts |= pag->getAllFieldsObjNode(*it);
        }
    }
}


/// Alias wrappers with an option of field-sensitivity matching.
//@{
llvm::AliasResult alias(NodeID node1, NodeID node2,
        PointerAnalysis *pta, bool considerFIObjs=false);

inline llvm::AliasResult alias(const llvm::Value *p1,
        const llvm::Value *p2, PointerAnalysis *pta,
        bool considerFIObjs=false) {
    PAG *pag = pta->getPAG();
    NodeID n1 = pag->getValueNode(p1);
    NodeID n2 = pag->getValueNode(p2);
    return alias(n1, n2, pta, considerFIObjs);
}
//@}


/// Dump the points-to set of a given node.
void printPts(NodeID n, PointerAnalysis *pta);

}

#endif /* RCUTIL_H_ */
