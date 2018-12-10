/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/addressinfo.h>
#include <LLP/include/address.h>

namespace LLP
{
    bool operator<(const AddressInfo &info1, const AddressInfo &info2)
    {
        return info1.Score() < info2.Score();
    }


    AddressInfo::AddressInfo(const Address *addr)
    : nHash(0)
    {
        if(addr)
            nHash = addr->GetHash();

        Init();
    }


    AddressInfo::AddressInfo()
    : nHash(0)
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


    /*  Calculates a score based on stats. Lower is better */
    uint32_t AddressInfo::Score() const
    {
        uint32_t score = 0;

        //add up the bad stats
        score += nDropped;
        score += nFailed * 2;
        score += nFails  * 3;
        score += nLatency / 100;

        //divide by the good stats

        score = score / (nConnected == 0 ? 1 : nConnected);

        //divide the score by the total session, in hours
        return score / ((nSession / 3600000) + 1);
    }

}
