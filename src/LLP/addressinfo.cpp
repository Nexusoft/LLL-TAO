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
    const uint32_t nConnectedWeight = 100;
    const uint32_t nDroppedWeight = 2;
    const uint32_t nFailedWeight = 5;
    const uint32_t nFailsWeight = 10;
    const uint32_t nLatencyWeight = 100;

    const uint32_t nMaxConnected = 100;
    const uint32_t nMaxDropped = 50;
    const uint32_t nMaxFailed = 50;
    const uint32_t nMaxFails = 10;
}

namespace LLP
{
    bool operator<(const AddressInfo &info1, const AddressInfo &info2)
    {
        return info1.Score() < info2.Score();
    }


    AddressInfo::AddressInfo(const Address &addr)
    : Address(addr)
    {
        Init();
    }


    AddressInfo::AddressInfo()
    : Address()
    {
        Init();
    }

    AddressInfo::~AddressInfo() { }


    void AddressInfo::Init()
    {
        nLastSeen = 0;
        nSession = 0;
        nConnected = 0;
        nDropped = 0;
        nFailed = 0;
        nFails = 0;
        nLatency = 0;
        nState = static_cast<uint8_t>(ConnectState::NEW);
    }


    /*  Calculates a score based on stats. Higher is better */
    double AddressInfo::Score() const
    {

        double nSessionHours = static_cast<double>(nSession) / 3600000;
        double nLatencyScore = static_cast<double>(nLatency) / nLatencyWeight;

        /* Add up the good stats */
        double good = std::min(nConnected, nMaxConnected) * nConnectedWeight +
                      nSessionHours;

        /* Add up the bad stats */
        double bad = std::min(nDropped, nMaxDropped) * nDroppedWeight +
                     std::min(nFailed,  nMaxFailed)  * nFailedWeight  +
                     std::min(nFails,   nMaxFails)   * nFailsWeight   +
                     nLatencyScore;

        /* Subtract good stats by bad stats */
        return good - bad;
    }
}
