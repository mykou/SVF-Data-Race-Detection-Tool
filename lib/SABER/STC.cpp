/*
 * STC.cpp
 *
 *  Created on: 8 Sep 2016
 *      Author: hua
 */
#include "SABER/STC.h"

using namespace llvm;
using namespace std;
using namespace analysisUtil;

static cl::opt<bool> DumpUAFLineNum("mb", cl::init(false), cl::desc("dump details for Micro Benchmarks"));
static cl::opt<bool> ValidateUAF("valid-uaf", cl::init(false), cl::desc("Validate use-after-free"));

void STC::stage0()
{
    outs()<<"stage 0 begins  ";
    stat.t_b_s0 = stat.getClk();
    StmtItemSet srcSet;
    getSrcSet(srcSet);
    for (StmtItemSet::iterator I = srcSet.begin(); I != srcSet.end(); ++I) {
        const StmtItem& src = *I;
        StmtItemSet snkSet;
        getSnkSet(src, snkSet);
        for (StmtItemSet::iterator J = snkSet.begin(); J != snkSet.end(); ++J) {
            const StmtItem& snk = *J;
            if (isAliasByAndersen(src, snk))
                s0_out[src].insert(snk);
        }
    }
    stat.t_e_s0 = stat.getClk();
    outs()<<"finished\n";
}

void STC::getSrcSet(StmtItemSet& srcSet)
{
    const PAG::CallSiteSet& callSiteSet = getPAG()->getCallSiteSet();
    for (PAG::CallSiteSet::iterator I = callSiteSet.begin(); I != callSiteSet.end(); ++I) {
        const CallSite& cs = *I;
        if (const Function* callee = cs.getCalledFunction()) {
            if (callee->getName().str().compare("free") == 0) {
                Value* arg = cs.getArgument(0);
                assert(isa<PointerType>(arg->getType()) && "expecting pointer type for argument in free()");
                NodeID vid = getPAGNodeID(arg);
                if (!getAnderPts(vid).empty()) {
                    StmtItem src(vid, cs.getInstruction());
                    srcSet.insert(src);
                }
            }
        }
    }
}

void STC::getSnkSet(const StmtItem& src, StmtItemSet& snkSet)
{
    InstWorkList worklist;
    InstSet visited_backward;
    InstSet visited_forward;
    worklist.push(src.getLoc());
    visited_backward.insert(src.getLoc());
    while(!worklist.empty()) {
        const Instruction* src = worklist.pop();
        forwardSearch(src, snkSet, visited_forward, visited_backward);
        backwardSearch(src, worklist, visited_backward);
    }
}

void STC::forwardSearch(const Instruction* src, StmtItemSet& snkSet, InstSet& visited_forward, InstSet& visited_backward)
{
    InstWorkList worklist;
    worklist.push(src);
    visited_forward.insert(src);
    //visited_forward.insert(&(src->getParent()->front())); //to avoid recurrsion
    while (!worklist.empty()) {
        const Instruction* curInst = worklist.pop();
        //outs() << "handling inst @ line: "<< analysisUtil::getSourceLoc(curInst) << "\n"; curInst->dump();
        if (const LoadInst* load = dyn_cast<LoadInst>(curInst)) {
            StmtItem snk(getPAGNodeID(load->getPointerOperand()), load);
            snkSet.insert(snk);
        } else if (const StoreInst* store = dyn_cast<StoreInst>(curInst)) {
            StmtItem snk(getPAGNodeID(store->getPointerOperand()), store);
            snkSet.insert(snk);
        } else if (const CallInst* call = dyn_cast<CallInst>(curInst)) {
            int pos = getExtCallPos(call->getCalledFunction());
            if (pos != -1) {
                StmtItem snk(getPAGNodeID(call->getOperand(pos)), call);
                snkSet.insert(snk);
            }
        }

        addNextInstToWorklist(curInst, worklist, visited_forward, visited_backward);
    }
}

void STC::addNextInstToWorklist(const Instruction* curInst, InstWorkList& worklist, InstSet& visited_forward, InstSet& visited_backward)
{
    const Instruction* nxtInst = 0;
    if (!curInst->isTerminator()) {
        nxtInst = curInst->getNextNode();
        worklist.push(nxtInst);
        if (const CallInst* call = dyn_cast<CallInst>(curInst)) {
            CallSite cs(const_cast<CallInst*>(call));
            if (!getAnderCG()->hasCallGraphEdge(call)) return;
            for (PTACallGraph::CallGraphEdgeSet::const_iterator I = getAnderCG()->getCallEdgeBegin(call); I != getAnderCG()->getCallEdgeEnd(call); ++I) {
                const PTACallGraphEdge *callEdge = *I;
                const Function *callee = callEdge->getDstNode()->getFunction();
                if (!callee->isIntrinsic() && !callee->empty() && getExtCallPos(callee) == -1) {
                    nxtInst = callee->getEntryBlock().getFirstNonPHI();
                    if (visited_forward.find(nxtInst) == visited_forward.end()) {
                        visited_forward.insert(nxtInst);
                        visited_backward.insert(call);
                        worklist.push(nxtInst);
                        nxtInst->dump();
                    }
                }
            }
        }
    }
    else {
        const BasicBlock *BB = curInst->getParent();
        for (succ_const_iterator I = succ_begin(BB); I != succ_end(BB); ++I) {
            const BasicBlock* sucBB = *I;
            nxtInst = &(sucBB->front());
            if (visited_forward.find(nxtInst) == visited_forward.end()) {
                visited_forward.insert(nxtInst);
                worklist.push(nxtInst);
            }
        }
    }
}

void STC::backwardSearch(const Instruction* src, InstWorkList& worklist, InstSet& visited_backward)
{
    const Function* srcFun = src->getParent()->getParent();
    PTACallGraphEdge::CallInstSet dirCallSet;
    getAnderCG()->getDirCallSitesInvokingCallee(srcFun, dirCallSet);
    for (PTACallGraphEdge::CallInstSet::iterator I = dirCallSet.begin(); I != dirCallSet.end(); ++I) {
        if(const CallInst* call = dyn_cast<CallInst>(*I)) {
            CallSite cs(const_cast<CallInst*>(call));
            if (visited_backward.find(call) == visited_backward.end()) {
                visited_backward.insert(call);
                worklist.push(call);
            }
        }
    }
    PTACallGraphEdge::CallInstSet indCallSet;
    getAnderCG()->getIndCallSitesInvokingCallee(srcFun, indCallSet);
    for (PTACallGraphEdge::CallInstSet::iterator I = indCallSet.begin(); I != indCallSet.end(); ++I) {
        if(const CallInst* call = dyn_cast<CallInst>(*I)) {
            CallSite cs(const_cast<CallInst*>(call));
            if (visited_backward.find(call) == visited_backward.end()) {
                visited_backward.insert(call);
                worklist.push(call);
            }
        }
    }
}


void STC::output()
{
    cout << "NUM_QUERY_ANDERSEN = " << stat.n_query_ander <<"\n";
    cout << "TIME_S0    = " << setw(6) << (stat.t_e_s0 - stat.t_b_s0) / TIMEINTERVAL <<" s\n";
    cout << flush;

    unsigned s0size = 0;
    for (FS_Result::iterator I = s0_out.begin(); I != s0_out.end(); ++I) {
        for (StmtItemSet::iterator J = I->second.begin(); J != I->second.end(); ++J) {
            ++s0size;
            if (DumpUAFLineNum) {
                const StmtItem& src = I->first;
                const StmtItem& snk = *J;
                const Value* srcVal = getValue(src.getCurNodeID());
                const Value* snkVal = getValue(snk.getCurNodeID());
                outs() << "UAF_" << s0size << ":\n";
                outs() << "   src : ";
                src.getLoc()->dump();
                outs() << "           " << analysisUtil::getSourceLoc(srcVal) << "\n";
                outs() << "   snk : ";
                snk.getLoc()->dump();
                outs() << "           " << analysisUtil::getSourceLoc(snkVal) << "\n";
            }
        }
    }
    outs() << "Num UAF (stage 0):  " << s0size << "\n";
}

/*!
 * Validate test cases for regression test purpose
 */
void STC::validateTests() {
    validateExpectedTruePositive("UAF_TP");
    validateExpectedFalsePositive("UAF_FP");
    validateExpectedTrueNegative("UAF_TN");
    validateExpectedFalseNegative("UAF_FN");
}

void STC::validateExpectedTruePositive(const char* fun)
{
    if (Function* checkFun = getModule()->getFunction(fun)) {
        if(!checkFun->use_empty())
            outs() << "[STC] Checking Expected True Positives" << fun << "\n";

        for (Value::user_iterator i = checkFun->user_begin(), e = checkFun->user_end(); i != e; ++i)
            if (CallInst *call = dyn_cast<CallInst>(*i)) {
                assert(call->getNumArgOperands() == 2 && "arguments should be two line numbers!!");
                ConstantInt *line_free = dyn_cast<ConstantInt>(call->getArgOperand(0));
                ConstantInt *line_use = dyn_cast<ConstantInt>(call->getArgOperand(1));
                assert(line_free && line_use && "null value??");
                //The actual index
                u32_t lf = line_free->getSExtValue();
                u32_t lu = line_use->getSExtValue();

                outs()  << (isUAF(lf,lu) ? sucMsg("\t SUCCESS :") : errMsg("\t FAIL :"))
                		<< fun << " check <free_line:" << lf << ", use_line:" << lu << "\n";
            }
            else
                assert(false && "UAF check functions not only used at callsite??");
    }
}

void STC::validateExpectedFalsePositive(const char* fun)
{
    if (Function* checkFun = getModule()->getFunction(fun)) {
        if(!checkFun->use_empty())
            outs() << "[STC] Checking Expected False Positives" << fun << "\n";

        for (Value::user_iterator i = checkFun->user_begin(), e = checkFun->user_end(); i != e; ++i)
            if (CallInst *call = dyn_cast<CallInst>(*i)) {
                assert(call->getNumArgOperands() == 2 && "arguments should be two line numbers!!");
                ConstantInt *line_free = dyn_cast<ConstantInt>(call->getArgOperand(0));
                ConstantInt *line_use = dyn_cast<ConstantInt>(call->getArgOperand(1));
                assert(line_free && line_use && "null value??");
                //The actual index
                u32_t lf = line_free->getSExtValue();
                u32_t lu = line_use->getSExtValue();

                outs()  << (isUAF(lf,lu) ? sucMsg("\t SUCCESS :") : errMsg("\t FAIL :"))
                		<< fun << " check <free_line:" << lf << ", use_line:" << lu << "\n";
            }
            else
                assert(false && "UAF check functions not only used at callsite??");
    }
}

void STC::validateExpectedTrueNegative(const char* fun)
{
    if (Function* checkFun = getModule()->getFunction(fun)) {
        if(!checkFun->use_empty())
            outs() << "[STC] Checking Expected True Negatives" << fun << "\n";

        for (Value::user_iterator i = checkFun->user_begin(), e = checkFun->user_end(); i != e; ++i)
            if (CallInst *call = dyn_cast<CallInst>(*i)) {
                assert(call->getNumArgOperands() == 2 && "arguments should be two line numbers!!");
                ConstantInt *line_free = dyn_cast<ConstantInt>(call->getArgOperand(0));
                ConstantInt *line_use = dyn_cast<ConstantInt>(call->getArgOperand(1));
                assert(line_free && line_use && "null value??");
                //The actual index
                u32_t lf = line_free->getSExtValue();
                u32_t lu = line_use->getSExtValue();

                outs()  << (isUAF(lf,lu) ? errMsg("\t FAIL :") : sucMsg("\t SUCCESS :"))
                		<< fun << " check <free_line:" << lf << ", use_line:" << lu << "\n";
            }
            else
                assert(false && "UAF check functions not only used at callsite??");
    }
}

void STC::validateExpectedFalseNegative(const char* fun)
{
    if (Function* checkFun = getModule()->getFunction(fun)) {
        if(!checkFun->use_empty())
            outs() << "[STC] Checking Expected False Negatives" << fun << "\n";

        for (Value::user_iterator i = checkFun->user_begin(), e = checkFun->user_end(); i != e; ++i)
            if (CallInst *call = dyn_cast<CallInst>(*i)) {
                assert(call->getNumArgOperands() == 2 && "arguments should be two line numbers!!");
                ConstantInt *line_free = dyn_cast<ConstantInt>(call->getArgOperand(0));
                ConstantInt *line_use = dyn_cast<ConstantInt>(call->getArgOperand(1));
                assert(line_free && line_use && "null value??");
                //The actual index
                u32_t lf = line_free->getSExtValue();
                u32_t lu = line_use->getSExtValue();

                outs()  << (isUAF(lf,lu) ? errMsg("\t FAIL :") : sucMsg("\t SUCCESS :"))
                		<< fun << " check <free_line:" << lf << ", use_line:" << lu << "\n";
            }
            else
                assert(false && "UAF check functions not only used at callsite??");
    }
}

bool STC::isUAF(u32_t free_line_num, u32_t use_line_num)
{
	for (FS_Result::iterator I = s0_out.begin(); I != s0_out.end(); ++I) {
		unsigned srcLine = 0, snkLine = 0;
		const StmtItem& src = I->first;
		const Instruction* srcInst = src.getLoc();
		if (MDNode *md = srcInst->getMetadata("dbg")) {
			DILocation* srcLoc = cast<DILocation>(md);     // DILocation is in DebugInfo.h
			srcLine = srcLoc->getLine();
		}
		if (free_line_num != srcLine) continue;
		for (StmtItemSet::iterator J = I->second.begin(); J != I->second.end(); ++J) {
			const StmtItem& snk = *J;
			const Instruction* snkInst = snk.getLoc();
			if (MDNode *md = snkInst->getMetadata("dbg")) {
				DILocation* snkLoc = cast<DILocation>(md);     // DILocation is in DebugInfo.h
				snkLine = snkLoc->getLine();
			}
			if (use_line_num == snkLine) return true;
		}
	}
	return false;
}
