/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/addressinfo.h>

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
    bool operator<(const AddressInfo &info1, const AddressInfo &info2)
    {
        double s1 = info1.Score();
        double s2 = info2.Score();

        if(s1 < s2)
            return true;

        /*use latency as a tiebreaker */
        if((s1 == s2) && (info1.nLatency >= info2.nLatency))
            return true;

        return false;
    }

    bool operator<(AddressInfo &info1, AddressInfo &info2)
    {
        double s1 = info1.Score();
        double s2 = info2.Score();

        if(s1 < s2)
            return true;

        /*use latency as a tiebreaker */
        if((s1 == s2) && (info1.nLatency >= info2.nLatency))
            return true;

        return false;
    }

    AddressInfo::AddressInfo()
    : BaseAddress()
    , nSession(0)
    , nLastSeen(0)
    , nConnected(0)
    , nDropped(0)
    , nFailed(0)
    , nFails(0)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , nState(static_cast<uint8_t>(ConnectState::NEW))
    {
    }

    AddressInfo::AddressInfo(const BaseAddress &addr)
    : BaseAddress(addr)
    , nSession(0)
    , nLastSeen(0)
    , nConnected(0)
    , nDropped(0)
    , nFailed(0)
    , nFails(0)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , nState(static_cast<uint8_t>(ConnectState::NEW))
    {
    }

    AddressInfo::AddressInfo(const AddressInfo &other)
    : BaseAddress()
    , nSession(other.nSession)
    , nLastSeen(other.nLastSeen)
    , nConnected(other.nConnected)
    , nDropped(other.nDropped)
    , nFailed(other.nFailed)
    , nFails(other.nFails)
    , nLatency(other.nLatency)
    , nState(other.nState)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }

    AddressInfo::AddressInfo(BaseAddress &addr)
    : BaseAddress(addr)
    , nSession(0)
    , nLastSeen(0)
    , nConnected(0)
    , nDropped(0)
    , nFailed(0)
    , nFails(0)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , nState(static_cast<uint8_t>(ConnectState::NEW))
    {
    }

    AddressInfo::AddressInfo(AddressInfo &other)
    : BaseAddress()
    , nSession(other.nSession)
    , nLastSeen(other.nLastSeen)
    , nConnected(other.nConnected)
    , nDropped(other.nDropped)
    , nFailed(other.nFailed)
    , nFails(other.nFails)
    , nLatency(other.nLatency)
    , nState(other.nState)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }

    AddressInfo::AddressInfo(const AddressInfo &&other)
    : BaseAddress()
    , nSession(other.nSession)
    , nLastSeen(other.nLastSeen)
    , nConnected(other.nConnected)
    , nDropped(other.nDropped)
    , nFailed(other.nFailed)
    , nFails(other.nFails)
    , nLatency(other.nLatency)
    , nState(other.nState)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }

    AddressInfo::AddressInfo(AddressInfo &&other)
    : BaseAddress()
    , nSession(other.nSession)
    , nLastSeen(other.nLastSeen)
    , nConnected(other.nConnected)
    , nDropped(other.nDropped)
    , nFailed(other.nFailed)
    , nFails(other.nFails)
    , nLatency(other.nLatency)
    , nState(other.nState)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }

    AddressInfo::~AddressInfo()
    {
    }

    AddressInfo &AddressInfo::operator=(const AddressInfo &other)
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
        this->nState = other.nState;

        return *this;
    }

    AddressInfo &AddressInfo::operator=(AddressInfo &other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        nPort = other.nPort;

        nSession = other.nSession;
        nLastSeen = other.nLastSeen;
        nConnected = other.nConnected;
        nDropped = other.nDropped;
        nFailed = other.nFailed;
        nFails = other.nFails;
        nLatency = other.nLatency;
        nState = other.nState;

        return *this;
    }

    AddressInfo &AddressInfo::operator=(const AddressInfo &&other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        nPort = other.nPort;

        nSession = other.nSession;
        nLastSeen = other.nLastSeen;
        nConnected = other.nConnected;
        nDropped = other.nDropped;
        nFailed = other.nFailed;
        nFails = other.nFails;
        nLatency = other.nLatency;
        nState = other.nState;

        return *this;
    }

    AddressInfo &AddressInfo::operator=(AddressInfo &&other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        nPort = other.nPort;

        nSession = other.nSession;
        nLastSeen = other.nLastSeen;
        nConnected = other.nConnected;
        nDropped = other.nDropped;
        nFailed = other.nFailed;
        nFails = other.nFails;
        nLatency = other.nLatency;
        nState = other.nState;

        return *this;
    }

    /*  Calculates a score based on stats. Higher is better */
    double AddressInfo::Score() const
    {
        double nLat = static_cast<double>(nLatency);

        double nLatencyScore = ::nMaxLatency - std::min(::nMaxLatency, nLat);

        /* Add up the good stats */
        double good = ::nConnectedWeight * std::min(nConnected, ::nMaxConnected) +
                      nLatencyScore * ::nLatencyWeight;

        /* Add up the bad stats */
        double bad = ::nDroppedWeight * std::min(nDropped, ::nMaxDropped) +
                     ::nFailedWeight  * std::min(nFailed,  ::nMaxFailed)  +
                     ::nFailsWeight   * std::min(nFails,   ::nMaxFails);

        /* Subtract good stats by bad stats */
        return good - bad;
    }
}
