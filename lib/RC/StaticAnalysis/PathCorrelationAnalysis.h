/*
 * PathCorrelationAnalysis.h
 *
 *  Created on: 29/02/2016
 *      Author: dye
 */

#ifndef PATHCORRELATIONANALYSIS_H_
#define PATHCORRELATIONANALYSIS_H_

#include "MemoryPartitioning.h"
#include "InterproceduralAnalysis.h"
#include "Util/ThreadCallGraph.h"


/*!
 * Guard extractor to identify the must-guarding conditions
 * of any given BasicBlock.
 * The analysis can be performed either intra-procedurally,
 * or inter-procedrually.
 */
template<typename Guard>
class GuardExtractor: public FunctionPassPool {
public:
    typedef llvm::SmallSet<const llvm::Function*, 16> FuncSet;
    typedef llvm::SmallSet<const llvm::BasicBlock*, 16> BbSet;
    typedef std::vector<Guard> Guards;
    typedef std::map<const llvm::BasicBlock*, Guards> Bb2GuardsMap;


    /// Constructor
    GuardExtractor() : tcg(NULL), performInterproceduralAnanlysis(false) {
    }

    /// Initilization
    void init(ThreadCallGraph *tcg, bool performInterproceduralAnanlysis) {
        this->tcg = tcg;
        this->performInterproceduralAnanlysis = performInterproceduralAnanlysis;
    }

    /// Lookup (or compute) the must-guarding conditions of a
    /// given Instruction.
    inline const Guards &getGuards(const llvm::Instruction *I) {
        return getGuards(I->getParent());
    }

    /// Lookup (or compute) the must-guarding conditions of a
    /// given BasicBlock.
    const Guards &getGuards(const llvm::BasicBlock *bb) {
        // Check if there is a existing one
        typename Bb2GuardsMap::const_iterator it = bb2GuardsMap.find(bb);
        if (it != bb2GuardsMap.end())     return it->second;

        // Compute a new set of conditions
        return computeGuards(bb);
    }

    /// Print the must-guarding conditions of a given Instruction
    /// or BasicBlock.
    //@{
    inline void printConditions(const llvm::Instruction *I) const {
        printConditions(I->getParent());
    }
    void printConditions(const llvm::BasicBlock *bb) const {
        typename Bb2GuardsMap::const_iterator it = bb2GuardsMap.find(bb);
        if (it == bb2GuardsMap.end())     return;

        llvm::outs() << " --- Must-guarding conditions for BasicBlock "
                << bb->getName() << " in Function "
                << bb->getParent()->getName() << " ("
                << rcUtil::getSourceLoc(bb) << "): ---\n";

        const Guards &guards = it->second;
        for (int i = 0, e = guards.size(); i != e; ++i) {
            guards[i].print();
        }
        llvm::outs() << "\n";
    }
    //@}


protected:
    /// Gather the must-guarding conditions of a BasicBlock.
    const Guards &computeGuards(const llvm::BasicBlock *bb) {
        Guards &guards = bb2GuardsMap[bb];

        // Compute internal guards
        computeIntraproceduralGuards(bb, guards);

        // Compute external guards
        if (performInterproceduralAnanlysis) {
            const llvm::Function *F = bb->getParent();
            if (!visitedFuncs.count(F)) {
                visitedFuncs.insert(F);
                computeInterproceduralGuards(F, guards);
            }
        }

        return guards;
    }


    /// Compute the intra-procedrual guards of a BasicBlock.
    void computeIntraproceduralGuards(const llvm::BasicBlock *dstBb,
            Guards &guards) const {
        const llvm::Function *F = dstBb->getParent();
        llvm::DominatorTree &dt = getDt(F);
        DomFrontier &df = getDf(F);
        const llvm::BasicBlock *rootBb = dt.getRoot();
        if (!dt.properlyDominates(rootBb ,dstBb))   return;

        // Identify BasicBlocks that are backward reachable from dstBb
        BbSet backwardReachable;
        identifyBackwardReachableBbs(dstBb, backwardReachable);

        // Prepare for reachability analysis
        BbSet visited;
        std::stack<const llvm::BasicBlock*> worklist;
        worklist.push(rootBb);
        Guard guard;

        // We try to find out the must-guarding conditions to "dstBb".
        while (!worklist.empty()) {
            const llvm::BasicBlock *currBb = worklist.top();
            worklist.pop();

            // Skip the BasicBlock if it is not backward reachable from dstBb
            if (!backwardReachable.count(currBb))   continue;

            // Skip the visited BasicBlock
            if (visited.count(currBb))    continue;
            visited.insert(currBb);

            // Add successors to worklist
            const llvm::TerminatorInst *TI = currBb->getTerminator();
            for (int i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
                const llvm::BasicBlock *succBb = TI->getSuccessor(i);
                if (backwardReachable.count(succBb) && !visited.count(succBb))
                    worklist.push(succBb);
            }

            // We only handle BranchInst at this moment.
            // TODO: handle other Instructions (e.g., switch).
            const llvm::BranchInst *BI = llvm::dyn_cast<llvm::BranchInst>(TI);
            if (!BI)    continue;

            // Handle unconditional BranchInst
            if (BI->isUnconditional()) {
                const llvm::BasicBlock *succBb = BI->getSuccessor(0);
                if (!visited.count(succBb))     worklist.push(succBb);
                continue;
            }

            // Handle the conditional BranchInst
            assert(2 == BI->getNumSuccessors());
            const llvm::BasicBlock *trueBranch = BI->getSuccessor(0);
            const llvm::BasicBlock *falseBranch = BI->getSuccessor(1);
            bool trueBranchIsDominator = dt.dominates(trueBranch, dstBb);
            bool falseBranchIsDominator = dt.dominates(falseBranch, dstBb);

            // We only consider the case iff one of the two branches dominates "dstBb".
            if (trueBranchIsDominator == falseBranchIsDominator)    continue;

            // The dominator branch should not be the joint node of a "if () { }" pattern,
            // This can be done by inspecting the dominance frontiers of the other branch.
            const llvm::BasicBlock *dominator =
                    trueBranchIsDominator ? trueBranch : falseBranch;
            const llvm::BasicBlock *other =
                    trueBranchIsDominator ? falseBranch : trueBranch;
            bool ifWithoutElsePattern = false;
            auto iter = df.find(const_cast<llvm::BasicBlock*>(other));
            if (iter != df.end()) {
                auto frontiers = iter->second;
                for (auto it = frontiers.begin(), ie = frontiers.end(); it != ie; ++it) {
                    const llvm::BasicBlock *frontier = *it;
                    if (frontier == dominator) {
                        ifWithoutElsePattern = true;
                        break;
                    }
                }
            }
            if (ifWithoutElsePattern)   continue;

            // Extract the condition from the conditional BranchInst
            const llvm::Value *cond = BI->getCondition();
            const llvm::CmpInst *CI = llvm::dyn_cast<llvm::CmpInst>(cond);
            if (guard.recalculate(CI, trueBranchIsDominator)) {
                guards.push_back(guard);
            }
        }

        return;
    }


    /// Compute the inter-procedrual guards of a Function.
    void computeInterproceduralGuards(const llvm::Function *F,
            Guards &guards) {
        PTACallGraphEdge::CallInstSet csInsts;
        rcUtil::getAllValidCallSitesInvokingCallee(tcg, F, csInsts);

        // Collect the BasicBlocks where F is called.
        BbSet bbs;
        for (auto it = csInsts.begin(), ie = csInsts.end(); it != ie; ++it) {
            const llvm::Instruction *I = *it;
            const llvm::BasicBlock *bb = I->getParent();
            bbs.insert(bb);
        }

        // We do nothing if we find no caller of "F".
        if (bbs.empty())    return;

        // Handle the unique BasicBlock case.
        if (1 == bbs.size()) {
            const llvm::BasicBlock *bb = *bbs.begin();
            const Guards &externalGuards = getGuards(bb);
            guards.insert(guards.end(), externalGuards.begin(), externalGuards.end());
        }
        // When there are more than one BasicBlocks, compute the intersection guards.
        else {
            auto iter = bbs.begin();
            const llvm::BasicBlock *bb = *(iter++);
            Guards intersectionGuards = getGuards(bb);
            std::vector<bool> hasNoMatchingGuard;
            hasNoMatchingGuard.resize(intersectionGuards.size());

            // Match every pair of the guards.
            while (iter != bbs.end()) {
                bb = *(iter++);
                const Guards &guards = getGuards(bb);
                for (int i = 0, ie = intersectionGuards.size(); i != ie; ++i) {
                    if (hasNoMatchingGuard[i])      continue;

                    hasNoMatchingGuard[i] = true;
                    Guard &guard = intersectionGuards[i];
                    for (int j = 0, je = guards.size(); j != je; ++j) {
                        if (guard == guards[j]) {
                            hasNoMatchingGuard[i] = false;
                            break;
                        }
                    }
                }
            }

            // Add the intersection guards into the output
            for (int i = 0, ie = intersectionGuards.size(); i != ie; ++i) {
                if (hasNoMatchingGuard[i])      continue;
                guards.push_back(intersectionGuards[i]);
            }
        }
    }


    /*!
     * Identify the backward reachable BasicBlocks of a given BasicBlock.
     * @param rootBb the input traversal root BasicBlock
     * @param bwdReachableBbs the output result recording the reachable BasicBlocks
     */
    void identifyBackwardReachableBbs(const llvm::BasicBlock *rootBb,
            BbSet &bwdReachableBbs) const {
        std::stack<const llvm::BasicBlock*> worklist;
        worklist.push(rootBb);
        while (!worklist.empty()) {
            const llvm::BasicBlock *BB = worklist.top();
            worklist.pop();
            if (bwdReachableBbs.count(BB))    continue;
            bwdReachableBbs.insert(BB);

            for (llvm::const_pred_iterator it = pred_begin(BB),
                    ie = pred_end(BB); it != ie; ++it) {
                const llvm::BasicBlock *predBb = *it;
                if (bwdReachableBbs.count(predBb))  continue;
                worklist.push(predBb);
            }
        }
    }


private:
    Bb2GuardsMap bb2GuardsMap;
    FuncSet visitedFuncs;
    ThreadCallGraph *tcg;
    bool performInterproceduralAnanlysis;
};



class LocksetAnalysis;  ///< used by PathCorrelationAnaysis


/*!
 * Intra-procedural path correlation analysis.
 */
class PathCorrelationAnaysis {
public:

    /*!
     * Representation of a condition guard that compares two values.
     */
    class SimpleCmpGuard {
    public:
        /// Constructor
        SimpleCmpGuard() :
                I(0), lhs(0), rhs(0),
                pred(llvm::CmpInst::BAD_ICMP_PREDICATE) {
        }

        /// Compute all valid conditions from a given Instruction
        /// along with its holding status.
        /// Return if the result is a valid SimpleCmpGuard.
        bool recalculate(const llvm::Instruction *I, bool hold);

        /// Override some operators
        //@{
        inline bool operator==(const SimpleCmpGuard &scg) const {
            return equals(scg);
        }
        //@}

        /// Compare if this equals a given SimpleCmpGuard
        inline bool equals(const SimpleCmpGuard &scg) const {
            return areEqual(*this, scg);
        }

        /// Dump information of this SimpleCmpGuard
        void print() const;

        /// Compare if two SimpleCmpGuard are equal
        static inline bool areEqual(const SimpleCmpGuard &scg1,
                const SimpleCmpGuard &scg2) {
            return scg1.lhs == scg2.lhs
                    && scg1.rhs == scg2.rhs && scg1.pred == scg2.pred;
        }

    private:
        const llvm::Instruction *I;     ///< the source Instruction
        const llvm::Value *lhs;         ///< the LHS value
        const llvm::Value *rhs;         ///< the RHS value
        llvm::CmpInst::Predicate pred;  ///< predicate of LHS and RHS
    };

    typedef SimpleCmpGuard Guard;
    typedef GuardExtractor<Guard>::Guards Guards;
    typedef GuardExtractor<Guard>::BbSet BbSet;

    /// Initialization
    void init(LocksetAnalysis *lsa);

    /// Release resources
    void release();

    /// Determine if the guards of I1 subsume the guards of I2.
    /// i.e., in first-order logic, Guards(I1) -> Guards(I2).
    inline bool subsumeOnGuards(const llvm::Instruction *I1,
            const llvm::Instruction *I2) {
        return subsumeOnGuards(I1->getParent(), I2->getParent());
    }

    /// Determine if the guards of bb1 subsume the guards of bb2.
    /// i.e., in first-order logic, Guards(bb1) -> Guards(bb2).
    bool subsumeOnGuards(const llvm::BasicBlock *bb1,
            const llvm::BasicBlock *bb2);

    /// Print condition guards
    //@{
    inline void printConditions(const llvm::BasicBlock *bb) const {
        guardExtractor.printConditions(bb);
    }
    inline void printConditions(const llvm::Instruction *I) const {
        guardExtractor.printConditions(I);
    }
    //@}


protected:
    /// Lookup (or compute) the must-guarding conditions of a
    /// given Instruction or BasicBlock.
    //@{
    inline const Guards &getGuards(const llvm::Instruction *I) {
        return guardExtractor.getGuards(I);
    }
    inline const Guards &getGuards(const llvm::BasicBlock *bb) {
        return guardExtractor.getGuards(bb);
    }
    //@}


private:
    LocksetAnalysis *lsa;
    GuardExtractor<Guard> guardExtractor;
};



#endif /* PATHCORRELATIONANALYSIS_H_ */
