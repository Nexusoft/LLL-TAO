/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_ADDRESSINFO_H
#define NEXUS_LLP_INCLUDE_ADDRESSINFO_H

#include <cstdint>
#include <Util/templates/serialize.h>
#include <LLP/include/baseaddress.h>

namespace LLP
{
    enum ConnectState
    {
        NEW        = (1 << 0),
        FAILED     = (1 << 1),
        DROPPED    = (1 << 2),
        CONNECTED  = (1 << 3)
    };

    #define CONNECT_FLAGS_ALL ConnectState::NEW       | \
                              ConnectState::FAILED    | \
                              ConnectState::DROPPED   | \
                              ConnectState::CONNECTED


    /** AddressInfo
     *
     *  This is a basic struct for keeping statistics on addresses and is used
     *  for handling and tracking connections in a meaningful way
     *
     **/
    class AddressInfo : public BaseAddress
    {
    public:


        /** AddressInfo
         *
         *  Default constructor
         *
         *  @param[in] addr The address to initalize associated hash
         *
         **/
        AddressInfo();
        AddressInfo(const BaseAddress &addr);
        AddressInfo(const AddressInfo &other);

        AddressInfo(BaseAddress &other) = delete;
        AddressInfo(AddressInfo &other) = delete;


        /** ~AddressInfo
         *
         *  Default destructor
         *
         **/
        virtual ~AddressInfo();

        AddressInfo &operator=(const AddressInfo &other);


        /* Serialization */
        IMPLEMENT_SERIALIZE
        (
            AddressInfo *pthis = const_cast<AddressInfo *>(this);
            BaseAddress *pAddr = (BaseAddress *)pthis;

            READWRITE(nSession);
            READWRITE(nConnected);
            READWRITE(nDropped);
            READWRITE(nFailed);
            READWRITE(nFails);
            READWRITE(nLatency);
            READWRITE(*pAddr);
        )


        /** Score
         *
         *  Calculates a score based on stats. Lower is better
         *
         **/
        double Score() const;


        uint64_t nSession;    //total time since connected
        uint64_t nLastSeen;   //unified time last seen
        uint32_t nConnected;  //total number of successful connections
        uint32_t nDropped;    //total number of dropped connections
        uint32_t nFailed;     //total number of failed connections
        uint32_t nFails;      //consecutive number of failed connections
        uint32_t nLatency;    //the latency experienced by the connection
        uint8_t  nState;      //the flag for the state of connection

        friend bool operator<(const AddressInfo &info1, const AddressInfo &info2);
    };

}

#endif
