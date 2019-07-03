/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/macro.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>
#include <LLD/include/address.h>
#include <LLD/include/version.h>
#include <LLP/include/manager.h>
#include <LLP/include/hosts.h>
#include <LLP/include/seeds.h>
#include <Util/include/debug.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace LLP
{
    /* Default constructor */
    AddressManager::AddressManager(uint16_t port)
    : mapTrustAddress()
    , mapBanned()
    , mapDNS()
    , MUTEX()
    , pDatabase(nullptr)
    , nPort(port)
    {
        pDatabase = new LLD::AddressDB(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);

        if(pDatabase)
            return;

        /* This should never occur. */
        debug::error(FUNCTION, "Failed to allocate memory for AddressManager");
    }


    /* Default destructor */
    AddressManager::~AddressManager()
    {
        /* Delete the database pointer if it exists. */
        if(pDatabase)
        {
            delete pDatabase;
            pDatabase = 0;
        }
    }

    /*  Determine if the address manager has any addresses in it. */
    bool AddressManager::IsEmpty() const
    {
        LOCK(MUTEX);

        return mapTrustAddress.empty();
    }


    /*  Get a list of addresses in the manager that have the flagged state. */
    void AddressManager::GetAddresses(std::vector<BaseAddress> &vBaseAddr, const uint8_t flags)
    {
        std::vector<TrustAddress> vTrustAddr;

        /* Critical section: Get the flagged trust addresses. */
        {
            LOCK(MUTEX);
            get_addresses(vTrustAddr, flags);
        }

        /*clear the passed in vector. */
        vBaseAddr.clear();

        /* build out base address vector */
        for(const auto &trust_addr : vTrustAddr)
        {
            BaseAddress base_addr(trust_addr);
            vBaseAddr.push_back(base_addr);
        }
    }


    /* Gets a list of trust addresses from the manager. */
    void AddressManager::GetAddresses(std::vector<TrustAddress> &vTrustAddr, const uint8_t flags)
    {
        LOCK(MUTEX);
        get_addresses(vTrustAddr, flags);
    }


    /* Gets the trust address count that have the specified flags */
    uint32_t AddressManager::Count(const uint8_t flags)
    {
        LOCK(MUTEX);
        return count(flags);
    }


    /*  Adds the address to the manager and sets it's connect state. */
    void AddressManager::AddAddress(const BaseAddress &addr, const uint8_t state)
    {
        /* Reject adding invalid addresses. */
        if(!addr.IsValid())
        {
            debug::log(3, FUNCTION, "Invalid Address ", addr.ToString());
            return;
        }

        uint64_t hash = addr.GetHash();

        LOCK(MUTEX);

        /* Reject banned addresses. */
        if(is_banned(hash))
            return;

        if(mapTrustAddress.find(hash) == mapTrustAddress.end())
            mapTrustAddress[hash] = addr;

        TrustAddress &trust_addr = mapTrustAddress[hash];

        /* Set the port number to match this server */
        trust_addr.SetPort(nPort);

        /* Update the stats for this address based on the state. */
        update_state(&trust_addr, state);

        /* Update the LLD Address database for this entry */
        //pDatabase->TxnBegin();
        pDatabase->WriteTrustAddress(hash, trust_addr);
        //pDatabase->TxnCommit();
    }


    /*  Adds the addresses to the manager and sets their state. */
    void AddressManager::AddAddresses(const std::vector<BaseAddress> &addrs,
                                      const uint8_t state)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
        {
            /* Don't add addresses that already exist in the address manager, as this could reset the state of existing connections */
            if(!Has(addrs[i]))
                AddAddress(addrs[i], state);
        }
    }


    /*  Adds the addresses to the manager and sets their state. */
    void AddressManager::AddAddresses(const std::vector<std::string> &addrs,
                                      const uint8_t state)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
        {
            /* Create a DNS lookup address to resolve to IP address. */
            BaseAddress lookup_address = BaseAddress(addrs[i], nPort, true);

            AddAddress(lookup_address, state);

            LOCK(MUTEX);

            /* Associate a DNS name with an address. */
            mapDNS[lookup_address.GetHash()] = addrs[i];
        }
    }


    /*  Removes an address from the manager if it exists. */
    void AddressManager::RemoveAddress(const BaseAddress &addr)
    {
        /* Ensure thread safety while removing address. */
        LOCK(MUTEX);
        remove_address(addr);
    }


    /*  Adds the seed node addresses to the addressmanager if they aren't
     *  already in there. */
    void AddressManager::AddSeedAddresses(bool testnet)
    {
        /* Add the testnet seed nodes if testnet flag is enabled. */
        if(testnet)
            AddAddresses(DNS_SeedNodes_Testnet);
        else
            AddAddresses(DNS_SeedNodes);
    }


    /*  Determines if the address manager has the address or not. */
    bool AddressManager::Has(const BaseAddress &addr) const
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
            return true;

        return false;
    }


    /*  Gets the Connect State of the address in the manager if it exists. */
    uint8_t AddressManager::GetState(const BaseAddress &addr) const
    {
        uint64_t hash = addr.GetHash();
        uint8_t state = static_cast<uint8_t>(ConnectState::NEW);
        LOCK(MUTEX);
        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
            state = it->second.nState;

        return state;
    }


    /*  Finds the trust address and sets it's updated latency. */
    void AddressManager::SetLatency(uint32_t lat, const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
        {
            it->second.nLatency = lat;

            /* Update the LLD Address database for this entry */
            //pDatabase->TxnBegin();
            pDatabase->WriteTrustAddress(hash, it->second);
            //pDatabase->TxnCommit();
        }
    }


    /*  Finds the trust address and sets it's updated block height. */
    void AddressManager::SetHeight(uint32_t height, const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
        {
            it->second.nHeight = height;

            /* Update the LLD Address database for this entry */
            //pDatabase->TxnBegin();
            pDatabase->WriteTrustAddress(hash, it->second);
            //pDatabase->TxnCommit();
        }

    }


    /*  Select a good address to connect to that isn't already connected. */
    bool AddressManager::StochasticSelect(BaseAddress &addr)
    {
        std::vector<TrustAddress> vAddresses;
        uint64_t nSelect = 0;
        uint64_t nTimestamp = runtime::unifiedtimestamp();
        uint64_t nRand = LLC::GetRand(nTimestamp);
        uint32_t nHash = LLC::SK32(BEGIN(nRand), END(nRand));


        /* Put unconnected address info scores into a vector and sort them. */
        uint8_t flags = ConnectState::NEW    |
                        ConnectState::FAILED | ConnectState::DROPPED;

        /* Critical section: Get address info for the selected flags. */
        {
            LOCK(MUTEX);
            get_addresses(vAddresses, flags);
        }

        uint64_t s = vAddresses.size();

        if(s == 0)
            return false;

        /* Select an index with a random weighted bias toward the front of the list. */
        nSelect = ((std::numeric_limits<uint64_t>::max() /
            std::max((uint64_t)std::pow(nHash, 1.95) + 1, (uint64_t)1)) - 3) % s;

        if(nSelect >= s)
          return debug::error(FUNCTION, "index out of bounds");

        /* sort info vector and assign the selected address */
        std::sort(vAddresses.begin(), vAddresses.end());
        std::reverse(vAddresses.begin(), vAddresses.end());


        addr.SetIP(vAddresses[nSelect]);
        addr.SetPort(vAddresses[nSelect].GetPort());

        return true;
    }


    /* Print the current state of the address manager. */
    std::string AddressManager::ToString()
    {
        LOCK(MUTEX);
        std::string strRet = debug::safe_printstr(
             "C=", count(ConnectState::CONNECTED),
            " D=", count(ConnectState::DROPPED),
            " F=", count(ConnectState::FAILED),   " |",
            " TC=", total_count(ConnectState::CONNECTED),
            " TD=", total_count(ConnectState::DROPPED),
            " TF=", total_count(ConnectState::FAILED), " |",
            " B=",  ban_count(), " |",
            " EID=", eid_count(), " |",
            " size=", mapTrustAddress.size());

        return strRet;
    }


    /*  Set the port number for all addresses in the manager. */
    void AddressManager::SetPort(uint16_t port)
    {
        LOCK(MUTEX);

        nPort = port;

        for(auto &addr : mapTrustAddress)
            addr.second.SetPort(port);
    }


    /*  Blacklists the given address so it won't get selected. The default
     *  behavior is to ban indefinitely.*/
    void AddressManager::Ban(const BaseAddress &addr, uint32_t banTime)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        /* Store the hash in the map for banned addresses */
        mapBanned[hash] = banTime;

        /* Remove the address from the map of addresses */
        remove_address(addr);
    }

    bool AddressManager::GetDNSName(const BaseAddress &addr, std::string &dns)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        /* Attempt to find the DNS string. */
        auto it = mapDNS.find(hash);

        /* Return if name not found. */
        if(it == mapDNS.end())
            return false;

        /* Set the dns lookup name. */
        dns = it->second;
        return true;
    }


    /*  Read the address database into the manager. */
    void AddressManager::ReadDatabase()
    {
        {
            LOCK(MUTEX);

            /* Make sure the map is empty. */
            mapTrustAddress.clear();
            mapBanned.clear();

            /* Make sure the database exists. */
            if(!pDatabase)
            {
                debug::error(FUNCTION, "database null");
                return;
            }

            /* Do a sequential read. */
            std::vector<TrustAddress> vAddr;
            if(pDatabase->BatchRead("addr", vAddr, -1))
            {
                /* Loop through items read. */
                for(const auto& addr : vAddr)
                {
                    /* Get the hash and load it into the map. */
                    uint64_t hash = addr.GetHash();
                    mapTrustAddress[hash] = addr;
                }
            }
        }

        /* Check if the DNS needs update. */
        uint64_t nLastUpdate = 0;
        if(!config::GetBoolArg("-nodns")
           && (!pDatabase->ReadLastUpdate(nLastUpdate) ||
                nLastUpdate + config::GetArg("-dnsupdate", 86400) <= runtime::unifiedtimestamp()))
        {
            /* Log out that DNS is updating. */
            debug::log(0, "DNS cache is out of date by ",
                (runtime::unifiedtimestamp() - (nLastUpdate + config::GetArg("-dnsupdate", 86400))),
                " seconds... refreshing");

            /* Add the DNS seeds for this server. */
            runtime::timer timer;
            timer.Start();

            /* Update the Seed addresses. */
            AddSeedAddresses(config::fTestNet.load());

            /* Write that DNS was updated. */
            pDatabase->WriteLastUpdate();

            /* Log the time it took to resolve DNS items. */
            debug::log(0, "DNS cache updated in ",
                timer.ElapsedMilliseconds(), " ms");
        }
    }


    /*  Write the addresses from the manager into the address database. */
    void AddressManager::WriteDatabase()
    {
        /* DEPRECATED */
        /* Make sure the database exists. */
        //if(!pDatabase)
        //{
        //    debug::error(FUNCTION, "database null");
        //    return;
        //}

        //LOCK(MUTEX);

        //if(!mapTrustAddress.size())
        //  return;

        //pDatabase->TxnBegin();

        /* Write the keys and addresses. */
        //for(auto it = mapTrustAddress.begin(); it != mapTrustAddress.end(); ++it)
        //    pDatabase->WriteTrustAddress(it->first, it->second);

        //pDatabase->TxnCommit();
    }


    /*  Gets an array of trust addresses specified by the state flags. */
    void AddressManager::get_addresses(std::vector<TrustAddress> &vInfo, const uint8_t flags)
    {
        vInfo.clear();
        for(const auto &addr : mapTrustAddress)
        {
            /* If the address is on the ban list, skip it. */
            if(mapBanned.find(addr.first) != mapBanned.end())
                continue;

            /* If the address matches the flag, add it to the vector. */
            if(addr.second.nState & flags)
                vInfo.push_back(addr.second);
        }
    }


    /*  Gets the number of addresses specified by the state flags. */
    uint32_t AddressManager::count(const uint8_t flags)
    {
        uint32_t c = 0;
        for(const auto &it : mapTrustAddress)
        {
            /* If the address is on the ban list, skip it. */
            if(mapBanned.find(it.first) != mapBanned.end())
                continue;

            /* If the address matches the flag, increment the count */
            if(it.second.nState & flags)
                ++c;
        }
        return c;
    }


    /*  Helper function that removes the given address from the map. */
    void AddressManager::remove_address(const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        auto it = mapTrustAddress.find(hash);

        /* Erase from the map if the address was found. */
        if(it != mapTrustAddress.end())
            mapTrustAddress.erase(hash);
    }


    /* Gets the cumulative count of each address state flags. */
    uint32_t AddressManager::total_count(const uint8_t flags)
    {
        uint32_t total = 0;

        for(const auto& addr : mapTrustAddress)
        {
            /* If the address is on the ban list, skip it. */
            if(mapBanned.find(addr.first) != mapBanned.end())
                continue;

            /* Sum up the total stats of each category */
            if(flags & ConnectState::CONNECTED)
                total += addr.second.nConnected;
            if(flags & ConnectState::DROPPED)
                total += addr.second.nDropped;
            if(flags & ConnectState::FAILED)
                total += addr.second.nFailed;
        }

        return total;
    }


    /*  Helper function to determine if an address identified by it's hash
     *  is banned. */
    bool AddressManager::is_banned(uint64_t hash)
    {
        return mapBanned.find(hash) != mapBanned.end();
    }


    /*  Returns the total number of addresses currently banned. */
    uint32_t AddressManager::ban_count()
    {
        return static_cast<uint32_t>(mapBanned.size());
    }


    /*  Returns the total number of LISP EID addresses. */
    uint32_t AddressManager::eid_count()
    {
        uint32_t c = 0;
        auto it = mapTrustAddress.begin();
        for(; it != mapTrustAddress.end(); ++it)
        {
            if(it->second.IsEID())
              ++c;
        }
        return c;
    }


    /*  Updates the state of the given Trust address. */
    void AddressManager::update_state(TrustAddress *pAddr, uint8_t state)
    {
        uint64_t ms = runtime::unifiedtimestamp(true);

        switch(state)
        {
          /* New State */
          case ConnectState::NEW:
              break;

          /* Connected State */
          case ConnectState::CONNECTED:

              /* If the address is already connected, don't update */
              if(pAddr->nState == ConnectState::CONNECTED)
                  break;

              ++pAddr->nConnected;
              pAddr->nSession = 0;
              pAddr->nFails = 0;
              pAddr->nLastSeen = ms;
              pAddr->nState = state;
              break;

          /* Dropped State */
          case ConnectState::DROPPED:

              /* If the address is already dropped, don't update */
              if(pAddr->nState == ConnectState::DROPPED)
                  break;

              ++pAddr->nDropped;
              pAddr->nSession = ms - pAddr->nLastSeen;
              pAddr->nLastSeen = ms;
              pAddr->nState = state;
              break;

          /* Failed State */
          case ConnectState::FAILED:
              ++pAddr->nFailed;
              ++pAddr->nFails;
              pAddr->nSession = 0;
              pAddr->nState = state;
              break;

          /* Unrecognized state */
          default:
              debug::log(0, FUNCTION, "Unrecognized state");
              break;
        }
    }
}
