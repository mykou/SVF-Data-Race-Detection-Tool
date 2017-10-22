/*
 * HeapRefinement.h
 *
 *  Created on: 04/02/2016
 *      Author: dye
 */

#ifndef HEAPREFINEMENT_H_
#define HEAPREFINEMENT_H_


#include "MemoryModel/PAG.h"
#include "InterproceduralAnalysis.h"
#include <llvm/ADT/SmallSet.h>

class RCMemoryPartitioning;
class ThreadEscapeAnalysis;
class MhpAnalysis;
class LocksetAnalysis;


/*!
 * \brief Heap refinement analysis.
 * This analysis is a demand-driven analysis to refine
 * the memory access pairs that must access different
 * instances of a heap object.
 */
class HeapRefinement : private FunctionPassPool {
public:
    typedef llvm::SmallSet<const llvm::Instruction*, 8> InstSet;
    typedef llvm::SmallSet<const llvm::BasicBlock*, 8> BbSet;

    /*!
     * The data structure to record the destinations where a heap object's
     * address exclusively flows to.
     * The exclusiveness guarantees the destinations use the desired instance
     * of this heap object.
     * We only handle the case where the heap object exclusively flows to
     * the argument of only one spawn site.
     */
    struct HeapFlowDestinations {
        /// Constructor
        HeapFlowDestinations() : uniqueSpawnSite(NULL) {
        }

        /// The exclusively reachable spawn site
        const llvm::Instruction *uniqueSpawnSite;

        /// Memory accesses that dereference the pointer which
        /// exclusively flows from the heap allocation address
        InstSet memoryAccessInsts;

        /// Non-spawning call sites
        InstSet csInsts;

        /// Dump the destinations
        void print() const;
    };

    typedef std::map<const llvm::Instruction*, HeapFlowDestinations> HeapFlowMap;

    /// Constructor
    HeapRefinement() :
            mhp(0), lsa(0), tea(0), pag(0) {
    }

    /// Initialization
    void init(MhpAnalysis *mhp, LocksetAnalysis *lsa, ThreadEscapeAnalysis *tea);

    /*!
     * Check if two MHP Instructions access different instances of a given heap object.
     * @param objId the id of a given memory object
     * @param I1 memory access Instruction
     * @param I2 memory access Instruction
     * @return true if (1) objId is a heap object, and (2) I1 and I2 must access
     *         different instances of this heap object.
     */
    bool accessDifferentHeapInstances(NodeID objId, const llvm::Instruction *I1,
            const llvm::Instruction *I2);

private:
    /// Get the HeapFlowDestinations of a given heap object in scope.
    const HeapFlowDestinations &getFlowDestinationsInScope(
            const llvm::Instruction *mallocSite);

    /*!
     * Compute the HeapFlowDestinations of a given heap object.
     * The destinations are:
     *   (1) in the same loop of mallocSite if mallocSite is in a loop;
     *   (2) in the same function of mallocSite if mallocSite is not in a loop.
     * The exclusiveness guarantees the destinations use the desired instance
     * of this heap object.
     */
    void computeReachableTargetInScope(const llvm::Instruction *mallocSite);

    /// Get the heap allocation site of a give memory object if it is a heap object.
    inline const llvm::Instruction *getHeapAllocationSite(NodeID objId) {
        // The object must be heap.
        const MemObj* memObj = pag->getObject(objId);
        if (!memObj->isHeap())  return NULL;
        // Get the heap allocation call site.
        const llvm::Value *V = pag->getObject(objId)->getRefVal();
        return llvm::dyn_cast<llvm::Instruction>(V);
    }

    /**
     * Get the call sites, whose callee has side effect of a specific
     * memory access Instruction, from a given HeapFlowDestinations.
     * @param flowDsts the input HeapFlowDestinations
     * @param I the input memory access Instruction
     * @param csInsts the output call site Instructions
     */
    void getSideEffectCallSites(const HeapFlowDestinations &flowDsts,
            const llvm::Instruction *I, InstSet &csInsts);

    /*!
     * Get reachable Instructions and BasicBlocks from a given Instruction
     * in the same loop.
     * @param root the input root Instruction to start
     * @param L the input Loop where the analysis is scoped
     * @param reachableInsts the output reachable Instruction set
     * @param reachableBbs the output reachable BasicBlock set
     */
    static void getReachableCodeInSameIteration(const llvm::Instruction *root,
            llvm::Loop *L, InstSet &reachableInsts, BbSet &reachableBbs);

    /*!
     * Get reachable Instructions and BasicBlocks from a given Instruction.
     * @param root the input root Instruction to start
     * @param reachableInsts the output reachable Instruction set
     * @param reachableBbs the output reachable BasicBlock set
     */
    static void getReachableCode(const llvm::Instruction *root,
            InstSet &reachableInsts, BbSet &reachableBbs);

    /// Check if a destination Value is exclusively from a given source.
    static bool exclusivelyFrom(const llvm::Instruction *src,
            const llvm::Value *dst);

    MhpAnalysis *mhp;
    LocksetAnalysis *lsa;
    ThreadEscapeAnalysis *tea;
    PAG *pag;
    HeapFlowMap heapFlowMap;
};


#endif /* HEAPREFINEMENT_H_ */
