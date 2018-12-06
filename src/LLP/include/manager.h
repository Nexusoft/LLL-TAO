/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_MANAGER_H
#define NEXUS_LLP_INCLUDE_MANAGER_H

#include <LLP/include/network.h>
#include <Util/templates/serialize.h>

#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <mutex>


namespace LLP
{
    enum ConnectState
    {
        NEW        = (1 << 0),
        FAILED     = (1 << 1),
        DROPPED    = (1 << 2),
        CONNECTED  = (1 << 3)
    };

    #define CONNECT_FLAGS_DEFAULT ConnectState::NEW       | \
                                  ConnectState::FAILED    | \
                                  ConnectState::DROPPED   | \
                                  ConnectState::CONNECTED


    /** CAddressInfo
     *
     *  This is a basic struct for keeping statistics on addresses and is used
     *  for handling and tracking connections in a meaningful way
     *
     **/
    struct CAddressInfo
    {

        /** CAddressInfo
         *
         *  Default constructor
         *
         *  @param[in] addr The address to initalize associated hash
         *
         **/
        CAddressInfo(const CAddress &addr);
        CAddressInfo();


        /** ~CAddressInfo
         *
         *  Default destructor
         *
         **/
        ~CAddressInfo();


        /* Serialization */
        IMPLEMENT_SERIALIZE(
            READWRITE(nLastSeen);
            READWRITE(nSession);
            READWRITE(nConnected);
            READWRITE(nDropped);
            READWRITE(nFailed);
            READWRITE(nFails);
            READWRITE(nLatency);
        )


        /** Init
         *
         *  Initalizes stats to zero.
         *
         **/
        void Init();


        /** Score
         *
         *  Calculates a score based on stats. Lower is better
         *
         **/
        uint32_t Score() const;

        uint64_t nHash;       //hash associated with CAddress
        int64_t  nLastSeen;   //unified time last seen
        int64_t  nSession;    //total time since connected
        uint32_t nConnected;  //total number of successful connections
        uint32_t nDropped;    //total number of dropped connections
        uint32_t nFailed;     //total number of failed connections
        uint32_t nFails;      //consecutive number of failed connections
        uint32_t nLatency;    //the latency experienced by the connection
        uint8_t  nState;      //the flag for the state of connection

        friend bool operator<(const CAddressInfo &info1, const CAddressInfo &info2);
    };


    /** CAddressManager
     *
     *  This class is a shared resource that servers can utilize that will
     *  manage state information on addresses used for selecting good connections
     *  to make
     *
     **/
    class CAddressManager
    {
    public:

        /** CAddressManager
         *
         *  Default constructor
         *
         */
        CAddressManager();


        /** CAddressManager
         *
         *  Default destructor
         *
         */
        ~CAddressManager();


        /** GetAddresses
         *
         *  Gets a list of addresses in the manager
         *
         *  @return A list of addresses in the manager
         *
         **/
        std::vector<CAddress> GetAddresses(const uint8_t flags = CONNECT_FLAGS_DEFAULT);


        /** GetInfo
         *
         *  Gets a list of address info in the manager
         *
         *  @return A list of address info in the manager
         *
         **/
        std::vector<CAddressInfo> GetInfo(const uint8_t flags = CONNECT_FLAGS_DEFAULT);


        /** AddAddress
         *
         *  Adds the address to the manager and sets the connect state for that
         *  address
         *
         *  @param[in] addr The address to add
         *
         *  @param[in] state The state of the connection for the address
         *
         **/
        void AddAddress(const CAddress &addr, const uint8_t &state = ConnectState::NEW);


        /** SetLatency
         *
         *  Finds the managed address info and sets the latency experienced by
         *  that address.
         *
         *  @param[in] lat The latency, in milliseconds
         *
         *  @param[in] addr The address in reference to
         *
         **/
        void SetLatency(uint32_t lat, const CAddress &addr);


        /** StochasticSelect
         *
         *  Select a good address to connect to that isn't already connected.
         *
         *  @param[out] addr The address to fill out
         *
         *  @return True is successful, false otherwise
         *
         **/
        bool StochasticSelect(CAddress &addr);

    private:

        /** get_info
         *
         *  Helper function to get an array of info on the connected states specified
         *  by flags
         *
         **/
        std::vector<CAddressInfo> get_info(const uint8_t flags = CONNECT_FLAGS_DEFAULT) const;

        std::unordered_map<uint64_t, CAddress> mapAddr;
        std::unordered_map<uint64_t, CAddressInfo> mapInfo;

        std::mutex mutex;
    };
}

#endif
