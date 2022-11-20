/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/ddos.h>

namespace LLP
{

    /*  Construct a DDOS Score of Moving Average Timespan. */
    DDOS_Score::DDOS_Score(int nTimespan)
    : SCORE()
    , TIMER()
    , nIterator()
    , MUTEX()
    {
        LOCK(MUTEX);

        for(int i = 0; i < nTimespan; ++i)
            SCORE.push_back(std::make_pair(true, 0));

        TIMER.Start();
        nIterator = 0;
    }

    /** Default Destructor. **/
    DDOS_Score::~DDOS_Score()
    {
        SCORE.clear();
    }


    /*  Reset the Timer and the Score Flags to be Overwritten. */
    void DDOS_Score::reset()
    {
        for(uint32_t i = 0; i < SCORE.size(); ++i)
            SCORE[i].first = false;

        TIMER.Reset();
        nIterator = 0;
    }


     /* Flush the DDOS Score to 0. */
    void DDOS_Score::Flush()
    {
        LOCK(MUTEX);

        reset();
        for(uint32_t i = 0; i < SCORE.size(); ++i)
            SCORE[i].second = 0;
    }


     /*  Access the DDOS Score from the Moving Average. */
    int32_t DDOS_Score::Score() const
    {
        LOCK(MUTEX);

        uint32_t nMovingAverage = 0;
        for(int32_t i = 0; i < SCORE.size(); ++i)
            nMovingAverage += SCORE[i].second;

        return nMovingAverage / SCORE.size();
    }


     /* Increase the Score by nScore. Operates on the Moving Average to
      *  Increment Score per Second. */
    DDOS_Score &DDOS_Score::operator+=(const uint32_t& nScore)
    {
        LOCK(MUTEX);

        if(nScore)
            debug::log(4, FUNCTION, "DDOS Penalty of +", nScore);

        uint32_t nTime = TIMER.Elapsed();

        /* If the Time has been greater than Moving Average Timespan, Set to Add Score on Time Overlap. */
        if(nTime >= SCORE.size())
        {
            reset();
            nTime = nTime % static_cast<int32_t>(SCORE.size());
        }

        /* Iterate as many seconds as needed, flagging that each has been iterated. */
        for(uint32_t i = nIterator; i <= nTime; ++i)
        {
            if(!SCORE[i].first)
            {
                SCORE[i].first  = true;
                SCORE[i].second = 0;
            }
        }

        /* Update the Moving Average Iterator and Score for that Second Instance. */
        SCORE[nTime].second += nScore;
        nIterator = nTime;

        return *this;
    }


    /** print
     *
     *  Print out the internal data inside the score object.
     *
     **/
    void DDOS_Score::print()
    {
        for(int32_t i = 0; i < SCORE.size(); ++i)
            printf(" %s %u |", SCORE[i].first ? "T" : "F", SCORE[i].second);

        printf("\n");
    }


    /* Default Constructor */
    DDOS_Filter::DDOS_Filter(const uint32_t nTimespan)
    : nTotalBans    (0)
    , nBanTimestamp (0)
    , rSCORE        (nTimespan)
    , cSCORE        (nTimespan)
    {
    }


    /* Ban a Connection, and Flush its Scores. */
    void DDOS_Filter::Ban(const std::string& strViolation)
    {
        /* Debug output. */
        rSCORE.print();
        ++nTotalBans;

        /* Calculate penalty. */
        uint32_t nPenalty = std::max(nTotalBans.load() * (rSCORE.Score() + 1) * (cSCORE.Score() + 1), nTotalBans.load() * 1200u);

        debug::drop(ANSI_COLOR_BRIGHT_RED, "XXXXX DDOS Filter XXXXX: ", ANSI_COLOR_RESET, " cScore = ", cSCORE.Score(),
            " rScore = ", rSCORE.Score(),
            " Banned for ", nPenalty,
            " Seconds. ", ANSI_COLOR_BRIGHT_RED, "[VIOLATION: ", strViolation, "]", ANSI_COLOR_RESET);

        /* Set new timestamp. */
        nBanTimestamp.store(runtime::unifiedtimestamp() + nPenalty);

        cSCORE.Flush();
        rSCORE.Flush();
    }


    /* Check if Connection is Still Banned. */
    bool DDOS_Filter::Banned() const
    {
        return runtime::unifiedtimestamp() < nBanTimestamp.load();
    }

}
