/*
 * MTA.cpp
 *
 *  Created on: May 14, 2014
 *      Author: Yulei Sui, Peng Di
 */

#include "MTA/MTA.h"
#include "MTA/MHP.h"
#include "MTA/TCT.h"
#include "MTA/LockAnalysis.h"
#include "MTA/MTAStat.h"
#include "WPA/Andersen.h"
#include "MTA/FSMPTA.h"
#include "Util/AnalysisUtil.h"

#include <llvm/Support/CommandLine.h>	// for llvm command line options
#include <llvm/IR/InstIterator.h>	// for inst iteration

#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>

using namespace llvm;
using namespace analysisUtil;

static RegisterPass<MTA> RACEDETECOR("pmhp", "May-Happen-in-Parallel Analysis");

static cl::opt<bool> AndersenAnno("tsan-ander", cl::init(false), cl::desc("Add TSan annotation according to Andersen"));

static cl::opt<bool> FSAnno("tsan-fs", cl::init(false), cl::desc("Add TSan annotation according to flow-sensitive analysis"));


char MTA::ID = 0;
llvm::ModulePass* MTA::modulePass = NULL;
MTA::FunToSEMap MTA::func2ScevMap;
MTA::FunToLoopInfoMap MTA::func2LoopInfoMap;

MTA::MTA() :
    ModulePass(ID), tcg(NULL), tct(NULL) {
    stat = new MTAStat();
}

MTA::~MTA() {
    if (tcg)
        delete tcg;
    //if (tct)
    //    delete tct;
}

/*!
 * Perform data race detection
 */
bool MTA::runOnModule(llvm::Module& module) {


    modulePass = this;

    MHP* mhp = computeMHP(module);
    LockAnalysis* lsa = computeLocksets(mhp->getTCT());
    pairAnalysis(module, mhp, lsa);


    /*
    if (AndersenAnno) {
        pta = mhp->getTCT()->getPTA();
        if (pta->printStat())
            stat->performMHPPairStat(mhp,lsa);
        AndersenWaveDiff::releaseAndersenWaveDiff();
    } else if (FSAnno) {

        reportMemoryUsageKB("Mem before analysis");
        DBOUT(DGENERAL, outs() << pasMsg("FSMPTA analysis\n"));
        DBOUT(DMTA, outs() << pasMsg("FSMPTA analysis\n"));

        DOTIMESTAT(double ptStart = stat->getClk());
        pta = FSMPTA::createFSMPTA(module, mhp,lsa);
        DOTIMESTAT(double ptEnd = stat->getClk());
        DOTIMESTAT(stat->FSMPTATime += (ptEnd - ptStart) / TIMEINTERVAL);

        reportMemoryUsageKB("Mem after analysis");

        if (pta->printStat())
            stat->performMHPPairStat(mhp,lsa);

        FSMPTA::releaseFSMPTA();
    }

    if (DoInstrumentation) {
        DBOUT(DGENERAL, outs() << pasMsg("ThreadSanitizer Instrumentation\n"));
        DBOUT(DMTA, outs() << pasMsg("ThreadSanitizer Instrumentation\n"));
        TSan tsan;
        tsan.doInitialization(*pta->getModule());
        for (Module::iterator it = pta->getModule()->begin(), eit = pta->getModule()->end(); it != eit; ++it) {
            tsan.runOnFunction(*it);
        }
        if (pta->printStat())
            PrintStatistics();
    }
    */

    delete mhp;
    delete lsa;

    return false;
}

/*!
 * Compute lock sets
 */
LockAnalysis* MTA::computeLocksets(TCT* tct) {
    LockAnalysis* lsa = new LockAnalysis(tct);
    lsa->analyze();
    return lsa;
}

MHP* MTA::computeMHP(llvm::Module& module) {

    DBOUT(DGENERAL, outs() << pasMsg("MTA analysis\n"));
    DBOUT(DMTA, outs() << pasMsg("MTA analysis\n"));
    PointerAnalysis* pta = AndersenWaveDiff::createAndersenWaveDiff(module);
    pta->getPTACallGraph()->dump("ptacg");
    DBOUT(DGENERAL, outs() << pasMsg("Create Thread Call Graph\n"));
    DBOUT(DMTA, outs() << pasMsg("Create Thread Call Graph\n"));
    tcg = new ThreadCallGraph(&module);
    tcg->updateCallGraph(pta);
    //tcg->updateJoinEdge(pta);
    DBOUT(DGENERAL, outs() << pasMsg("Build TCT\n"));
    DBOUT(DMTA, outs() << pasMsg("Build TCT\n"));

    DOTIMESTAT(double tctStart = stat->getClk());
    tct = new TCT(tcg, pta);
    DOTIMESTAT(double tctEnd = stat->getClk());
    DOTIMESTAT(stat->TCTTime += (tctEnd - tctStart) / TIMEINTERVAL);

    if (pta->printStat()) {
        stat->performThreadCallGraphStat(tcg);
        stat->performTCTStat(tct);
    }

    tcg->dump("tcg");

    DBOUT(DGENERAL, outs() << pasMsg("MHP analysis\n"));
    DBOUT(DMTA, outs() << pasMsg("MHP analysis\n"));

    DOTIMESTAT(double mhpStart = stat->getClk());
    MHP* mhp = new MHP(tct);
    mhp->analyze();
    DOTIMESTAT(double mhpEnd = stat->getClk());
    DOTIMESTAT(stat->MHPTime += (mhpEnd - mhpStart) / TIMEINTERVAL);

    DBOUT(DGENERAL, outs() << pasMsg("MHP analysis finish\n"));
    DBOUT(DMTA, outs() << pasMsg("MHP analysis finish\n"));
    return mhp;
}

///*!
// * Check   (1) write-write race
// * 		 (2) write-read race
// * 		 (3) read-read race
// * when two memory access may-happen in parallel and does not protected by the same lock
// * (excluding global constraints because they are initialized before running the main function)
// */
void MTA::detect(llvm::Module& module) {

    DBOUT(DGENERAL, outs() << pasMsg("Starting Race Detection\n"));

    LoadSet loads;
    StoreSet stores;

    std::set<const Instruction*> needcheckinst;
    // Add symbols for all of the functions and the instructions in them.
    for (Module::iterator F = module.begin(), E = module.end(); F != E; ++F) {
        // collect and create symbols inside the function body
        for (inst_iterator II = inst_begin(*F), E = inst_end(*F); II != E; ++II) {
            const Instruction *inst = &*II;
            if (const LoadInst* load = dyn_cast<LoadInst>(inst)) {
                loads.insert(load);
            } else if (const StoreInst* store = dyn_cast<StoreInst>(inst)) {
                stores.insert(store);
            }
        }
    }

    for (LoadSet::const_iterator lit = loads.begin(), elit = loads.end(); lit != elit; ++lit) {
        const LoadInst* load = *lit;
        bool loadneedcheck = false;
        for (StoreSet::const_iterator sit = stores.begin(), esit = stores.end(); sit != esit; ++sit) {
            const StoreInst* store = *sit;

            loadneedcheck = true;
            needcheckinst.insert(store);
        }
        if (loadneedcheck)
            needcheckinst.insert(load);
    }

    outs() << "HP needcheck: " << needcheckinst.size() << "\n";
}

bool hasDataRace(InstructionPair &pair, llvm::Module& module, MHP *mhp, LockAnalysis *lsa){
    //check alias
    PointerAnalysis* pta = AndersenWaveDiff::createAndersenWaveDiff(module);
    bool alias = false;
    if (const StoreInst *p1 = dyn_cast<StoreInst>(pair.getInst1())){
        if (const StoreInst *p2 = dyn_cast<StoreInst>(pair.getInst2())){
            AliasResult results = pta->alias(p1->getPointerOperand(),p2->getPointerOperand());
            pair.setAlias(results);
            alias = (results == MayAlias || results == MustAlias || results == PartialAlias);
        } else if (const LoadInst *p2 = dyn_cast<LoadInst>(pair.getInst2())) {
            AliasResult results = pta->alias(p1->getPointerOperand(),p2->getPointerOperand());
            pair.setAlias(results);
            alias = (results == MayAlias || results == MustAlias || results == PartialAlias);
        }
    } else if (const LoadInst *p1 = dyn_cast<LoadInst>(pair.getInst1())){
        if (const LoadInst *p2 = dyn_cast<LoadInst>(pair.getInst2())){
            AliasResult results = pta->alias(p1->getPointerOperand(),p2->getPointerOperand());
            pair.setAlias(results);
            alias = (results == MayAlias || results == MustAlias || results == PartialAlias);
        } else if (const StoreInst *p2 = dyn_cast<StoreInst>(pair.getInst2())){
            AliasResult results = pta->alias(p1->getPointerOperand(),p2->getPointerOperand());
            pair.setAlias(results);
            alias = (results == MayAlias || results == MustAlias || results == PartialAlias);
        }
    }
    if (!alias) return false;
    //check mhp
    if (!mhp->mayHappenInParallel(pair.getInst1(), pair.getInst2())) return false;
    //check lock
    return (!lsa->isProtectedByCommonLock(pair.getInst1(),pair.getInst2()));
}

bool isShared(const llvm::Value *val, llvm::Module& module){
    PointerAnalysis* pta = AndersenWaveDiff::createAndersenWaveDiff(module);
    PAG* pag = pta->getPAG();
    const PointsTo& target = pta->getPts(pag->getValueNode(val));
    for (PointsTo::iterator it = target.begin(), eit = target.end();
            it != eit; ++it) {
        const MemObj *obj = pag->getObject(*it);
        if (obj->isGlobalObj() || obj->isStaticObj() || obj->isHeap()){
            return true;
        }
    }
    return false;
}

bool isShared(const Instruction *loc, llvm::Module& module){
    if (const StoreInst *p1 = dyn_cast<StoreInst>(loc)){
        return isShared(p1->getPointerOperand(), module);
    } else if (const LoadInst *p1 = dyn_cast<LoadInst>(loc)){
        return isShared(p1->getPointerOperand(), module);
    }
    return false;
}

bool isSite(const Instruction *inst){
    llvm::CallSite cs(const_cast<llvm::Instruction*>(inst));
    return (analysisUtil::isStaticExtCall(cs) || analysisUtil::isHeapAllocExtCallViaRet(cs));
}

void MTA::pairAnalysis(llvm::Module& module, MHP *mhp, LockAnalysis *lsa){
    std::cout << " --- Running pair analysis ---\n";

    std::set<const Instruction *> instructions;
    std::vector<InstructionPair> pairs;

    for (Module::iterator F = module.begin(), E = module.end(); F != E; ++F) {
        for (inst_iterator II = inst_begin(&*F), E = inst_end(&*F); II != E; ++II) {
            const Instruction *inst = &*II;
            if (const StoreInst *st = dyn_cast<StoreInst>(inst)) {
                instructions.insert(st);
            }

            else if (const LoadInst *ld = dyn_cast<LoadInst>(inst)) {
                instructions.insert(ld);
            }
        }
    }
    int total = instructions.size();
    int count = 0;
    // pair up all instructions (NC2) pairs and add to pair vector if they contain data race
    for (auto it = instructions.cbegin(); it != instructions.cend();){   
        // if this instruction is not global/heap/static then skip
        ++count;
        if (count%50==0){
            std::cout << "Analysing... ";
            std::cout << int((double(count)/total)*100) << " %\r";    
            std::cout.flush();
        }
        
        if (!isShared(*it,module)) {
            ++it;
            continue;
        }
        for (auto it2 = ++it; it2 != instructions.cend(); ++it2){        
            InstructionPair pair = InstructionPair(*it,*it2);  
            if (hasDataRace(pair, module,mhp,lsa)){
                pairs.push_back(pair);
            }
        }
    }
    
    instructions.clear();

    // remove empty instructions
    // write to txt file
    std::ofstream output;
    output.open("output.txt");
    /*
    separate error message into
    line1
    filename1
    line2
    filename2
    for the plugin
    */
    std::string s1;
    std::string s2;
    std::regex line("ln: (\\d+)");
    std::regex file("fl: (.*)");
    std::smatch match;
    for (auto it = pairs.cbegin(); it != pairs.cend(); ++it){
        s1 = getSourceLoc(it->getInst1());
        s2 = getSourceLoc(it->getInst2());
        if (s1.empty() || s2.empty()) continue;
        if (std::regex_search(s1, match, line))
            output << match[1] << std::endl;
        if (std::regex_search(s1, match, file))
            output << match[1] << std::endl;
        if (std::regex_search(s2, match, line))
            output << match[1] << std::endl;
        if (std::regex_search(s2, match, file))
            output << match[1] << std::endl;
    }
    output.close();
    //need to also remove paairs that are a local variable. need to go into mem, and check.

}

