/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_MANAGER_H
#define NEXUS_LLP_INCLUDE_MANAGER_H

#include <LLD/include/address.h>
#include <LLP/include/network.h>
#include <Util/templates/serialize.h>


#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <mutex>


namespace LLP
{


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
        std::vector<CAddress> GetAddresses(const uint8_t flags = CONNECT_FLAGS_ALL);


        /** GetInfo
         *
         *  Gets a list of address info in the manager
         *
         *  @return A list of address info in the manager
         *
         **/
        std::vector<CAddressInfo> GetInfo(const uint8_t flags = CONNECT_FLAGS_ALL);


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
        void AddAddress(const CAddress &addr, const uint8_t state = ConnectState::NEW);


        /** AddAddress
         *
         *  Adds the addresses to the manager and sets the connect state for that
         *  address
         *
         *  @param[in] addrs The addresses to add
         *
         *  @param[in] state The state of the connection for the address
         *
         **/
        void AddAddress(const std::vector<CAddress> &addrs, const uint8_t state = ConnectState::NEW);


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

        LLD::AddressDB *addrDatabase;

        std::vector<CAddressInfo> get_info(const uint8_t flags = CONNECT_FLAGS_ALL) const;

        std::unordered_map<uint64_t, CAddress> mapAddr;
        std::unordered_map<uint64_t, CAddressInfo> mapInfo;

        std::mutex mutex;
    };
}

#endif
