/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_DDOS_H
#define NEXUS_LLP_TEMPLATES_DDOS_H

#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/debug.h>
#include <vector>

namespace LLP
{

    /** DoS
     *
     *  Wrapper for Returning
     *
     **/
    template<typename NodeType>
    inline bool DoS(NodeType* pfrom, uint32_t nDoS, bool fReturn)
    {
        if(pfrom && pfrom->fDDOS.load())
            pfrom->DDOS->rSCORE += nDoS;

        return fReturn;
    }


    /** DDOS_Score
     *
     *  Class that tracks DDOS attempts on LLP Servers.
     *
     *  Uses a Timer to calculate Request Score [rScore] and Connection Score [cScore]
     *  as a unit of Score / Second. Pointer stored by Connection class and
     *  Server Listener DDOS_MAP.
     *
     **/
    class DDOS_Score
    {
        /** Moving Average for tracking DDOS score thresholds. **/
        std::vector< std::pair<bool, uint32_t> > SCORE;


        /** Track time since last call for calculating overlap. **/
        runtime::timer TIMER;


        /** Current time iterator in minutes. **/
        uint32_t nIterator;


        /** Internal mutex for thread safety. **/
        mutable std::mutex MUTEX;


        /** Reset
         *
         *  Reset the Timer and the Score Flags to be Overwritten.
         *
         **/
        void reset();

    public:

        /** DDOS_Score
         *
         *  Construct a DDOS Score of Moving Average Timespan.
         *
         *  @param[in] nTimespan number of scores to create to build a
         *             moving average score
         *
         **/
        DDOS_Score(int nTimespan);


        /** Default Destructor. **/
        ~DDOS_Score();


        /** Flush
         *
         * Flush the DDOS Score to 0.
         *
         **/
        void Flush();


        /** Score
         *
         *  Access the DDOS Score from the Moving Average.
         *
         *  @return the DDOS score
         *
         **/
        int32_t Score() const;


        /** operator+=
         *
         *  Increase the Score by nScore. Operates on the Moving Average to
         *  Increment Score per Second.
         *
         *  @param[in] nScore rhs DDOS_Score to add
         *
         **/
        DDOS_Score &operator+=(const uint32_t& nScore);


        /** print
         *
         *  Print out the internal data inside the score object.
         *
         **/
        void print();

    };


    /** DDOS_Filter
     *
     * Filter to Contain DDOS Scores and Handle DDOS Bans.
     *
     **/
    class DDOS_Filter
    {
        /** Keep track of the total times banned. **/
        std::atomic<uint32_t> nTotalBans;


        /** Timestamp in the future when ban is over. **/
        std::atomic<uint64_t> nBanTimestamp;


    public:

        /** R-Score or Request Score regulating packet flow. **/
        DDOS_Score rSCORE;


        /** C-Score or Connection Score regulating connection requests. **/
        DDOS_Score cSCORE;


        /** DDOS_Filter
         *
         *  Default Constructor
         *
         * @param[in] nTimespan The timespan to initialize scores with
         *
         **/
        DDOS_Filter(const uint32_t nTimespan);


        /** Ban
         *
         *  Ban a Connection, and Flush its Scores.
         *
         *  @param[in] strViolation identifiable tag for the violation
         *
         **/
        void Ban(const std::string& strViolation = "SCORE THRESHOLD");


        /** Banned
         *
         *  Check if Connection is Still Banned.
         *
         *  @return True if elapsed time is less than bantime, false otherwise
         *
         **/
        bool Banned() const;
    };
}

#endif
