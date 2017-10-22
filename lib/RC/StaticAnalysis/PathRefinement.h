/*
 * PathRefinement.h
 *
 *  Created on: 04/02/2016
 *      Author: dye
 */

#ifndef PATHREFINEMENT_H_
#define PATHREFINEMENT_H_

#include "PathCorrelationAnalysis.h"
#include "MhpAnalysis.h"


/*!
 * \brief Path correlation refinement analysis.
 *
 * This analysis is a demand-driven analysis to refine
 * the results obtained from the course-grain MhpAnalysis.
 *
 * TODO: We currently only focus on non-structed global variables.
 * We may extend to support other data types.
 */
class PathRefinement : private FunctionPassPool {
public:
    typedef MhpAnalysis::FuncSet FuncSet;
    typedef MhpAnalysis::BbSet BbSet;
    typedef MhpAnalysis::InstSet InstSet;

    /*!
     * Representation of a condition guard that compares a variable
     * to a constant value.
     * The variable is considered as the lhs expression,
     * and the constant is considered as the rhs expression.
     * This can be derived from a llvm::CmpInst.
     */
    class CmpValConst {
    public:
        /// Constructor
        CmpValConst() :
                I(0), val(0), ptr(0), C(0),
                pred(llvm::CmpInst::BAD_ICMP_PREDICATE) {
        }

        /// Compute all valid conditions from a given Instruction
        /// along with its holding status.
        /// Return if the result is a valid CmpVarConst.
        bool recalculate(const llvm::Instruction *I, bool hold);

        /// Override some operators
        //@{
        inline bool operator== (const CmpValConst &cvc) const {
            return equals(cvc);
        }
        //@}

        /// Check if two expressions are equal.
        inline bool equals(const CmpValConst &cvc) const {
            return areEqual(*this, cvc);
        }

        /// Check if two expressions are complementary.
        inline bool excludes(const CmpValConst &cvc) const {
            return areExclusive(*this, cvc);
        }

        /// Check if two expressions are equal.
        static inline bool areEqual(const CmpValConst &cvc1,
                const CmpValConst &cvc2) {
            return cvc1.ptr == cvc2.ptr && cvc1.C == cvc2.C
                    && cvc1.pred == cvc2.pred;
        }

        /// Check if two expressions are complementary.
        static inline bool areExclusive(const CmpValConst &cvc1,
                const CmpValConst &cvc2) {
            return cvc1.ptr == cvc2.ptr && cvc1.C == cvc2.C
                    && cvc1.pred == rcUtil::getInversePredicate(cvc2.pred);
        }

        /// Get the source Instruction
        inline const llvm::Instruction *getInst() const {
            return I;
        }

        /// Get the variable's pointer
        inline const llvm::Value *getVariablePtr() const {
            return ptr;
        }

        /// Get the variable's value
        inline const llvm::Instruction *getVariableVal() const {
            return val;
        }

        /// Dump information of this CmpValConst
       void print() const;

    private:
        const llvm::Instruction *I;     ///< the source Instruction
        const llvm::Instruction *val;   ///< the value of the variable (lhs)
        const llvm::Value *ptr;         ///< the pointer to the lhs variable
        const llvm::Constant *C;        ///< the constant value (rhs)
        llvm::CmpInst::Predicate pred;  ///< predicate of *ptr (lhs) and C (rhs)

        /// The expression is valid.
        inline bool isValid() const {
            return ptr && C;
        }
    };


    /*!
     * The information of a variable that determines a condition.
     * It denotes the lhs expression of the CmpValConst class.
     */
    class CondVarInfo {
    public:
        typedef std::set<const llvm::Instruction*> InstSet;
        typedef InstSet::iterator iterator;
        typedef InstSet::const_iterator const_iterator;
        typedef std::map<const llvm::Instruction*, CodeSet> ReachableCodeMap;


        /// Iterator for directModsites
        //@{
        inline iterator directModsitesBegin() {
            return directModsites.begin();
        }
        inline iterator directModsitesEnd() {
            return directModsites.end();
        }
        inline const_iterator directModsitesBegin() const {
            return directModsites.begin();
        }
        inline const_iterator directModsitesEnd() const {
            return directModsites.end();
        }
        //@}

        /// Iterator for indirectModsites
        //@{
        inline iterator indirectModsitesBegin() {
            return indirectModsites.begin();
        }
        inline iterator indirectModsitesEnd() {
            return indirectModsites.end();
        }
        inline const_iterator indirectModsitesBegin() const {
            return indirectModsites.begin();
        }
        inline const_iterator indirectModsitesEnd() const {
            return indirectModsites.end();
        }
        //@}

        /// Reset this CondVarInfo object content.
        void reset(const llvm::Value *ptr);

        /// Get the reachable CodeSet map.
        inline const ReachableCodeMap &getReachableCodeMap() const {
            return reachableCodeMap;
        }

        /// Initialization of static members
        inline static void init(PathRefinement *refinement) {
            CondVarInfo::refinement = refinement;
        }

    private:
        /// Collect all sites that may directly or indirectly modify this->ptr.
        void computeModsites();

        /// Compute the reachable code of every modification site.
        void identifyReachableCode();

        const llvm::Value *ptr;
        InstSet directModsites;
        InstSet indirectModsites;
        ReachableCodeMap reachableCodeMap;

        /*!
         * Intra-procedural reachability analysis
         * @param I The input starting point.
         * @param reachable The output reachable CodeSet.
         */
        static void identifyReachableCodeWithinFunctionScope(
                const llvm::Instruction *I, CodeSet &reachable);

        static PathRefinement *refinement;
    };

    typedef CmpValConst Guard;
    typedef GuardExtractor<Guard>::Guards Guards;
    typedef std::map<const llvm::Value*, CondVarInfo> CondVarInfoMap;


    /// Initialization
    void init(MhpAnalysis *mhp, RCMemoryPartitioning *mp);

    /// Check if two Instructions are guarded by exclusive conditions.
    bool pathRefined(const llvm::Instruction *I1,
            const llvm::Instruction *I2);

    /// Member access
    //@{
    inline MhpAnalysis *getMhpAnalysis() {
        return mhp;
    }
    inline const RCMemoryPartitioning *getRCMemeoryPartitioning() const {
        return mp;
    }
    //@}

    /// Print the must-guarding conditions of a given Instruction or BasicBlock.
    //@{
    inline void printConditions(const llvm::Instruction *I) const {
        guardExtractor.printConditions(I);
    }
    inline void printConditions(const llvm::BasicBlock *bb) const {
        guardExtractor.printConditions(bb);
    }
    //@}


private:
    /// Check if two Guards are complementary.
    bool haveExclusiveGuard(const Guards &guards1, const Guards &guards2);

    /*!
     * Check if the value of a variable may not be consistent
     * at two program points.
     * @param varPtr The pointer of the memory object.
     * @param LI1 A program point.
     * @param LI2 Another program point.
     * @return True if the value of the variable may be inconsistent.
     */
    bool mayNotBeConsistent(const llvm::Value *varPtr,
            const llvm::Instruction *LI1,
            const llvm::Instruction *LI2);

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

    /// Get the information of a condition variable.
    //@{
    inline const CondVarInfo &getCondVarInfo(const llvm::Value *varPtr) {
        auto iter = condVarInfoMap.find(varPtr);
        if (iter != condVarInfoMap.end())   return iter->second;

        CondVarInfo &info = condVarInfoMap[varPtr];
        info.reset(varPtr);
        return info;
    }
    //@}

    GuardExtractor<Guard> guardExtractor;
    CondVarInfoMap condVarInfoMap;
    MhpAnalysis *mhp;
    RCMemoryPartitioning *mp;
};


#endif /* PATHREFINEMENT_H_ */
