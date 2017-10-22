/*
 * ThreadEscapeAnalysis.h
 *
 *  Created on: 30/10/2015
 *      Author: dye
 */

#ifndef THREADESCAPEANALYSIS_H_
#define THREADESCAPEANALYSIS_H_

#include "MemoryModel/PointerAnalysis.h"


class ThreadCallGraph;      // Used by ThreadEscapeAnalysis
class PointerAnalysis;      // Used by ThreadEscapeAnalysis
class MemoryPartitioning;   // Used by ThreadEscapeAnalysis


/*!
 * Lightweight thread escape analysis.
 */
class ThreadEscapeAnalysis {
public:
    /// Initialization
    void init(ThreadCallGraph *tcg, PointerAnalysis *pta,
            MemoryPartitioning *mp);

    /// Perform thread escape analysis
    void analyze();

    /// Release resources
    void release();

    /// Check if a partition may escape from a given or any thread
    //@{
    inline bool mayEscape(unsigned id) {
        return (globalVisible.test(id) || allSpawnSiteVisible.test(id));
    }
    inline bool mayEscape(unsigned id, const llvm::Instruction *spawnSite) {
        return (globalVisible.test(id) || spawnSiteVisibleMap[spawnSite].test(id));
    }
    template<typename InstSetType>
    inline bool mayEscape(unsigned id, InstSetType &spawnSites) {
        for (auto it = spawnSites.begin(), ie = spawnSites.end(); it != ie; ++it) {
            if (mayEscape(id, *it))     return true;
        }
        return false;
    }
    //@}

    /// Get the spawn sites for which a given partition is visible
    template<typename InstSetType>
    inline void getVisibleSpawnSites(unsigned id, InstSetType &spawnSites) {
        if (globalVisible.test(id)) {
            for (auto it = spawnSiteVisibleMap.begin(),
                    ie = spawnSiteVisibleMap.end(); it != ie; ++it) {
                spawnSites.insert(it->first);
            }
        } else {
            for (auto it = spawnSiteVisibleMap.begin(),
                    ie = spawnSiteVisibleMap.end(); it != ie; ++it) {
                if (it->second.test(id))    spawnSites.insert(it->first);
            }
        }
    }


    /// Partition/object visibility
    //@{
    inline PointsTo &getGlobalVisible() {
        return globalVisible;
    }
    inline PointsTo &getAllSpawnSiteVisible() {
        return allSpawnSiteVisible;
    }
    inline PointsTo &getSpawnSiteVisible(const llvm::Instruction *spawnSite) {
        return spawnSiteVisibleMap[spawnSite];
    }
    //@}

    /// Check if the object pointed to by a given pointer is visible.
    //@{
    inline bool isGlobalVisible(const llvm::Value *p) {
        return isVisible(p, globalVisible);
    }
    inline bool isAllSpawnSiteVisible(const llvm::Value *p) {
        return isVisible(p, allSpawnSiteVisible);
    }
    inline bool isSpawnSiteVisible(const llvm::Value *p,
            const llvm::Instruction *spawnSite) {
        return isVisible(p, spawnSiteVisibleMap[spawnSite]);
    }
    //@}

private:
    PointerAnalysis *pta;
    ThreadCallGraph *tcg;
    MemoryPartitioning *mp;
    /// visible partitions/objects from global pointers
    PointsTo globalVisible;
    /// visible partitions/objects from the arguments of all spawn sites
    PointsTo allSpawnSiteVisible;
    /// visible partitions/objects from the arguments of each spawn site
    std::map<const llvm::Instruction*, PointsTo> spawnSiteVisibleMap;

    /*!
     * Check if the object pointed to by a given pointer is visible in
     * a specific set of memory partitions.
     * @param p the input pointer
     * @param visibleParts the input visible partitions
     * @return the output boolean value
     */
    bool isVisible(const llvm::Value *p, PointsTo &visibleParts) const;

    /*!
     * Compute the iterative points-to set of a given pointer.
     * The result contains all the memory objects that are visible from
     * the given pointer (i.e., those can be directly or indirectly
     * accessed via the pointer).
     * @param val the input pointer.
     * @param ret the output conjuncted points-to set.
     */
    void getIterativePtsForAllFields(const llvm::Value *val,
            PointsTo &ret) const;

    /// Collect visible objects from global values
    void collectGlobalVisibleObjs();

    /// Collect visible objects from the arguments of spawn sites
    void collectSpawnSiteVisibleObjs();
};




#endif /* THREADESCAPEANALYSIS_H_ */
