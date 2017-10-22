/*
 * MhpPathFinder.h
 *
 *  Created on: 14/04/2016
 *      Author: dye
 */

#ifndef MHPPATHFINDER_H_
#define MHPPATHFINDER_H_


#include "MhpAnalysis.h"
#include "ContextSensitiveAliasAnalysis.h"


/*!
 * \brief The class to represent context pairs.
 *
 * This is used for queries of the CFL-reachability-based
 * context-sensitive pointer analysis.
 */
template<typename CtxImpl>
class CtxPairs {
public:
    typedef std::pair<const CtxImpl, const CtxImpl> CtxPair;
    typedef std::vector<CtxPair> CtxPairVec;
    typedef typename CtxPairVec::const_iterator const_iterator;
    typedef typename CtxPairVec::iterator iterator;

    /// Iterators
    //@{
    inline const_iterator begin() const {
        return ctxPairVec.begin();
    }
    inline const_iterator end() const {
        return ctxPairVec.end();
    }
    inline iterator begin() {
        return ctxPairVec.begin();
    }
    inline iterator end() {
        return ctxPairVec.end();
    }
    //@}

    /// Get the number of all context pairs.
    inline size_t size() const {
        return ctxPairVec.size();
    }

    /// Clear
    inline void clear() {
        ctxPairVec.clear();
    }

    /// Add an context pair.
    inline void addCtxPair(const CtxImpl &ctx1, const CtxImpl &ctx2) {
        ctxPairVec.push_back(std::make_pair(ctx1, ctx2));
    }

    /// Dump the CtxPairs information
    inline void print() const {
        llvm::outs() << "CtxPairs include " << size() << " pairs:\n";
        for (auto it = begin(), ie = end(); it != ie; ++it) {
            llvm::outs() << "\t--- ";
            it->first.print();
            llvm::outs() << "\t--- ";
            it->second.print();
            llvm::outs() << "\n";
        }
    }

private:
    CtxPairVec ctxPairVec;
};



/*!
 * \brief The analysis to find all feasible MHP context paths for
 * any Instruction pair.
 *
 * It aims to provide the context input for queries in the
 * CFL-reachability-based context-sensitive pointer analysis.
 * It utilizes the analysis results from MhpAnalysis.
 */
class MhpPathFinder {
public:
    typedef RC_CTX Ctx;

    /*!
     * The class to represent the mhp context paths for
     * two Instructions.
     */
    class MhpPaths {
    public:
        typedef CtxPairs<Ctx> MhpPathPairs;
        typedef MhpPathPairs::const_iterator const_iterator;
        typedef MhpPathPairs::iterator iterator;

        /// Constructors
        //@{
        MhpPaths() :
                I1(0), I2(0) {
        }
        MhpPaths(const llvm::Instruction *I1, const llvm::Instruction *I2) :
                I1(I1), I2(I2) {
        }
        //@}

        /// Iterators
        //@{
        inline const_iterator begin() const {
            return pathPairs.begin();
        }
        inline const_iterator end() const {
            return pathPairs.end();
        }
        inline iterator begin() {
            return pathPairs.begin();
        }
        inline iterator end() {
            return pathPairs.end();
        }
        //@}

        /// Get the number of all MHP path pairs.
        inline size_t size() const {
            return pathPairs.size();
        }

        /// Clear the MhpPaths
        inline void clear() {
            I1 = NULL;
            I2 = NULL;
            pathPairs.clear();
        }

        /// Add an Mhp path pair.
        inline void addCtxPair(const Ctx &ctx1, const Ctx &ctx2) {
            pathPairs.addCtxPair(ctx1, ctx2);
        }

        /// Reset the MhpPaths with two Instructions
        inline void reset(const llvm::Instruction *I1,
                const llvm::Instruction *I2) {
            this->I1 = I1;
            this->I2 = I2;
            pathPairs.clear();
        }

        /// Dump the information
        inline void print() const {
            llvm::outs() << "MHP path pairs for Instructions:\n";
            llvm::outs() << "\t" << rcUtil::getSourceLoc(I1) << "\n";
            llvm::outs() << "\t" << rcUtil::getSourceLoc(I2) << "\n";
            pathPairs.print();
        }

    private:
        const llvm::Instruction *I1;
        const llvm::Instruction *I2;
        MhpPathPairs pathPairs;
    };

    /// Constructor
    MhpPathFinder() : mhp(0), dbg(false), status(statusInit) {
    }

    /// Initialization
    void init(MhpAnalysis *mhp);

    /*!
     * Get the MhpPaths for two given Instructions.
     * @param I1 the input Instruction
     * @param rp1 the input ReachablePoint
     * @param I2 the other input Instruction
     * @param rp2 the other input ReachablePoint
     * @return the output MhpPaths of the two input Instructions
     */
    const MhpPaths *getMhpPaths(const llvm::Instruction *I1,
            const MhpAnalysis::ReachablePoint &rp1,
            const llvm::Instruction *I2,
            const MhpAnalysis::ReachablePoint &rp2);

    /// Dbg setters
    //@{
    inline void enableDbg() {
        dbg = true;
    }
    inline void disableDbg() {
        dbg = false;
    }
    //@}

    /// Status getters
    //@{
    inline int getStatus() const {
        return status;
    }
    inline bool isOutOfBudget() const {
        return statusOutOfBudget == status;
    }
    inline bool isSolved() const {
        return StatusSolved == status;
    }
    //@}

protected:

    /*!
     * The data structure describing a vector of Ctx along with its
     * thread splitting site.
     * It is used to compute the ctxPathPairs.
     */
    class CtxPaths {
    public:
        typedef std::pair<Ctx, const llvm::Instruction*> CtxPath;
        typedef std::vector<CtxPath>::iterator iterator;
        typedef std::vector<CtxPath>::const_iterator const_iterator;

        /// Constructor
        CtxPaths(const MhpAnalysis::ReachablePoint &rp) :
            rp(rp) {
        }

        /// Iterators
        //@{
        inline iterator begin() {
            return pathVec.begin();
        }
        inline iterator end() {
            return pathVec.end();
        }
        inline const_iterator begin() const {
            return pathVec.begin();
        }
        inline const_iterator end() const {
            return pathVec.end();
        }
        //@}

        /// New empty CtxPath
        inline iterator newCtxPath() {
            pathVec.resize(pathVec.size() + 1);
            return --pathVec.end();
        }

        /// Get the number of ctxPaths
        inline size_t size() const {
            return pathVec.size();
        }

        /// Get ReachablePoint
        const MhpAnalysis::ReachablePoint &getReachablePoint() const {
            return rp;
        }

    private:
        std::vector<CtxPath> pathVec;
        const MhpAnalysis::ReachablePoint rp;
    };

    /*!
     * Get all feasible context paths from a context-root to
     * a given Instruction through a given ReachablePoint.
     * @param I the input Instruction
     * @param ctxPaths the output CtxPaths
     * @return true if solved, or false otherwise
     */
    //@{
    /// The wrapper function.
    bool getCtxPaths(const llvm::Instruction *I, CtxPaths &ctxPaths) const;
    /// Proceed the fork-join case. The context-root is the "main" Function.
    bool getCtxPathsForForkJoin(const llvm::Instruction *I,
            CtxPaths &ctxPaths) const;
    /// Proceed the parallel-for case. The context-root is the relevant parallel-for call site.
    bool getCtxPathsForParFor(const llvm::Instruction *I,
            CtxPaths &ctxPaths) const;
    //@}


    /*!
     * Compute the MhpPaths for two given CtxPaths. The result is recorded
     * into this->mhpPaths.
     * @param ctxPaths1 the input CtxPath
     * @param ctxPaths2 the other input CtxPath
     * @return true if solved within budget, or false otherwise
     */
    bool computeMhpPaths(CtxPaths &ctxPaths1, CtxPaths &ctxPaths2);

    /*!
     * Push a Ctx into a CtxPaths.
     * Note that the following two rules should be applied when
     * the Ctx is pushed into the CtxPaths:
     *   (1) the element order of the input Ctx should be reversed;
     *   (2) the first element (i.e., the interested Instruction, which
     *       is not a part of a calling context) should be ignored.
     * @param ctx the input ctx
     * @param splittingSite the thread splitting site
     * @param ctxPaths the output CtxPaths
     */
    void pushCtxIntoCtxPaths(const Ctx &ctx,
            const llvm::Instruction *splittingSite, CtxPaths &ctxPaths) const;

    /*!
     * Check if a given Instruction is trunk reachable from a
     * thread spawn site (or its side effect site) intra-procedurally.
     * @param I the input Instruction
     * @param spawnSite the input thread spawn site
     * @return the splitting point if succeeds
     */
    const llvm::Instruction *isIntraprocedurallyTrunkReachable(
            const llvm::Instruction *I,
            const llvm::Instruction *spawnSite) const;

    /*!
     * Get the intra-procedural backward reachable code from
     * a given Instruction.
     * @param root the input Instruction
     * @param reachableCode the output CodeSet
     */
    void getBackwardReachableCode(const llvm::Instruction *root,
            CodeSet &reachableCode) const;

    /*!
     * Check if a Ctx path is feasible for a given thread-splitting site.
     * @param branchPath the Ctx path from a branch-reachable Instruction
     * @param splittingSite the corresponding thread-splitting site
     * @return the feasibility
     */
    bool isFeasibleBranchReachableCtxPath(const Ctx &branchPath,
            const llvm::Instruction *splittingSite) const;

private:

    /// Status setters
    //@{
    inline void setStatus(int s) {
        status = s;
    }
    inline void setInitStatus() {
        status = statusInit;
    }
    inline void setOutOfBudget() {
        status = statusOutOfBudget;
    }
    inline void setSolved() {
        status = StatusSolved;
    }
    inline void setOtherFailureStatus() {
        status = statusOtherFailure;
    }
    //@}

    MhpAnalysis *mhp;
    MhpPaths mhpPaths;
    bool dbg;
    int status;

    /// Status definitions
    //@{
    static constexpr int statusInit = 0;
    static constexpr int StatusSolved = 1;
    static constexpr int statusOutOfBudget = -1;
    static constexpr int statusOtherFailure = -2;
    //@}

    /// Budgets
    //@{
    static constexpr int maxCtxPathSize = 100;
    static constexpr int maxPathPairCount = 1000;
    //@}
};


#endif /* MHPPATHFINDER_H_ */
