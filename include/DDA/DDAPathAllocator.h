/*
 * DDAPathAllocator.h
 *
 *  Created on: Sep 9, 2014
 *      Author: Yulei Sui
 */

#ifndef DDAPATHALLOCATOR_H_
#define DDAPATHALLOCATOR_H_

#include "Util/PathCondAllocator.h"

/*!
 * Path condition allocator for DDA analysis
 */
class DDAPathAllocator : public PathCondAllocator {

public:
    /// Basic Block pair to represent CFG edges
    typedef std::pair<const llvm::BasicBlock*, const llvm::BasicBlock*> BBPair;
    /// Map a CFG to its conditions for caching purpose
    typedef std::map<const BBPair,Condition*> BBPairToConditionMap;

    /// CFG edge set class
    class CFGEdgeSet {
    public:
        /// less than operator for associative containers
        typedef struct {
            bool operator()(const BBPair& lhs, const BBPair &rhs) const {
                if (lhs.first < rhs.first)
                    return true;
                else if ( (lhs.first == rhs.first) && (lhs.second < rhs.second) )
                    return true;
                else
                    return false;
            }
        } equalBBPair;

        /// CFG edge set
        typedef std::map<BBPair,bool,equalBBPair> EdgeSetMap;
        EdgeSetMap edgeSet;
        /// Constructor
        CFGEdgeSet() {}
        /// Copy Constructor
        CFGEdgeSet(const CFGEdgeSet& e): edgeSet(e.getEdgeMap()) {}
        /// Return EdgeSet map
        inline const EdgeSetMap& getEdgeMap() const {
            return edgeSet;
        }
        /// Add new a CFG Edge
        inline void addCFGEdge(const llvm::BasicBlock* from,const llvm::BasicBlock* to) {
            edgeSet[std::make_pair(from,to)] = true;
        }
        /// Remove a CFG Edge
        inline void rmCFGEdge(const llvm::BasicBlock* from,const llvm::BasicBlock* to) {
            assert(hasCFGEdge(from,to) && "CFG edge not exit??");
            edgeSet[std::make_pair(from,to)] = false;
        }
        /// Whether bb has outgoing edge in edgeSet
        inline bool hasOutgoingCFGEdge(const llvm::BasicBlock* bb) const {
            for(EdgeSetMap::const_iterator it = edgeSet.begin(), eit = edgeSet.end(); it!=eit; ++it) {
                if(it->first.first == bb) {
                    if(it->second == true)
                        return true;
                }
            }
            return false;
        }
        /// Whether has the CFG Edge
        inline bool hasCFGEdge(const llvm::BasicBlock* from, const llvm::BasicBlock* to) const {
            EdgeSetMap::const_iterator it = edgeSet.find(std::make_pair(from,to));
            if(it!=edgeSet.end())
                return it->second;
            return false;
        }
        /// Clear set
        inline void clear() {
            edgeSet.clear();
        }
    };

    /// Constructor
    DDAPathAllocator(): _endBB(NULL) {
    }
    /// Destructor
    virtual ~DDAPathAllocator() {
    }

    typedef std::map<const Condition*, const llvm::Instruction* > CondToInstMap;	// map a condition to its branch instruction

    /// Guard Computation for a value-flow (between two basic blocks)
    //@{
    Condition* ComputeIntraVFGGuard(const llvm::BasicBlock* src, const llvm::BasicBlock* dst);
    Condition* ComputeInterCallVFGGuard(const llvm::BasicBlock* src, const llvm::BasicBlock* dst, const llvm::BasicBlock* callBB);
    Condition* ComputeInterRetVFGGuard(const llvm::BasicBlock* src, const llvm::BasicBlock* dst, const llvm::BasicBlock* retBB);
    //@}

    /// Get complement condition (from B1 to B0) according to a complementBB (BB2) at a phi
    /// e.g., B0: dstBB; B1:incomingBB; B2:complementBB
    Condition* getPHIComplementCond(const llvm::BasicBlock* BB1, const llvm::BasicBlock* BB2, const llvm::BasicBlock* BB0);

private:
    /// Allocate path condition for every basic block
    void allocateForBB(const llvm::BasicBlock& bb);

    /// Get/Set branch condition
    //@{
    void setBranchCond(const llvm::BasicBlock *bb, const llvm::BasicBlock *succ, Condition* cond);
    Condition* getBranchCond(const llvm::BasicBlock * bb, const llvm::BasicBlock *succ) const;
    //@}

    /// Allocate a new condition
    inline Condition* newCond(const llvm::Instruction* inst) {
        Condition* cond = bddCondMgr->createNewCond(totalCondNum++);
        assert(condToInstMap.find(cond)==condToInstMap.end() && "this should be a fresh condition");
        condToInstMap[cond] = inst;
        return cond;
    }

    /// Get conditional instruction
    inline const llvm::Instruction* getCondInst(const Condition* cond) const {
        CondToInstMap::const_iterator it = condToInstMap.find(cond);
        assert(it!=condToInstMap.end() && "this should be a fresh condition");
        return it->second;
    }

    /// Get/Set source basic block for computing intra value-flow
    //@{
    inline void setEndBB(const llvm::BasicBlock* bb) {
        _endBB = bb;
    }
    inline const llvm::BasicBlock* getEndBB() const {
        return _endBB;
    }
    //@}

    /// Find/Add/Set CFGEdges
    //@{
    inline void setCFGEdgeSet(const CFGEdgeSet& edges) {
        cfgEdges = edges;
    }
    //@}

    /// Recursively compute control flow guards
    Condition* getCFGuard(const llvm::BasicBlock* bb, CFGEdgeSet& edgeSet);

    CondToInstMap condToInstMap;			///< map a condition to its corresponding llvm instruction
    const llvm::BasicBlock* _endBB;		///< source node for during backward traverse of a value-flow
    CFGEdgeSet cfgEdges;					///< traversed cfg edges;
    BBPairToConditionMap _cachedCond;		/// map a basic block to
};


#endif /* DDAPATHALLOCATOR_H_ */
