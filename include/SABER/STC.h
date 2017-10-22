/*
 * STC.h
 *
 *  Created on: 8 Sep 2016
 *      Author: hua
 */

#ifndef INCLUDE_DDA_STC_H_
#define INCLUDE_DDA_STC_H_

#include "SABER/LeakChecker.h"
#include "DDA/FlowDDA.h"
#include "DDA/ContextDDA.h"
#include "DDA/PathDDA.h"
#include <iomanip>
#include <llvm/IR/DebugInfo.h>

using namespace llvm;
using namespace std;

struct Ext_Call {
    const char *name;
    unsigned pos; //position that may be use-after-free
};

static const Ext_Call ext_call[] = { { "strlen", 0 }, { "strcmp", 1 }, {"strstr", 1 }, {"uaf_use", 0 }, { 0, 0 } };

class STCStat
{
public:
    STCStat() : n_query_ander(0),
        t_b_s0(0),
        t_e_s0(0) {}

    static inline double getClk() {
        return CLOCK_IN_MS();
    }

    unsigned n_query_ander;
    double t_b_s0, t_e_s0;
};

class STC: public LeakChecker
{
public:
    typedef StmtDPItem<Instruction> StmtItem;
    typedef CxtStmtDPItem<Instruction> CxtStmtItem;
    typedef PathStmtDPItem<Instruction> PathStmtItem;
    typedef set<StmtItem> StmtItemSet;
    typedef set<CxtStmtItem> CxtStmtItemSet;
    typedef set<PathStmtItem> PathStmtItemSet;
    typedef FIFOWorkList<const Instruction*> InstWorkList;
    typedef FIFOWorkList<const BasicBlock*> BBWorkList;
    typedef FIFOWorkList<const Function*> FuncWorkList;
    typedef vector<const Instruction*> InstVec;
    typedef set<const Instruction*> InstSet;
    typedef set<const BasicBlock*> BBSet;
    typedef set<const Function*> FuncSet;
    typedef set<CallSiteID> CSIDSet;
    typedef map<StmtItem*, CSIDSet> SrcVFtoCSID;
    typedef set<pair<ContextCond, ContextCond>> CxtPairSet;
    typedef map<StmtItem, StmtItemSet> FS_Result;			//flow-sensitive result
    typedef map<CxtStmtItem, CxtStmtItemSet> CS_Result;	//context-sensitive result
    typedef map<PathStmtItem, PathStmtItemSet> PS_Result;	//path-sensitive result
    typedef set<const CallDirSVFGEdge*> SVFGCallDirEdgeSet;
    typedef set<const CallIndSVFGEdge*> SVFGCallIndEdgeSet;
    typedef map<CallSiteID, SVFGCallDirEdgeSet> CSIDToVFGCallDirEdgeSet;
    typedef map<CallSiteID, SVFGCallIndEdgeSet> CSIDToVFGCallIndEdgeSet;


    STC() :
        _ander(0),
        _svfg(0)
    {
        for (const Ext_Call *p = ext_call; p->name; ++p) {
            EXT_API[p->name] = p->pos;
        }
    }

    ~STC() { }

    void analyze(Module& module) {
        _ander = AndersenWaveDiff::createAndersenWaveDiff(module);
        SVFGBuilder svfgBuilder;
        _svfg = svfgBuilder.buildSVFG(_ander);
        stage0();
        output();
        validateTests();
    }

private:
    void stage0(); //pre-analysis
    void output(); //output results

    /*helper functions*/
    void getSrcSet(set<StmtItem>& srcs);
    void getSnkSet(const StmtItem& src, set<StmtItem>& snkSet);
    void forwardSearch(const Instruction* src, StmtItemSet& snkSet, InstSet& visited_forward, InstSet& visited_backward);
    void addNextInstToWorklist(const Instruction* curInst, InstWorkList& worklist, InstSet& visited_forward, InstSet& visited_backward);
    void backwardSearch(const Instruction* src, InstWorkList& worklist, InstSet& visited_backward);

    inline bool isAliasByAndersen(const StmtItem& item1, const StmtItem& item2) {
        //outs() << "ander query  NodeID: " << item1.getCurNodeID()<< " @ " << analysisUtil::getSourceLoc(item1.getLoc()) << "\n"
        //       << "             NodeID: " << item2.getCurNodeID()<< " @ " << analysisUtil::getSourceLoc(item2.getLoc()) << "\n"
        //       << (_ander->alias(item1.getCurNodeID(), item2.getCurNodeID()) ? "        MAY alias\n\n" : "        NOT alias\n\n" );
        ++stat.n_query_ander;
        return _ander->alias(item1.getCurNodeID(), item2.getCurNodeID());
    }

    void collectVFGIndEdgeAtCallSite();
    bool hasValueFlowInToCalleeViaCS(CallSiteID csid);

    inline PAG* getPAG() {
    	return _ander->getPAG();
    }
    inline Module* getModule() {
    	return _ander->getModule();
    }
    inline NodeID getPAGNodeID(const Value* v) {
    	return getPAG()->getValueNode(v);
    }
    inline PAGNode* getPAGNode(NodeID id){
    	return getPAG()->getPAGNode(id);
    }
    inline const Value* getValue(NodeID id) {
    	return getPAG()->getPAGNode(id)->getValue();
    }

    inline int getExtCallPos(const Function* fun) {
        if (!fun || EXT_API.find(fun->getName()) == EXT_API.end())
            return -1;
        return EXT_API[fun->getName()];
    }
    inline PointsTo& getAnderPts(NodeID id) {
    	return _ander->getPts(id);
    }
    inline PTACallGraph* getAnderCG() {
    	return _ander->getPTACallGraph();
    }

protected:
    /// Check true/false alarms to verify correctness of use-after-free
    //@{
    bool isUAF(u32_t free_line_num, u32_t use_line_num);
    void validateTests();
    void validateExpectedTruePositive(const char* fun);
    void validateExpectedTrueNegative(const char* fun);
    void validateExpectedFalsePositive(const char* fun);
    void validateExpectedFalseNegative(const char* fun);
    //@}

private:
    AndersenWaveDiff* _ander;
    SVFG* _svfg;

    StringMap<unsigned> EXT_API;
    PointsTo* srcPts;

    //result of stage 0
    FS_Result s0_out;

    //statistics
    STCStat stat;
};

#endif /* INCLUDE_DDA_STC_H_ */

