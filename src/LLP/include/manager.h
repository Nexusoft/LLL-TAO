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


        /** IsEmpty
         *
         *  Determine if the address manager has any addresses in it or not
         *
         *  @return True if no addresses exist. false otherwise.
         *
         **/
        bool IsEmpty() const;


        /** GetAddresses
         *
         *  Gets a list of addresses in the manager
         *
         *  @param[in] flags the types of connect state
         *
         *  @return A list of addresses in the manager
         *
         **/
        std::vector<BaseAddress> GetAddresses(const uint8_t flags = CONNECT_FLAGS_ALL);


        /** GetInfo
         *
         *  Gets a list of address info in the manager
         *
         *  @param[out] vAddrInfo The vector of address info output
         *
         *  @param[in] flags the types of connect state
         *
         **/
        void GetInfo(std::vector<AddressInfo> &vAddrInfo, const uint8_t flags = CONNECT_FLAGS_ALL);

        /** GetInfoCount
         *
         *  Gets the count of addresses info in the manager
         *
         *  @param[in] flags the types of connect state
         *
         *  @return Count of addresss info with connect state flags
         *
         **/
        uint32_t GetInfoCount(const uint8_t flags = CONNECT_FLAGS_ALL);


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
        void AddAddress(const BaseAddress &addr, const uint8_t state = ConnectState::NEW);


        /** AddAddresses
         *
         *  Adds the addresses to the manager and sets the connect state for that
         *  address
         *
         *  @param[in] addrs The addresses to add
         *
         *  @param[in] state The state of the connection for the address
         *
         **/
        void AddAddresses(const std::vector<BaseAddress> &addrs, const uint8_t state = ConnectState::NEW);


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
        void SetLatency(uint32_t lat, const BaseAddress &addr);


        /** StochasticSelect
         *
         *  Select a good address to connect to that isn't already connected.
         *
         *  @param[out] addr The address to fill out
         *
         *  @return True is successful, false otherwise
         *
         **/
        bool StochasticSelect(BaseAddress &addr);


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


        /** PrintStats
         *
         *  print the current state of the address manager
         *
         **/
        void PrintStats();

    private:

        /** print_stats
         *
         *  print the current state of the address manager
         *
         **/
        void print_stats();


        /** get_info
         *
         *  Helper function to get an array of info on the connected states specified
         *  by flags
         *
         *  @param[out] info The resulting outputted address info vector
         *
         *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        void get_info(std::vector<AddressInfo> &info, const uint8_t flags = CONNECT_FLAGS_ALL);


        /** get_info_count
         *
         *  Helper function to get the number of addresses of the connect type
         *
         *  *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        uint32_t get_info_count(const uint8_t flags = CONNECT_FLAGS_ALL);


        /** get_total_count
         *
         *  Helper function to get the total connected, dropped, or failed attempts
         *  from the entire data set
         *
         *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        uint32_t get_total_count(const uint8_t flags);


        LLD::AddressDB *pDatabase;
        std::unordered_map<uint64_t, AddressInfo> mapAddrInfo;

        mutable std::mutex mut;
    };
}

#endif
