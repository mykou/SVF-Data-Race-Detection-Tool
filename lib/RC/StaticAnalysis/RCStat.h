/*
 * RCStat.h
 *
 *  Created on: 29/07/2015
 *      Author: dye
 */

#ifndef RCSTAT_H_
#define RCSTAT_H_

#include "Util/AnalysisUtil.h"
#include "Util/BasicTypes.h"
#include <map>
#include <chrono>


/*!
 * RaceComb Statistics
 */
class RCStat {
public:
    /// Timing
    enum StatTarget {
        Stat_TotalAnalysis,      ///< Total analysis
        Stat_PreAnalysis,        ///< Pre-analysis
        Stat_RcAnalysis,         ///< RaceComb analysis
        Stat_OpCollection,       ///< Operation collection
        Stat_MemPart,            ///< Memory partitioning
        Stat_EscapeAnalysis,     ///< Thread escape analysis
        Stat_MhpAnalysis,        ///< Mhp analysis
        Stat_LocksetAnalysis,    ///< Lockset analysis
        Stat_BarrierAnalysis,    ///< Barrier analysis
        Stat_FurtherRefinement,  ///< Further refinement
        Stat_CsRefinement        ///< Context-sensitive refinement
    };
    class StatInfo {
    public:
        StatInfo(const char *s) : description(s) {
        }
        typedef std::chrono::high_resolution_clock Clock;
        std::string description;
        Clock::time_point begin;
        Clock::time_point end;
    };
    typedef std::map<StatTarget, StatInfo> TIMEStatMap;


    /// Constructor
    RCStat() {
        initTimeStatMap();
    }

    /// Destructor
    virtual ~RCStat() {
    }

    /// Timmer functions to time a given statistic item.
    /// @param statItem the given statistic item to be timed.
    //@{
    inline void startTiming(StatTarget target) {
        auto iter = timeStatMap.find(target);
        assert(iter != timeStatMap.end() && "Unknown StatTarget.");
        iter->second.begin = StatInfo::Clock::now();
    }
    inline void endTiming(StatTarget target) {
        auto iter = timeStatMap.find(target);
        assert(iter != timeStatMap.end() && "Unknown StatTarget.");
        iter->second.end = StatInfo::Clock::now();
    }
    //@}

    /// Dump the statistics
    void print() const;

protected:
    /// Initialize StatMap
    void initTimeStatMap() {
        timeStatMap.insert(TIMEStatMap::value_type(Stat_TotalAnalysis,      "Total Analysis "));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_PreAnalysis,        " | Pre-analysis "));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_RcAnalysis,         " | RaceComb Analysis "));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_OpCollection,       "   | Operation Collection"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_MemPart,            "   | Memory Partitioning"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_EscapeAnalysis,     "   | Thread Escape Analysis"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_MhpAnalysis,        "   | MHP Analysis"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_LocksetAnalysis,    "   | Lockset Analysis"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_BarrierAnalysis,    "   | Barrier Analysis"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_FurtherRefinement,  "   | Further Refinement"));
        timeStatMap.insert(TIMEStatMap::value_type(Stat_CsRefinement,       "   | CS Refinement"));
    }

    TIMEStatMap timeStatMap;
};





#endif /* RCSTAT_H_ */
