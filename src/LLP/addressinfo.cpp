/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/addressinfo.h>

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
        /* Add up the bad stats */
        double bad = (nFailed * 8) + (nFails * 12 ) + (nLatency / 100);

        /* Divide by zero check */
        if(bad == 0)
            bad = 1;

        /* Add up the good stats */
        double good = (nDropped * 50) + (nConnected * 100) + (nSession / 3600000);


        /* Subtract good stats by bad stats */
        return good - bad;
    }
}
