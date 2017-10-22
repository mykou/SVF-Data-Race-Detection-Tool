/*
 * @file: DDAClient.h
 * @author: yesen
 * @date: 4 Feb 2015
 *
 * LICENSE
 *
 */


#ifndef DDACLIENT_H_
#define DDACLIENT_H_


#include "MemoryModel/PAG.h"
#include "MemoryModel/PAGBuilder.h"
#include "MemoryModel/PointerAnalysis.h"
#include "MSSA/SVFGNode.h"
#include "Util/BasicTypes.h"
#include "Util/CPPUtil.h"
#include <llvm/IR/DataLayout.h>


/**
 * General DDAClient which queries all top level pointers by default.
 */
class DDAClient {
public:
    DDAClient(llvm::Module& mod) : pag(NULL), module(mod), curPtr(0), solveAll(true) {}

    virtual ~DDAClient() {}

    virtual inline void initialise(llvm::Module& module) {}

    /// Collect candidate pointers for query.
    virtual inline NodeBS& collectCandidateQueries(PAG* p) {
        setPAG(p);
        if (solveAll)
            candidateQueries = pag->getAllValidPtrs();
        else {
            for (NodeBS::iterator it = userInput.begin(), eit = userInput.end(); it != eit; ++it)
                addCandidate(*it);
        }
        return candidateQueries;
    }
    /// Get candidate queries
    inline const NodeBS& getCandidateQueries() const {
        return candidateQueries;
    }

    /// Call back used by DDAVFSolver.
    virtual inline void handleStatement(const SVFGNode* stmt, NodeID var) {}
    /// Set PAG graph.
    inline void setPAG(PAG* g) {
        pag = g;
    }
    /// Set the pointer being queried.
    void setCurrentQueryPtr(NodeID ptr) {
        curPtr = ptr;
    }
    /// Set pointer to be queried by DDA analysis.
    void setQuery(NodeID ptr) {
        userInput.set(ptr);
        solveAll = false;
    }
    /// Get LLVM module
    inline llvm::Module& getModule() const {
        return module;
    }
    virtual void answerQueries(PointerAnalysis* pta);

    virtual inline void performStat(PointerAnalysis* pta) {}

    virtual inline void collectWPANum(llvm::Module& mod) {}
protected:
    void addCandidate(NodeID id) {
        if (pag->isValidTopLevelPtr(pag->getPAGNode(id)))
            candidateQueries.set(id);
    }

    PAG*   pag;					///< PAG graph used by current DDA analysis
    llvm::Module& module;		///< LLVM module
    NodeID curPtr;				///< current pointer being queried
    NodeBS candidateQueries;	///< store all candidate pointers to be queried

private:
    NodeBS userInput;           ///< User input queries
    bool solveAll;				///< TRUE if all top level pointers are being queried
};


/**
 * DDA client with function pointers as query candidates.
 */
class FunptrDDAClient : public DDAClient {
public:
    FunptrDDAClient(llvm::Module& module) : DDAClient(module) {}
    ~FunptrDDAClient() {}

    /// Only collect function pointers as query candidates.
    virtual inline NodeBS& collectCandidateQueries(PAG* p) {
        setPAG(p);
        for(PAG::CallSiteToFunPtrMap::const_iterator it = pag->getIndirectCallsites().begin(),
                eit = pag->getIndirectCallsites().end(); it!=eit; ++it) {
            if (cppUtil::isVirtualCallSite(it->first)) {
                const llvm::Value *vtbl = cppUtil::getVCallVtblPtr(it->first);
                assert(pag->hasValueNode(vtbl));
                NodeID vtblId = pag->getValueNode(vtbl);
                addCandidate(vtblId);
            } else
                addCandidate(it->second);
        }
        return candidateQueries;
    }
};


/**
 * DDA client with pointers at bitcast instructions as query candidates.
 */
class CastDDAClient : public DDAClient {
public:
    CastDDAClient(llvm::Module& module) : DDAClient(module) {}
    ~CastDDAClient() {}

    virtual inline NodeBS& collectCandidateQueries(PAG* p) {
        setPAG(p);
        const llvm::Module* module = pag->getModule();
        for (llvm::Module::const_iterator funIter = module->begin(), funEiter = module->end();
                funIter != funEiter; ++funIter) {
            const llvm::Function& func = *funIter;
            for (llvm::Function::const_iterator bbIt = func.begin(), bbEit = func.end();
                    bbIt != bbEit; ++bbIt) {
                const llvm::BasicBlock& bb = *bbIt;
                for (llvm::BasicBlock::const_iterator instIt = bb.begin(), instEit = bb.end();
                        instIt != instEit; ++instIt) {
                    const llvm::Instruction& inst = *instIt;
                    if (const llvm::BitCastInst* bitcast = llvm::dyn_cast<llvm::BitCastInst>(&inst)) {
                        const llvm::Value* value = bitcast->getOperand(0);
                        if (pag->hasValueNode(value)) {
                            addCandidate(pag->getValueNode(value));
                            instToPtrMap.insert(std::make_pair(bitcast, pag->getValueNode(value)));
                        }
                    }
                }
            }
        }
        return candidateQueries;
    }

    virtual void performStat(PointerAnalysis* pta);

private:
    /// Check each element's size in nodes according to the specified type.
    u32_t checkTypeSize(PointsTo& nodes, llvm::Type* type, llvm::DataLayout* dl);
    /// Find number of function target.
    u32_t statFunctionPointeeNumber(PointsTo& pts);
    /// Check each element's size according to the pointeeSize.
    u32_t statPtsTypeIncompatible(PointsTo& pts, uint64_t pointeeSize, llvm::DataLayout* dl);

    typedef std::map<const llvm::BitCastInst*, NodeID> CastInstPtrMap;
    CastInstPtrMap instToPtrMap;
};


/**
 * DDA client finding use after free statements.
 */
class FreeDDAClient : public DDAClient {
private:
    /**
     * Add support for handling free() call in program.
     */
    class DDAFreePAGBuilder : public PAGBuilder {
    public:
        DDAFreePAGBuilder() : PAGBuilder() {}
        ~DDAFreePAGBuilder() {}

        inline bool isFreeStore(const PAGEdge* edge) const {
            return (llvm::isa<StorePE>(edge)) ? (nullStoreEdges.find(edge) != nullStoreEdges.end()) : false;
        }

    private:
        /// Overrided methods to handle free().
        /// Set the memory location pointed by argument in free() to NULL after the call site.
        void handleExtCall(llvm::CallSite cs, const llvm::Function* callee) {
            PAGBuilder::handleExtCall(cs, callee);
            if(analysisUtil::isExtCall(callee)) {
                ExtAPI::extf_t tF= analysisUtil::extCallTy(callee);
                if (tF == ExtAPI::EFT_FREE) {
                    /// 1. Find the memory location deallocated by free().
                    llvm::Value* arg = cs.getArgument(0);
                    assert(llvm::isa<llvm::PointerType>(arg->getType()) && "expecting pointer type for argument in free()");
                    const llvm::Value* ptrValue = findFreeValue(arg);

                    if (ptrValue == NULL)
                        return;

                    /// 2. Store null to the memory location.
                    NodeID src = getPAG()->getNullPtr();
                    NodeID dst = getValueNode(ptrValue);
                    getPAG()->addStoreEdge(src, dst);
                    PAGEdge* edge = getPAG()->getIntraPAGEdge(src, dst, PAGEdge::Store);
                    nullStoreEdges.insert(llvm::dyn_cast<StorePE>(edge));
                }
            }
        }

        /// Find source value of argument in free()
        const llvm::Value* findFreeValue(const llvm::Value* val) const {

            if (const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(val))
                return load->getOperand(0);
            else if (const llvm::BitCastInst* bitcast = llvm::dyn_cast<llvm::BitCastInst>(val))
                return findFreeValue(bitcast->getOperand(0));
            else
                return NULL;
        }

        typedef std::set<const PAGEdge*> PAGEdgeSet;
        PAGEdgeSet nullStoreEdges;	///< null store edges added after free().
    };

public:
    FreeDDAClient(llvm::Module& module) : DDAClient(module) {}
    ~FreeDDAClient() {}

    void initialise(llvm::Module& module) {
        /// Generate an PAG with store null statement after each free() call.
        SymbolTableInfo* symTable = SymbolTableInfo::Symbolnfo();
        symTable->buildMemModel(module);

        DBOUT(DGENERAL, llvm::outs() << analysisUtil::pasMsg("Building PAG ...\n"));
        PointerAnalysis::setPAG(pagBuilder.build(module));
    }

    /// Collect all load pointers as query candidates.
    inline NodeBS& collectCandidateQueries(PAG* p) {
        setPAG(p);
        const PAGEdge::PAGEdgeSetTy& loadEdges = pag->getEdgeSet(PAGEdge::Load);
        PAGEdge::PAGEdgeSetTy::const_iterator it = loadEdges.begin();
        PAGEdge::PAGEdgeSetTy::const_iterator eit = loadEdges.end();
        for (; it != eit; ++it) {
            PAGEdge* edge = *it;
            addCandidate(edge->getSrcID());
        }
        return candidateQueries;
    }

    void handleStatement(const SVFGNode* stmt, NodeID var) {
        if (const StoreSVFGNode* store = llvm::dyn_cast<StoreSVFGNode>(stmt)) {
            if (store->getPAGSrcNodeID() == pag->getNullPtr() && pagBuilder.isFreeStore(store->getPAGEdge())) {
                reachableFreeStore.insert(store);
                detectedUseAfterFree.insert(std::make_pair(curPtr, store->getPAGEdge()));
            }
        }
    }

    void dumpUseAfterFree();

    inline Size_t getLoadAfterFreePtrNum() const {
        NodeBS ptrs;
        LoadAfterFreePairSet::const_iterator it = detectedUseAfterFree.begin();
        LoadAfterFreePairSet::const_iterator eit = detectedUseAfterFree.end();
        for (; it != eit; ++it) {
            ptrs.set(it->first);
        }
        return ptrs.count();
    }

    inline Size_t getReachableFreeStoreNum() const {
        return reachableFreeStore.size();
    }

    void performStat(PointerAnalysis* pta);

private:
    typedef std::set<const StoreSVFGNode*> StoreSVFGNodeSet;
    typedef std::pair<NodeID, const PAGEdge*> PtrStorePAGPair;
    typedef std::set<PtrStorePAGPair> LoadAfterFreePairSet;

    StoreSVFGNodeSet reachableFreeStore;     ///< all free store stmt
    LoadAfterFreePairSet detectedUseAfterFree; ///< use after free detected
    DDAFreePAGBuilder pagBuilder;	///< PAG builder
};

class AndersenWaveDiff;

/**
 * DDA client finding uninitialised variables.
 */
class UninitDDAClient : public DDAClient {
private:
    /**
     * Add null store constraints after each alloca instructions.
     */
    class DDAUninitPAGBuilder : public PAGBuilder {
    public:
        DDAUninitPAGBuilder() : PAGBuilder() {}
        ~DDAUninitPAGBuilder() {}

        inline const llvm::Value* getAllocaInstForDummyObj(NodeID id) const {
            ObjNodeToValueMap::const_iterator it = objNodeToValueMap.find(id);
            assert(it != objNodeToValueMap.end() && "can not find alloca inst for this dummy node");
            return it->second;
        }

        inline bool isDummyStore(const PAGEdge* edge) const {
            return dummyStore.find(edge) != dummyStore.end();
        }

    private:
        /// Overrided methods to add null store constraints after alloca/malloc instructions.
        ///@{
        void handleExtCall(llvm::CallSite cs, const llvm::Function *callee);

        void visitAllocaInst(llvm::AllocaInst &AI);
        //@}

        /// Store dummy value node to the memory location.
        void addUninitStorePAGEdge(const llvm::Value* value);

        typedef std::map<NodeID, const llvm::Value*> ObjNodeToValueMap;
        typedef std::set<const PAGEdge*> PAGEdgeSet;

        PAGEdgeSet dummyStore;
        ObjNodeToValueMap objNodeToValueMap;
    };

public:
    UninitDDAClient(llvm::Module& module) : DDAClient(module) {}
    ~UninitDDAClient() {}

    void collectWPANum(llvm::Module& mod);

    void initialise(llvm::Module& module) {
        /// Generate an PAG with store null statement after each free() call.
        SymbolTableInfo* symTable = SymbolTableInfo::Symbolnfo();
        symTable->buildMemModel(module);

        DBOUT(DGENERAL, llvm::outs() << analysisUtil::pasMsg("Building PAG ...\n"));
        PointerAnalysis::setPAG(pagBuilder.build(module));
    }

    /// Collect all destination ptr of load constraints as query candidates.
    NodeBS& collectCandidateQueries(PAG* p);

    void performStat(PointerAnalysis* pta);

    void printQueryPTS(PointerAnalysis* pta);

private:
    void dumpUninitPtrs(const NodeBS& vars, PointerAnalysis* pta);
    void dumpUninitObjs(const NodeBS& vars, PointerAnalysis* pta);

    void dumpSUStore(PointerAnalysis* pta);
    void dumpSUPointsTo(const StoreSVFGNode* store, PointerAnalysis* ddaPta);

    void getUninitVarNumber(PointerAnalysis* pta, NodeBS& vars, NodeBS& varsFil, NodeBS& objs, NodeBS& objsFil) const;

    DDAUninitPAGBuilder pagBuilder;			///< PAG builder
};


#endif /* DDACLIENT_H_ */
