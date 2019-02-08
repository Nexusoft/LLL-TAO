/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_MANAGER_H
#define NEXUS_LLP_INCLUDE_MANAGER_H

#include <LLP/include/trust_address.h>
#include <map>
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

        AddressManager() = delete;

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
         *  Get a list of addresses in the manager that have the flagged state.
         *
         *  @param[in] flags the types of connect state
         *
         *  @return A list of addresses in the manager
         *
         **/
        void GetAddresses(std::vector<BaseAddress> &vAddr, const uint8_t flags = CONNECT_FLAGS_ALL);


        /** GetAddresses
         *
         *  Gets a list of address info in the manager
         *
         *  @param[out] vAddrInfo The vector of address info output
         *
         *  @param[in] flags the types of connect state
         *
         **/
        void GetAddresses(std::vector<TrustAddress> &vAddrInfo, const uint8_t flags = CONNECT_FLAGS_ALL);

        /** Count
         *
         *  Gets the count of addresses info in the manager
         *
         *  @param[in] flags the types of connect state
         *
         *  @return Count of addresss info with connect state flags
         *
         **/
        uint32_t Count(const uint8_t flags = CONNECT_FLAGS_ALL);


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
         *  @param[in] state The state of the connection for the address
         *
         **/
        void AddAddresses(const std::vector<BaseAddress> &addrs, const uint8_t state = ConnectState::NEW);
        void AddAddresses(const std::vector<std::string> &addrs, const uint8_t state = ConnectState::NEW);


        /** RemoveAddress
         *
         *  Removes an address from the manager if it exists.
         *
         *  @param[in] addr The address to remove.
         *
         **/
        void RemoveAddress(const BaseAddress &addr);


        /** AddSeedAddresses
         *
         *  Adds the seed node addresses to the addressmanager if they aren't
         *  already in there.
         *
         *  @param[in] testnet Flag for if testnet seed nodes should be added.
         *
         **/
        void AddSeedAddresses(bool testnet = false);


        /** Has
         *
         *  Determines if the address manager has the address or not.
         *
         *  @param[in] addr The address to find.
         *
         **/
        bool Has(const BaseAddress &addr) const;


        /** GetState
         *
         *  Gets the Connect State of the address in the manager if it exists.
         *
         *  @param[in] addr The address to get the state from.
         *
         *  @return Returns the connect state of the address.
         *
         **/
        uint8_t GetState(const BaseAddress &addr) const;


        /** SetLatency
         *
         *  Finds the managed address info and sets the latency experienced by
         *  that address.
         *
         *  @param[in] lat The latency, in milliseconds
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


        /** ToString
         *
         *  Print the current state of the address manager.
         *
         **/
        std::string ToString();


        /** SetPort
         *
         *  Set the port number for all addresses in the manager.
         *
         **/
        void SetPort(uint16_t port);

    private:

        /** get_addresses
         *
         *  Helper function to get an array of info on the connected states specified
         *  by flags
         *
         *  @param[out] info The resulting outputted address info vector
         *
         *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        void get_addresses(std::vector<TrustAddress> &vInfo, const uint8_t flags = CONNECT_FLAGS_ALL);


        /** count
         *
         *  Helper function to get the number of addresses of the connect type
         *
         *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        uint32_t count(const uint8_t flags = CONNECT_FLAGS_ALL);


        /** total_count
         *
         *  Gets the cumulative count of each address state flags.
         *
         *  @param[in] flags Specify which types of connections to get the info from.
         *
         **/
        uint32_t total_count(const uint8_t flags);


        /** to_string
         *
         *  Print the current state of the address manager.
         *
         **/
        std::string to_string();


        LLD::AddressDB *pDatabase;
        std::map<uint64_t, TrustAddress> mapTrustAddress;
        mutable std::mutex mut;
        uint16_t nPort;
    };
}

#endif
