/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

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
    inline bool DoS(NodeType* pfrom, int nDoS, bool fReturn)
    {
        if(pfrom)
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
        std::vector< std::pair<bool, int> > SCORE;
        runtime::Timer TIMER;
        int nIterator;
        std::recursive_mutex MUTEX;

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


        /** Reset
         *
         *  Reset the Timer and the Score Flags to be Overwritten.
         *
         **/
        void Reset();


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
        int32_t Score();


        /** operator+=
         *
         *  Increase the Score by nScore. Operates on the Moving Average to
         *  Increment Score per Second.
         *
         *  @param[in] nScore rhs DDOS_Score to add
         *
         **/
        DDOS_Score &operator+=(const int& nScore);

    };


    /** DDOS_Filter
     *
     * Filter to Contain DDOS Scores and Handle DDOS Bans.
     *
     **/
    class DDOS_Filter
    {
        runtime::Timer TIMER;
        uint32_t BANTIME, TOTALBANS;

    public:

        DDOS_Score rSCORE, cSCORE;
        std::recursive_mutex MUTEX;

        /** DDOS_Filter
         *
         *  Default Constructor
         *
         * @param[in] nTimespan The timespan to initialize scores with
         *
         **/
        DDOS_Filter(uint32_t nTimespan);


        /** Ban
         *
         *  Ban a Connection, and Flush its Scores.
         *
         *  @param[in] strViolation identifiable tag for the violation
         *
         **/
        void Ban(std::string strViolation = "SCORE THRESHOLD");


        /** Banned
         *
         *  Check if Connection is Still Banned.
         *
         *  @return True if elapsed time is less than bantime, false otherwise
         *
         **/
        bool Banned();
    };
}

#endif
