/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_DDOS_H
#define NEXUS_LLP_TEMPLATES_DDOS_H

#include <vector>

#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/debug.h>

namespace LLP
{

    /* DoS Wrapper for Returning  */
    template<typename NodeType>
    inline bool DoS(NodeType* pfrom, int nDoS, bool fReturn)
    {
        if(pfrom)
            pfrom->DDOS->rSCORE += nDoS;

        return fReturn;
    }


    /** Class that tracks DDOS attempts on LLP Servers.
        Uses a Timer to calculate Request Score [rScore] and Connection Score [cScore] as a unit of Score / Second.
        Pointer stored by Connection class and Server Listener DDOS_MAP. **/
    class DDOS_Score
    {
        std::vector< std::pair<bool, int> > SCORE;
        Timer TIMER;
        int nIterator;
        std::recursive_mutex MUTEX;


        /** Reset the Timer and the Score Flags to be Overwritten. **/
        void Reset()
        {
            for(int i = 0; i < SCORE.size(); i++)
                SCORE[i].first = false;

            TIMER.Reset();
            nIterator = 0;
        }

    public:

        /** Construct a DDOS Score of Moving Average Timespan. **/
        DDOS_Score(int nTimespan)
        {
            LOCK(MUTEX);

            for(int i = 0; i < nTimespan; i++)
                SCORE.push_back(std::make_pair(true, 0));

            TIMER.Start();
            nIterator = 0;
        }


        /** Flush the DDOS Score to 0. **/
        void Flush()
        {
            LOCK(MUTEX);

            Reset();
            for(int i = 0; i < SCORE.size(); i++)
                SCORE[i].second = 0;
        }


        /** Access the DDOS Score from the Moving Average. **/
        int Score()
        {
            LOCK(MUTEX);

            int nMovingAverage = 0;
            for(int i = 0; i < SCORE.size(); i++)
                nMovingAverage += SCORE[i].second;

            return nMovingAverage / SCORE.size();
        }


        /** Increase the Score by nScore. Operates on the Moving Average to Increment Score per Second. **/
        DDOS_Score & operator+=(const int& nScore)
        {
            LOCK(MUTEX);

            int nTime = TIMER.Elapsed();


            /** If the Time has been greater than Moving Average Timespan, Set to Add Score on Time Overlap. **/
            if(nTime >= SCORE.size())
            {
                Reset();
                nTime = nTime % SCORE.size();
            }


            /** Iterate as many seconds as needed, flagging that each has been iterated. **/
            for(int i = nIterator; i <= nTime; i++)
            {
                if(!SCORE[i].first)
                {
                    SCORE[i].first  = true;
                    SCORE[i].second = 0;
                }
            }


            /** Update the Moving Average Iterator and Score for that Second Instance. **/
            SCORE[nTime].second += nScore;
            nIterator = nTime;


            return *this;
        }
    };


    /** Filter to Contain DDOS Scores and Handle DDOS Bans. **/
    class DDOS_Filter
    {
        Timer TIMER;
        uint32_t BANTIME, TOTALBANS;

    public:
        DDOS_Score rSCORE, cSCORE;
        DDOS_Filter(uint32_t nTimespan) : BANTIME(0), TOTALBANS(0), rSCORE(nTimespan), cSCORE(nTimespan) { }
        std::recursive_mutex MUTEX;

        /** Ban a Connection, and Flush its Scores. **/
        void Ban(std::string strViolation = "SCORE THRESHOLD")
        {
            LOCK(MUTEX);

            if((TIMER.Elapsed() < BANTIME))
                return;

            TIMER.Start();
            TOTALBANS++;

            BANTIME = std::max(TOTALBANS * (rSCORE.Score() + 1) * (cSCORE.Score() + 1), TOTALBANS * 1200u);

            debug::log("XXXXX DDOS Filter cScore = %i rScore = %i Banned for %u Seconds. [VIOLATION: %s]\n", cSCORE.Score(), rSCORE.Score(), BANTIME, strViolation.c_str());

            cSCORE.Flush();
            rSCORE.Flush();
        }

        /** Check if Connection is Still Banned. **/
        bool Banned()
        {
            LOCK(MUTEX);

            uint32_t ELAPSED = TIMER.Elapsed();

            return (ELAPSED < BANTIME);

        }
    };
}

#endif
