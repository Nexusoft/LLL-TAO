/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/trust_address.h>

namespace
{
    /* constant variables to tweak score */
    const double nConnectedWeight = 100.0;
    const double nDroppedWeight = 2.0;
    const double nFailedWeight = 5.0;
    const double nFailsWeight = 10.0;
    const double nLatencyWeight = 10.0;

    const uint32_t nMaxConnected = 100;
    const uint32_t nMaxDropped = 5000;
    const uint32_t nMaxFailed = 2000;
    const uint32_t nMaxFails = 1000;
    const double nMaxLatency = 1000.0;

}

namespace LLP
{
    TrustAddress::TrustAddress()
    : BaseAddress()
    , nSession(0)
    , nLastSeen(0)
    , nConnected(0)
    , nDropped(0)
    , nFailed(0)
    , nFails(0)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , nHeight(0)
    , nState(static_cast<uint8_t>(ConnectState::NEW))
    , nType(0)
    {
    }


    /*  Copy constructor */
    TrustAddress::TrustAddress(const TrustAddress &other)
    : BaseAddress()
    , nSession(other.nSession)
    , nLastSeen(other.nLastSeen)
    , nConnected(other.nConnected)
    , nDropped(other.nDropped)
    , nFailed(other.nFailed)
    , nFails(other.nFails)
    , nLatency(other.nLatency)
    , nHeight(other.nHeight)
    , nState(other.nState)
    , nType(0)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }


    /*  Copy constructors */
    TrustAddress::TrustAddress(const BaseAddress &other)
    : BaseAddress(other)
    , nSession(0)
    , nLastSeen(0)
    , nConnected(0)
    , nDropped(0)
    , nFailed(0)
    , nFails(0)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , nHeight(0)
    , nState(static_cast<uint8_t>(ConnectState::NEW))
    , nType(0)
    {
    }


    /* Default destructor */
    TrustAddress::~TrustAddress()
    {
    }


    /* Copy assignment operator */
    TrustAddress &TrustAddress::operator=(const TrustAddress &other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            this->ip[i] = other.ip[i];

        this->nPort = other.nPort;

        this->nSession = other.nSession;
        this->nLastSeen = other.nLastSeen;
        this->nConnected = other.nConnected;
        this->nDropped = other.nDropped;
        this->nFailed = other.nFailed;
        this->nFails = other.nFails;
        this->nLatency = other.nLatency;
        this->nHeight = other.nHeight;
        this->nState = other.nState;
        this->nType = other.nType;

        return *this;
    }


    /* Copy assignment operator */
    TrustAddress &TrustAddress::operator=(const BaseAddress &other)
    {
        this->SetPort(other.GetPort());
        this->SetIP(other);

        this->nSession = 0;
        this->nLastSeen = 0;
        this->nConnected = 0;
        this->nDropped = 0;
        this->nFailed = 0;
        this->nFails = 0;
        this->nLatency = std::numeric_limits<uint32_t>::max();
        this->nHeight = 0;
        this->nState = static_cast<uint8_t>(ConnectState::NEW);
        this->nType = 0;

        return *this;
    }


    /* Calculates a score based on stats. A higher score is better. */
    double TrustAddress::Score() const
    {
        double nLat = static_cast<double>(nLatency);

        double nLatencyScore = ::nMaxLatency - std::min(::nMaxLatency, nLat);

        /* Add up the good stats. */
        double good = ::nConnectedWeight * std::min(nConnected, ::nMaxConnected) +
                      nLatencyScore * ::nLatencyWeight;

        /* Add up the bad stats. */
        double bad = ::nDroppedWeight * std::min(nDropped, ::nMaxDropped) +
                     ::nFailedWeight  * std::min(nFailed,  ::nMaxFailed)  +
                     ::nFailsWeight   * std::min(nFails,   ::nMaxFails);

        /* Subtract the bad stats from the good stats. */
        return good - bad;
    }

    /* Prints information about this address. */
    void TrustAddress::Print()
    {
        debug::log(0, "TrustAddress(", ToString(), ")");
        debug::log(0, ":\t", "nSession=", nSession);
        debug::log(0, ":\t", "nLastSeen=", nLastSeen);
        debug::log(0, ":\t", "nConnected=", nConnected);
        debug::log(0, ":\t", "nDropped=", nDropped);
        debug::log(0, ":\t", "nFailed=", nFailed);
        debug::log(0, ":\t", "nFails=", nFails);
        debug::log(0, ":\t", "nLatency=", nLatency);
        debug::log(0, ":\t", "nHeight=", nHeight);
        debug::log(0, ":\t", "nState=",  static_cast<uint32_t>(nState));
        debug::log(0, ":\t", "nType=", static_cast<uint32_t>(nType));
    }

    /* Comparison less than operator used for sorting */
    bool operator<(const TrustAddress &info1, const TrustAddress &info2)
    {
        const double s1 = info1.Score();
        const double s2 = info2.Score();

        if(s1 < s2)
            return true;

        /*use latency as a tiebreaker */
        if((s1 == s2) && (info2.nLatency < info1.nLatency))
            return true;

        return false;
    }
}
