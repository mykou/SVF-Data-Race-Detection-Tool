/*
 * RCUtil.cpp
 *
 *  Created on: 20/10/2014
 *      Author: dye
 */

#include "RCUtil.h"
#include "MemoryPartitioning.h"
#include "MhpAnalysis.h"
#include "Util/ThreadCallGraph.h"
#include "MemoryModel/PointerAnalysis.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Transforms/Utils/Local.h>    // for FindAllocaDbgDeclare
#include <sstream>

using namespace llvm;
using namespace std;
using namespace analysisUtil;
using namespace rcUtil;

extern cl::opt<bool> RcDetail;
extern cl::opt<bool> RcHandleFree;

/*
 * Dump MemoryAccess information
 */
void MemoryAccess::print() const {
    outs() << " --- MemoryAccess ---\n";
    outs() << rcUtil::getSourceLoc(I) << "\t";
    string s = isWrite ? "write" : "read";
    outs() << s << " access dereferencing the pointer "
            << rcUtil::getSourceLoc(p) << "\n";
}


OperationCollector *OperationCollector::singleton = NULL;

/*
 * Initialization
 */
void OperationCollector::init(Module *M) {
    this->M = M;
}


/*
 * Collect interested operations
 */
void OperationCollector::analyze() {
    assert(M && "OperationCollector uninitialised!\n");

    for (Module::const_iterator it = M->begin(), ie = M->end(); it != ie; ++it) {
        const Function *F = &*it;
        if (F->isDeclaration())     continue;

        MemoryAccess ma;
        for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            const Instruction *inst = const_cast<Instruction*>(&*I);
            // LoadInst
            if (const LoadInst *LI = dyn_cast<LoadInst>(inst)) {
                ma.reset(LI, LI->getPointerOperand(), false);
                reads.push_back(ma);

            // StoreInst
            } else if (const StoreInst *SI = dyn_cast<StoreInst>(inst)) {
                ma.reset(SI, SI->getPointerOperand(), true);
                writes.push_back(ma);

            // Lock acquire and release
            } else if (isLockAquireCall(inst) || isLockReleaseCall(inst)) {
                locksUnlocks.push_back(inst);

            // Thread spawn sites
            } else if (isThreadForkCall(inst) || isThreadJoinCall(inst)) {
                spawnsJoins.push_back(inst);

            // Barrier wait sites
            } else if (isBarrierWaitCall(inst)) {
                barriers.push_back(inst);

            // hare_parallel_for sites
            } else if (isHareParForCall(inst)) {
                hareParFors.push_back(inst);

            // The unhandled pthread APIs
            } else if (rcUtil::isMissHandledThreadApiCall(inst)) {
                if (RcDetail) {
                    outs() << bugMsg1("OperationCollector")
                            << ": thread API not handled\t"
                            << rcUtil::getSourceLoc(inst) << "\n";
                }

            // memset and memcpy
            } else if (const Function *callee = getCallee(inst)) {
                if (isExtCall(callee)) {
                    const Value *readPtr = NULL;
                    const Value *writePtr = NULL;
                    bool memset = isMemset(callee) || (RcHandleFree && isFree(callee));
                    if (memset) {
                        writePtr = analysisUtil::stripAllCasts(inst->getOperand(0));
                    } else if (isMemcpy(callee)) {
                        writePtr = inst->getOperand(0);
                        readPtr = inst->getOperand(1);
                    }

                    if (readPtr) {
                        ma.reset(inst, readPtr, false);
                        reads.push_back(ma);
                    }
                    if (writePtr) {
                        ma.reset(inst, writePtr, true);
                        writes.push_back(ma);
                    }
                }
            }

            // Maintain mapping between csInst to csID
            if (analysisUtil::isCallSite(inst)) {
                // We do not record intrinsic Functions
                const Function *callee = getCallee(inst);
                if (callee && callee->isIntrinsic())  {
                    continue;
                }
                addCsInst(inst);
            }
        }
    }
}


/*
 * Dump the information of collected operations
 */
void OperationCollector::print() const {
    outs() << pasMsg(" --- Operations ---\n");
    outs() << "No. of reads: \t" << getReads().size() << "\n";
    outs() << "No. of writes: \t" << getWrites().size() << "\n";
    outs() << "No. of spawns/joins: \t" << getSpawnsJoins().size() << "\n";
    outs() << "No. of locks/unlocks: \t" << getLocksUnlocks().size() << "\n";
    outs() << "No. of barrier waits: \t" << getBarriers().size() << "\n";
    outs() << "No. of hare ParFor: \t" << getHareParFors().size() << "\n";
    outs() << "\n";
}


/*
 * Initialization
 */
void DomFrontier::init(const Function *F) {
    DominatorTree &dt = FunctionPassPool::getDt(F);
    Base.analyze(dt);
}


/*
 * Determine if Instruction "I" is a memory access operation.
 */
bool rcUtil::isMemoryAccess(const Instruction *I) {
    if (isa<LoadInst>(I) || isa<StoreInst>(I))  return true;
    if (const Function *callee = analysisUtil::getCallee(I)) {
        if (isMemcpy(callee))   return true;
        if (isMemset(callee))   return true;
        if (RcHandleFree && isFree(callee))  return true;
    }
    return false;
}


/*
 * Get the meta data here for line number and file name
 */
string rcUtil::getSourceLoc(const Value *v) {
    if (NULL == v)  return "empty val";

    string str;
    StringRef File = "UnknownFile";
    unsigned Line = 0;
    raw_string_ostream rawstr(str);

    // Set default file:line as the Instruction's file:line
    const Instruction *I = dyn_cast<Instruction>(v);
    const Value *v_ = v;
    while (!I && !v_->user_empty()) {
        const User *user = *v_->user_begin();
        if (user == v_->user_back()) {
            I = dyn_cast<Instruction>(user);
        }
        v_ = user;
    }
    if (I) {
        if (MDNode *N = I->getMetadata("dbg")) {
            DILocation *Loc = cast<DILocation>(N);
            Line = Loc->getLine();
            File = Loc->getFilename();
        }
    }

    // Strip all casts of v
    v = stripAllCasts(v);

    // Gather relevant information
    if (const Instruction *inst = dyn_cast<Instruction>(v)) {
        switch (inst->getOpcode()) {
        case Instruction::Call: {
            const CallInst *CI = cast<const CallInst>(inst);
            const Value *calledValue = CI->getCalledValue();
            string callee;
            if (isa<Function>(calledValue)) {
                callee = calledValue->getName();
            } else if (const LoadInst *LI = dyn_cast<LoadInst>(calledValue)) {
                callee = "*";
                callee += LI->getPointerOperand()->getName();
            }
            rawstr << "Call: " << callee << " ";
            break;
        }
        case Instruction::Invoke: {
            const InvokeInst *CI = cast<const InvokeInst>(inst);
            const Value *calledValue = CI->getCalledValue();
            string callee;
            if (isa<Function>(calledValue)) {
                callee = calledValue->getName();
            } else if (const LoadInst *LI = dyn_cast<LoadInst>(calledValue)) {
                callee = "*";
                callee += LI->getPointerOperand()->getName();
            }
            rawstr << "Invoke: " << callee << " ";
            break;
        }
        case Instruction::Load: {
            const LoadInst *LI = dyn_cast<LoadInst>(inst);
            const Value *p = stripAllCasts(LI->getPointerOperand());
            rawstr << "Load: " << p->getName() << " ";
            break;
        }
        case Instruction::Store: {
            const StoreInst *SI = dyn_cast<StoreInst>(inst);
            const Value *p = stripAllCasts(SI->getPointerOperand());
            rawstr << "Store: " << p->getName() << " ";
            break;
        }
        case Instruction::Alloca: {
            rawstr << "Alloc: " << inst->getName() << " ";
            break;
        }
        default: {
            rawstr << "I: " << inst->getName() << " ";
            break;
        }
        }
        if (isa<AllocaInst>(inst)) {
            DbgDeclareInst* DDI = FindAllocaDbgDeclare(
                    const_cast<Instruction*>(inst));
            if (DDI) {
                DIVariable *DIVar = cast<DIVariable>(DDI->getVariable());
                File = DIVar->getFilename();
                Line = DIVar->getLine();
            }
        } else if (MDNode *N = inst->getMetadata("dbg")) { // Here I is an LLVM instruction
            DILocation *Loc = cast<DILocation>(N);         // DILocation is in DebugInfo.h
            Line = Loc->getLine();
            File = Loc->getFilename();
        }
        rawstr << "(" << File << ":" << Line << ")";

    } else if (const Argument* argument = dyn_cast<Argument>(v)) {
        if (argument->getArgNo() % 10 == 1)
            rawstr << argument->getArgNo() << "st";
        else if (argument->getArgNo() % 10 == 2)
            rawstr << argument->getArgNo() << "nd";
        else if (argument->getArgNo() % 10 == 3)
            rawstr << argument->getArgNo() << "rd";
        else
            rawstr << argument->getArgNo() << "th";
        rawstr << "Arg: " << argument->getParent()->getName() << " "
                << getSourceLoc(argument->getParent());

    } else if (const GlobalVariable* gvar = dyn_cast<GlobalVariable>(v)) {
        rawstr << "Glob: " << gvar->getName() << " ";
        MDNode *N = gvar->getMetadata("dbg");
        NamedMDNode* CU_Nodes = gvar->getParent()->getNamedMetadata(
                "llvm.dbg.cu");
        if (N && CU_Nodes) {
            for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
                DICompileUnit *CUNode = cast<DICompileUnit>(CU_Nodes->getOperand(i));
                for (DIGlobalVariableExpression *GVE : CUNode->getGlobalVariables()) {
                    DIGlobalVariable *GV = GVE->getVariable();
                    if (N == GV) {
                        Line = GV->getLine();
                        File = GV->getFilename();
                    }
                }
            }
        }
        rawstr << "(" << File << ":" << Line << ")";

    } else if (const Function* func = dyn_cast<Function>(v)) {
        if (DISubprogram *SP = func->getSubprogram()) {
            assert(SP->describes(func));
            rawstr << "Func: " << func->getName() << ": " << SP->getFilename()
                    << ": " << SP->getLine();
        }

    } else {
        rawstr << "Can only get source location for instruction, argument, "
                "global var or function.";
    }

    return rawstr.str();
}


/*
 * Embed offset information into source loc string
 */
static string embedOffsetInfo(string &sourceLoc, Size_t offset) {
    vector<string> strs;
    rcUtil::split(sourceLoc, '(', strs);
    if (2 != strs.size()) {
        return sourceLoc;
    }

    raw_string_ostream rawstr(sourceLoc);
    sourceLoc.clear();
    strs[0].pop_back();
    rawstr << strs[0] << "+" << offset << " (" << strs[1];
    return rawstr.str();
}


/*
 * Get the meta data here for line number and file name.
 */
string rcUtil::getSourceLoc(NodeID objId) {
    // Get the base object meta data
    PAG *pag = PAG::getPAG();

    // Get the PAGNode
    PAGNode *pn = pag->getPAGNode(objId);
    assert(pn && "pagNode must not be NULL.");

    // Skip dummy nodes
    if (pn->getNodeKind() == PAGNode::DummyObjNode) {
        return "DummyObjPN";
    } else if (pn->getNodeKind() == PAGNode::DummyValNode) {
        return "DummyValPN";
    }

    // If it is not a MemObj, then print the value.
    const MemObj *memObj = pag->getObject(objId);
    if (!memObj) {
        const Value *v = pag->getPAGNode(objId)->getValue();
        return getSourceLoc(v);
    }

    const Value *v = memObj->getRefVal();
    if (!v) {
        return "LLVM Value of the object not found.";
    }
    string ret = rcUtil::getSourceLoc(v);

    // Check if it is a field of a struct
    GepObjPN *gepObjPN = llvm::dyn_cast<GepObjPN>(pn);
    if (gepObjPN) {
        // Embed the offset information into the output.
        Size_t offset = gepObjPN->getLocationSet().getOffset();
        ret = embedOffsetInfo(ret, offset);
    }

    return ret;
}


/*
 * Split std::string "s" by "delim", and store the substrings into "elems".
 */
vector<string> &rcUtil::split(const string &s, char delim,
        vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


/*
 * Add PAG Edges for a call.
 */
static void handleCallForPAG(PAG *pag, CallSite cs, const Function *F) {
    assert(F);

    // We only handle the ret.val. if it's used as a ptr.
    if (isa<PointerType>(cs.getType())) {
        NodeID dstrec = pag->getValueNode(cs.getInstruction());
        //Does it actually return a ptr?
        if (isa<PointerType>(F->getReturnType())) {
            NodeID srcret = pag->getReturnNode(F);
            pag->addRetEdge(srcret, dstrec, cs.getInstruction());
        } else {
            // This is a int2ptr cast during parameter passing
            pag->addBlackHoleAddrEdge(dstrec);
        }
    }

    // Connect the actual and formal parameters
    CallSite::arg_iterator itA = cs.arg_begin(), ieA = cs.arg_end();
    Function::const_arg_iterator itF = F->arg_begin(), ieF = F->arg_end();

    // Handle fixed parameters
    while (itF != ieF && itA != ieA) {
        const Argument *formalParameter = &*itF;
        const Value *actualParameter = *itA;

        if (isa<PointerType>(formalParameter->getType())) {
            // Connect the parameters
            NodeID dst = pag->getValueNode(formalParameter);

            // Add PAGEgdes for the parameters
            if (isa<PointerType>(actualParameter->getType())) {
                NodeID src = pag->getValueNode(actualParameter);
                pag->addCallEdge(src, dst, cs.getInstruction());
            } else {
                // This is a int2ptr cast during parameter passing
                pag->addFormalParamBlackHoleAddrEdge(dst, formalParameter);
            }
        }

        // Increment iterators
        ++itF;
        ++itA;
    }

    // Handle the rest actual parameters (i.e., the varargs case)
    if (F->isVarArg()) {
        NodeID dst = pag->getVarargNode(F);
        while (itA != ieA) {
            const Value *actualParameter = *itA;
            if (isa<PointerType>(actualParameter->getType())) {
                NodeID src = pag->getValueNode(actualParameter);
                pag->addCallEdge(src, dst, cs.getInstruction());
            } else {
                // This is a int2ptr cast during parameter passing
                // pag->addBlackHoleAddrEdge(vaF);
            }

            // Increment iterator
            ++itA;
        }
    }
}


/*
 * Update PAG by applying ThreadCallGraph to solve indirect calls.
 */
void rcUtil::updatePAG(PAG *pag, ThreadCallGraph *tcg) {
    // Handle non-thread-spawning indirect call sites
    auto &indirectCallsites = pag->getIndirectCallsites();
    for (auto it = indirectCallsites.begin(), ie = indirectCallsites.end();
            it != ie; ++it) {
        CallSite cs = it->first;
        const Instruction *csInst = cs.getInstruction();
        if (!tcg->hasCallGraphEdge(csInst))     continue;

        for (auto it = tcg->getCallEdgeBegin(csInst),
                ie = tcg->getCallEdgeEnd(csInst); it != ie; ++it) {
            PTACallGraphEdge *edge = *it;

            // Skip the callee that does not have a definition
            const Function *callee = edge->getDstNode()->getFunction();
            if (callee->isDeclaration())    continue;
            handleCallForPAG(pag, cs, callee);
        }
    }

    // Collect thread-spawning indirect call sites.
    vector<const Instruction*> spawnSites;
    for (auto it = tcg->forksitesBegin(), ie = tcg->forksitesEnd();
            it != ie; ++it) {
        const Instruction *spawnSite = *it;

        // Skip the direct calls
        const Value* forkedValue = getForkedFun(spawnSite);
        if (dyn_cast<Function>(forkedValue) != NULL)   continue;

        // Add to spawnSites
        spawnSites.push_back(spawnSite);
    }
    for (auto it = tcg->parForSitesBegin(), ie = tcg->parForSitesEnd();
            it != ie; ++it) {
        const Instruction *spawnSite = *it;

        // Skip the direct calls
        const Value* forkedValue = getTaskFuncAtHareParForSite(spawnSite);
        if (dyn_cast<Function>(forkedValue) != NULL)   continue;

        // Add to spawnSites
        spawnSites.push_back(spawnSite);
    }

    // Handle thread-spawning indirect call sites.
    for (auto it = spawnSites.begin(), ie = spawnSites.end(); it != ie; ++it) {
        const Instruction *spawnSite = *it;

        // Get the thread spawning API function
        const Function *threadSpawningAPI = analysisUtil::getCallee(spawnSite);
        assert(threadSpawningAPI);

        // Get the actual parameter
        const Value *actualParameter = NULL;
        if (isThreadForkCall(spawnSite)) {
            actualParameter = getActualParmAtForkSite(spawnSite);
        } else if (isHareParForCall(spawnSite)) {
            actualParameter = getTaskDataAtHareParForSite(spawnSite);
        } else {
            assert(false);
        }
        NodeID src = pag->getValueNode(actualParameter);

        // Skip dummy nodes
        const PAGNode *pn = pag->getPAGNode(src);
        if (pn->getNodeKind() == PAGNode::DummyValNode
                || pn->getNodeKind() == PAGNode::DummyObjNode) {
            continue;
        }

        // Iterate the callees via the already updated ThreadCallGraph
        for (auto it = tcg->getCallEdgeBegin(spawnSite),
                ie = tcg->getCallEdgeEnd(spawnSite); it != ie; ++it) {
            PTACallGraphEdge *edge = *it;
            const Function *spawnee = edge->getDstNode()->getFunction();

            // Skip the thread spawning API function (e.g., pthread_create)
            if (spawnee == threadSpawningAPI)   continue;

            // Skip the spawnee that does not have a definition
            if (spawnee->isDeclaration())       continue;

            // We only handle the cases where there is 1 or 2 parameters
            if(spawnee->arg_size() > 2 && spawnee->arg_size() < 1)  continue;

            // Get the formal parameter and skip the non-ptr types
            const Argument* formalParameter = &(spawnee->getArgumentList().front());
            if (!isa<PointerType>(formalParameter->getType()))  continue;

            // Connect the parameters
            if (isa<PointerType>(actualParameter->getType())) {
                NodeID dst = pag->getValueNode(formalParameter);
                pag->addThreadForkEdge(src, dst, spawnSite);
            } else {
                // TODO: This is a int2ptr cast during parameter passing
                // pag->addThreadForkEdge(...);
            }
        }
    }
}


/*
 * Update PTACallGraph "cg" by applying "pta" to solve indirect calls.
 */
void rcUtil::updateCallGraph(PTACallGraph *cg, PointerAnalysis *pta) {
    PTACallGraph::CallEdgeMap::const_iterator iter = pta->getIndCallMap().begin();
    PTACallGraph::CallEdgeMap::const_iterator eiter = pta->getIndCallMap().end();
    for (; iter != eiter; iter++) {
        llvm::CallSite cs = iter->first;
        const Instruction *callInst = cs.getInstruction();
        const PTACallGraph::FunctionSet &functions = iter->second;
        for (PTACallGraph::FunctionSet::const_iterator func_iter =
                functions.begin(); func_iter != functions.end(); func_iter++) {
            const Function *callee = *func_iter;
            cg->addIndirectCallGraphEdge(callInst, callee);
        }
    }
}


/*
 * Check if a Function is a dead function (i.e. not reachable from main)
 */
bool rcUtil::isDeadFunction(const Function *F) {
    return InterproceduralAnalysisBaseStaticContainer::isDeadFunction(F);
}


/*
 * Check if it is not interested thread API.
 */
bool rcUtil::isNotInterestedThreadApi(const llvm::Function *F) {
    assert(F);
    static std::string apis[] = {"mutexattr_", "_init", "_destroy", "_finish"};
    for (int i = 0; i < 4; ++i) {
        if (F->getName().find(apis[i]) != llvm::StringRef::npos)   return true;
    }
    return false;
}


/*
 * Get the mirror predicate of a given one.
 */
CmpInst::Predicate rcUtil::getMirrorPredicate(CmpInst::Predicate p) {
    switch (p) {
    case CmpInst::ICMP_EQ:
        return CmpInst::ICMP_EQ;
    case CmpInst::ICMP_NE:
        return CmpInst::ICMP_NE;
    case CmpInst::ICMP_UGT:
        return CmpInst::ICMP_ULT;
    case CmpInst::ICMP_UGE:
        return CmpInst::ICMP_ULE;
    case CmpInst::ICMP_ULT:
        return CmpInst::ICMP_UGT;
    case CmpInst::ICMP_ULE:
        return CmpInst::ICMP_UGE;
    case CmpInst::ICMP_SGT:
        return CmpInst::ICMP_SLT;
    case CmpInst::ICMP_SGE:
        return CmpInst::ICMP_SLE;
    case CmpInst::ICMP_SLT:
        return CmpInst::ICMP_SGT;
    case CmpInst::ICMP_SLE:
        return CmpInst::ICMP_SGE;
    default:
        return CmpInst::BAD_ICMP_PREDICATE;
    }
}


/*
 * Get the inverse predicate of a given one.
 */
CmpInst::Predicate rcUtil::getInversePredicate(CmpInst::Predicate p) {
    switch (p) {
    case CmpInst::ICMP_EQ:
        return CmpInst::ICMP_NE;
    case CmpInst::ICMP_NE:
        return CmpInst::ICMP_EQ;
    case CmpInst::ICMP_UGT:
        return CmpInst::ICMP_ULE;
    case CmpInst::ICMP_UGE:
        return CmpInst::ICMP_ULT;
    case CmpInst::ICMP_ULT:
        return CmpInst::ICMP_UGE;
    case CmpInst::ICMP_ULE:
        return CmpInst::ICMP_UGT;
    case CmpInst::ICMP_SGT:
        return CmpInst::ICMP_SLE;
    case CmpInst::ICMP_SGE:
        return CmpInst::ICMP_SLT;
    case CmpInst::ICMP_SLT:
        return CmpInst::ICMP_SGE;
    case CmpInst::ICMP_SLE:
        return CmpInst::ICMP_SGT;
    default:
        return CmpInst::BAD_ICMP_PREDICATE;
    }
}


/*
 * Get the string from a given predicate.
 */
string rcUtil::getPredicateString(CmpInst::Predicate p) {
    switch (p) {
    case CmpInst::ICMP_EQ:
        return string("==");
    case CmpInst::ICMP_NE:
        return string("!=");
    case CmpInst::ICMP_UGT:
        return string(">");
    case CmpInst::ICMP_UGE:
        return string(">=");
    case CmpInst::ICMP_ULT:
        return string("<");
    case CmpInst::ICMP_ULE:
        return string("<=");
    case CmpInst::ICMP_SGT:
        return string(">");
    case CmpInst::ICMP_SGE:
        return string(">=");
    case CmpInst::ICMP_SLT:
        return string("<");
    case CmpInst::ICMP_SLE:
        return string("<=");
    default:
        return string("??");
    }
}


/*
 * Check if two ICMP predicate are sign-insensitively equal.
 */
bool rcUtil::areSignInsensitivelyEqualIcmpPredicate(CmpInst::Predicate p1,
        CmpInst::Predicate p2) {
    if (p1 == p2)   return true;

    // Swap if p1 is greater than p2
    if (p1 > p2) {
        CmpInst::Predicate tmp = p1;
        p1 = p2;
        p2 = tmp;
    }

    // Return false if at least one of those is not a ICMP predicate.
    if (p1 < CmpInst::FIRST_ICMP_PREDICATE)     return false;
    if (p2 > CmpInst::LAST_ICMP_PREDICATE)      return false;

    // Check if they are equal sign-insensitively.
    if (p2 >= CmpInst::ICMP_SGT && p2 <= CmpInst::ICMP_SLE) {
        return (p2 - p1) == (CmpInst::ICMP_SLE - CmpInst::ICMP_ULE);
    }

    return false;
}


/*
 * Alias wrappers with an option of field-sensitivity matching.
 */
AliasResult rcUtil::alias(NodeID node1, NodeID node2,
        PointerAnalysis *pta, bool considerFIObjs) {
    PointsTo& p1 = pta->getPts(node1);
    PointsTo& p2 = pta->getPts(node2);
    if (!considerFIObjs) {
        if (pta->containBlackHoleNode(p1) || pta->containBlackHoleNode(p2)
                || p1.intersects(p2))
            return llvm::MayAlias;
        else
            return llvm::NoAlias;
    }

    static PointsTo pts1;
    static PointsTo pts2;
    PAG *pag = pta->getPAG();
    pts1.clear();
    pts2.clear();
    expandFIObjects(pag, p1, pts1);
    expandFIObjects(pag, p2, pts2);
    if (pta->containBlackHoleNode(pts1) || pta->containBlackHoleNode(pts2)
            || pts1.intersects(pts2))
        return llvm::MayAlias;
    else
        return llvm::NoAlias;
}


/*
 * Dump the points-to set of a given node.
 */
void rcUtil::printPts(NodeID n, PointerAnalysis *pta) {
    PointsTo &pts = pta->getPts(n);
    outs() << " ------- Points-to Set of PAG node " << n << "\t";
    outs() << getSourceLoc(n) << "\n";
    outs() << " " << pts.count() << " objects:\n";
    for (NodeID o : pts) {
        outs() << "\t" << o << "\t" << getSourceLoc(o) << "\n";
    }
    outs() << "\n";
}

