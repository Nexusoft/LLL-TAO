/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_MANAGER_H
#define NEXUS_LLP_INCLUDE_MANAGER_H

#include <LLP/include/addressinfo.h>
#include <Util/templates/serialize.h>


#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>
#include <memory>

/* forward declarations */
namespace LLD
{
    class AddressDB;
}

namespace LLP
{

    /** AddressManager
     *
     *  This class is a shared resource that servers can utilize that will
     *  manage state information on addresses used for selecting good connections
     *  to make
     *
     **/
    class AddressManager
    {
    public:

        /** AddressManager
         *
         *  Default constructor
         *
         */
        AddressManager(uint16_t port);


        /** AddressManager
         *
         *  Default destructor
         *
         */
        ~AddressManager();


        /** GetAddresses
         *
         *  Gets a list of addresses in the manager
         *
         *  @return A list of addresses in the manager
         *
         **/
        std::vector<Address> GetAddresses(const uint8_t flags = CONNECT_FLAGS_ALL);


        /** GetInfo
         *
         *  Gets a list of address info in the manager
         *
         *  @return A list of address info in the manager
         *
         **/
        std::vector<AddressInfo> GetInfo(const uint8_t flags = CONNECT_FLAGS_ALL);


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
        void AddAddress(const Address &addr, const uint8_t state = ConnectState::NEW);


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
        void AddAddress(const std::vector<Address> &addrs, const uint8_t state = ConnectState::NEW);


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
        void SetLatency(uint32_t lat, const Address &addr);


        /** StochasticSelect
         *
         *  Select a good address to connect to that isn't already connected.
         *
         *  @param[out] addr The address to fill out
         *
         *  @return True is successful, false otherwise
         *
         **/
        bool StochasticSelect(Address &addr);


        /** ReadDatabase
         *
         *  Read the address database into the manager
         *
         **/
        void ReadDatabase();


        /** WriteDatabase
         *
         *  Write the addresses from the manager into the address database
         *
         **/
        void WriteDatabase();

    private:


        /** get_info
         *
         *  Helper function to get an array of info on the connected states specified
         *  by flags
         *
         *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        std::vector<AddressInfo> get_info(const uint8_t flags = CONNECT_FLAGS_ALL) const;


        /** get_count
         *
         *  Helper function to get the number of addresses of the connect type
         *
         *  *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        uint32_t get_count(const uint8_t flags = CONNECT_FLAGS_ALL) const;

        std::unique_ptr<LLD::AddressDB> pDatabase;
        std::unordered_map<uint64_t, AddressInfo> mapAddrInfo;

        std::mutex mutex;
    };
}

#endif
