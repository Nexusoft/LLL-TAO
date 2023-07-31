/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
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
        AddressManager(uint16_t nPort);


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
         *  Get a list of addresses in the manager that have the flagged nState.
         *
         *  @param[in] nFlags the types of connect nState
         *
         *  @return A list of addresses in the manager
         *
         **/
        void GetAddresses(std::vector<BaseAddress> &vAddr, const uint8_t nFlags = CONNECT_FLAGS_ALL);


        /** GetAddresses
         *
         *  Gets a list of address info in the manager
         *
         *  @param[out] vAddrInfo The vector of address info output
         *
         *  @param[in] nFlags the types of connect nState
         *
         **/
        void GetAddresses(std::vector<TrustAddress> &vAddrInfo, const uint8_t nFlags = CONNECT_FLAGS_ALL);


        /** Count
         *
         *  Gets the count of addresses info in the manager
         *
         *  @param[in] nFlags the types of connect nState
         *
         *  @return Count of addresses info with connect state flags
         *
         **/
        uint32_t Count(const uint8_t nFlags = CONNECT_FLAGS_ALL);


        /** AddAddress
         *
         *  Adds the address to the manager and sets the connect nState for that
         *  address
         *
         *  @param[in] addr The address to add
         *  @param[in] nState The nState of the connection for the address
         *  @param[in] nSession The session length for address to give it higher score.
         *
         **/
        void AddAddress(const BaseAddress &addr, const uint8_t nState = ConnectState::NEW, const uint32_t nSession = 0);


        /** AddAddresses
         *
         *  Adds the addresses to the manager and sets the connect nState for that
         *  address
         *
         *  @param[in] addrs The addresses to add (as a vector of BaseAddress).
         *  @param[in] nState The nState of the connection for the address
         *
         **/
        void AddAddresses(const std::vector<BaseAddress> &addrs, const uint8_t nState = ConnectState::NEW);


        /** AddAddresses
         *
         *  Adds the addresses to the manager and sets the connect nState for that
         *  address
         *
         *  @param[in] addrs The addresses to add (as a vector of strings).
         *  @param[in] nState The nState of the connection for the address
         *
         **/
        void AddAddresses(const std::vector<std::string> &addrs, const uint8_t nState = ConnectState::NEW);


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
         *  @param[in] fTestnet Flag for if fTestnet seed nodes should be added.
         *
         **/
        void AddSeedAddresses(bool fTestnet = false);


        /** Has
         *
         *  Determines if the address manager has the address or not.
         *
         *  @param[in] addr The address to find.
         *
         **/
        bool Has(const BaseAddress &addr) const;


        /** Get
         *
         *  Gets a TrustAddress from the BaseAddress
         *
         *  @param[in] addr The address to find.
         *
         **/
        const LLP::TrustAddress& Get(const BaseAddress &addr);


        /** GetState
         *
         *  Gets the Connect State of the address in the manager if it exists.
         *
         *  @param[in] addr The address to get the state from.
         *
         *  @return Returns the connect nState of the address.
         *
         **/
        uint8_t GetState(const BaseAddress &addr) const;


        /** SetLatency
         *
         *  Finds the trust address and sets it's updated latency.
         *
         *  @param[in] lat The latency, in milliseconds
         *  @param[in] addr The address in reference to
         *
         **/
        void SetLatency(uint32_t lat, const BaseAddress &addr);


        /** SetHeight
         *
         *  Finds the trust address and sets it's updated block height.
         *
         *  @param[in] lat The latency, in milliseconds
         *  @param[in] addr The address in reference to
         *
         **/
        void SetHeight(uint32_t height, const BaseAddress &addr);


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


        /** ToString
         *
         *  Print the current nState of the address manager.
         *
         **/
        std::string ToString();


        /** SetPort
         *
         *  Set the port number for all addresses in the manager.
         *
         **/
        void SetPort(uint16_t nPortIn);


        /** Ban
         *
         *  Blacklists the given address so it won't get selected. The default
         *  behavior is to ban indefinitely.
         *
         *  @param[in] addr The address to ban.
         *  @param[in] nBanTime The time to ban for(or zero for indefinite ban)
         *
         **/
         void Ban(const BaseAddress &addr, uint32_t nBanTime = 0);


         /** GetDNSName
          *
          *  Gets the DNS name associated with the given address.
          *
          *  @param[in] addr The address to determine DNS from.
          *  @param[out] strDNS The DNS name, if any.
          *
          *  @return Returns true if the address has a DNS name.
          *
          **/
         bool GetDNSName(const BaseAddress &addr, std::string &strDNS);


    private:

        /** get_addresses
         *
         *  Helper function to get an array of info on the connected states specified
         *  by flags
         *
         *  @param[out] info The resulting outputted address info vector
         *
         *  @param[in] nFlags Specify which types of connections to get the info from.
         *
         **/
        void get_addresses(std::vector<TrustAddress> &vInfo, const uint8_t nFlags = CONNECT_FLAGS_ALL);


        /** count
         *
         *  Helper function to get the number of addresses of the connect type
         *
         *  @param[in] nFlags Specify which types of connections to get the info from.
         *
         **/
        uint32_t count(const uint8_t nFlags = CONNECT_FLAGS_ALL);


        /** total_count
         *
         *  Gets the cumulative count of each address state flags.
         *
         *  @param[in] nFlags Specify which types of connections to get the info from.
         *
         **/
        uint32_t total_count(const uint8_t nFlags);


        /** remove_address
         *
         *  Helper function that removes the given address from the map.
         *
         *  @param[in] addr The address to remove if it exists.
         *
         **/
         void remove_address(const BaseAddress &addr);


        /** is_banned
         *
         *  Helper function to determine if an address identified by it's hash
         *  is banned.
         *
         *  @param[in] hash The hash of the address to check for.
         *
         *  @return Returns true if address is banned, false otherwise.
         *
         **/
        bool is_banned(uint64_t hash);


        /** ban_count
         *
         *  Returns the total number of addresses currently banned.
         *
         **/
        uint32_t ban_count();


        /** eid_count
         *
         *  Returns the total number of LISP EID addresses.
         *
         **/
        uint32_t eid_count();


        /** update_state
         *
         *  Updates the nState of the given Trust address.
         *
         *  @param[out] pAddr The pointer to the trust address to update.
         *  @param[in] nState The nState the address should be updated to.
         *
         **/
        void update_state(TrustAddress *pAddr, uint8_t nState);


    private:

        /* The map of trust addresses to track. */
        std::map<uint64_t, TrustAddress> mapTrustAddress;

        /* The map of banned addresses to ignore. */
        std::map<uint64_t, uint32_t> mapBanned;

        /* The map of DNS related addresses. */
        std::map<uint64_t, std::string> mapDNS;

        /* The mutex used for thread locking. */
        mutable std::mutex MUTEX;

        /* The pointer to the address database. */
        LLD::AddressDB *pDatabase;

        /* The global port number for the manager (associated with server port). */
        uint16_t nPort;

    };
}

#endif
