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

#include <LLD/types/address.h>
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
    AddressManager::AddressManager(uint16_t nPortIn)
    : mapTrustAddress()
    , mapBanned()
    , mapDNS()
    , MUTEX()
    , pDatabase(nullptr)
    , nPort(nPortIn)
    {
        pDatabase = new LLD::AddressDB(nPortIn, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        if(!pDatabase)
            throw debug::exception(FUNCTION, "Failed to allocate memory for AddressManager");
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


    /*  Get a list of addresses in the manager that have the flagged nState. */
    void AddressManager::GetAddresses(std::vector<BaseAddress> &vBaseAddr, const uint8_t nFlags)
    {
        std::vector<TrustAddress> vTrustAddr;

        /* Critical section: Get the flagged trust addresses. */
        {
            LOCK(MUTEX);
            get_addresses(vTrustAddr, nFlags);
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
    void AddressManager::GetAddresses(std::vector<TrustAddress> &vTrustAddr, const uint8_t nFlags)
    {
        LOCK(MUTEX);
        get_addresses(vTrustAddr, nFlags);
    }


    /* Gets the trust address count that have the specified nFlags */
    uint32_t AddressManager::Count(const uint8_t nFlags)
    {
        LOCK(MUTEX);
        return count(nFlags);
    }


    /*  Adds the address to the manager and sets it's connect nState. */
    void AddressManager::AddAddress(const BaseAddress &addr, const uint8_t nState, const uint32_t nSession)
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

        /* Add address to map if not already added. */
        if(mapTrustAddress.find(hash) == mapTrustAddress.end())
            mapTrustAddress[hash] = addr;

        /* Set the port number to match this server */
        TrustAddress& trust_addr = mapTrustAddress[hash];
        trust_addr.SetPort(nPort);
        trust_addr.nSession = nSession;

        /* Update the stats for this address based on the nState. */
        update_state(&trust_addr, nState);

        /* Update the LLD Address database for this entry */
        pDatabase->WriteTrustAddress(hash, trust_addr);
    }


    /*  Adds the addresses to the manager and sets their nState. */
    void AddressManager::AddAddresses(const std::vector<BaseAddress> &addrs,
                                      const uint8_t nState)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
        {
            /* Don't add addresses that already exist in the address manager, as this could reset the nState of existing connections */
            if(!Has(addrs[i]))
                AddAddress(addrs[i], nState);
        }
    }


    /*  Adds the addresses to the manager and sets their nState. */
    void AddressManager::AddAddresses(const std::vector<std::string> &addrs,
                                      const uint8_t nState)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
        {
            /* Create a DNS lookup address to resolve to IP address. */
            BaseAddress lookup_address = BaseAddress(addrs[i], nPort, true);

            AddAddress(lookup_address, nState, 120);

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
    void AddressManager::AddSeedAddresses(bool fTestnet)
    {
        /* Add the fTestnet seed nodes if fTestnet flag is enabled. */
        if(fTestnet)
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

    /* Gets a TrustAddress from the BaseAddress */
    const LLP::TrustAddress& AddressManager::Get(const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        return mapTrustAddress[hash];

    }


    /*  Gets the Connect State of the address in the manager if it exists. */
    uint8_t AddressManager::GetState(const BaseAddress &addr) const
    {
        uint64_t hash = addr.GetHash();
        uint8_t nState = static_cast<uint8_t>(ConnectState::NEW);
        LOCK(MUTEX);
        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
            nState = it->second.nState;

        return nState;
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
            pDatabase->WriteTrustAddress(hash, it->second);
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
            pDatabase->WriteTrustAddress(hash, it->second);
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
        uint8_t nFlags = ConnectState::NEW    |
                        ConnectState::FAILED | ConnectState::DROPPED;

        /* Critical section: Get address info for the selected nFlags. */
        {
            LOCK(MUTEX);
            get_addresses(vAddresses, nFlags);
        }

        uint64_t nSize = vAddresses.size();

        if(nSize == 0)
            return false;

        /* Select an index with a random weighted bias toward the front of the list. */
        nSelect = ((std::numeric_limits<uint64_t>::max() /
            std::max((uint64_t)std::pow(nHash, 1.95) + 1, (uint64_t)1)) - 3) % nSize;

        if(nSelect >= nSize)
          return debug::error(FUNCTION, "index out of bounds");

        /* sort info vector and assign the selected address */
        std::sort(vAddresses.begin(), vAddresses.end());
        std::reverse(vAddresses.begin(), vAddresses.end());


        addr.SetIP(vAddresses[nSelect]);
        addr.SetPort(vAddresses[nSelect].GetPort());

        return true;
    }


    /* Print the current nState of the address manager. */
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
    void AddressManager::SetPort(uint16_t nPortIn)
    {
        LOCK(MUTEX);

        nPort = nPortIn;

        for(auto &addr : mapTrustAddress)
            addr.second.SetPort(nPort);
    }


    /*  Blacklists the given address so it won't get selected. The default
     *  behavior is to ban indefinitely.*/
    void AddressManager::Ban(const BaseAddress &addr, uint32_t nBanTime)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        /* Store the hash in the map for banned addresses */
        mapBanned[hash] = nBanTime;

        /* Remove the address from the map of addresses */
        remove_address(addr);
    }

    bool AddressManager::GetDNSName(const BaseAddress &addr, std::string &strDNS)
    {
        uint64_t hash = addr.GetHash();
        LOCK(MUTEX);

        /* Attempt to find the DNS string. */
        auto it = mapDNS.find(hash);

        /* Return if name not found. */
        if(it == mapDNS.end())
            return false;

        /* Set the dns lookup name. */
        strDNS = it->second;
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
            if(pDatabase->BatchRead("addr", vAddr, 1000))
            {
                /* Get the last element. */
                uint64_t hashLast = 0;
                while(!config::fShutdown)
                {
                    /* Loop through items read. */
                    for(const auto& addr : vAddr)
                    {
                        /* Get the hash and load it into the map. */
                        uint64_t hash = addr.GetHash();
                        mapTrustAddress[hash] = addr;

                        hashLast = hash;
                    }

                    /* Read another batch. */
                    if(vAddr.size() < 1000
                    || !pDatabase->BatchRead(std::make_pair(std::string("addr"), hashLast), "addr", vAddr, 1000))
                        break;
                }
            }

            debug::log(0, FUNCTION, "Loaded ", mapTrustAddress.size(), " Addresses");
        }

        /* Check if the DNS needs update. */
        uint64_t nLastUpdate = 0;
        if(!config::GetBoolArg("-nodns")
            && (!pDatabase->ReadLastUpdate(nLastUpdate) ||
                nLastUpdate + config::GetArg("-dnsupdate", 86400) <= runtime::timestamp()))
        {
            /* Log out that DNS is updating. */
            debug::log(0, "DNS cache is out of date by ",
                (runtime::timestamp() - (nLastUpdate + config::GetArg("-dnsupdate", 86400))),
                " seconds for port ", nPort, "... refreshing");

            /* Add the DNS seeds for this server. */
            runtime::timer timer;
            timer.Start();

            /* Update the Seed addresses. */
            AddSeedAddresses(config::fTestNet.load());

            /* Write that DNS was updated. */
            if(!pDatabase->WriteLastUpdate())
                debug::error(FUNCTION, "failed to update DNS timer");

            /* Log the time it took to resolve DNS items. */
            debug::log(0, "DNS cache updated in ",
                timer.ElapsedMilliseconds(), " ms");
        }
    }


    /*  Gets an array of trust addresses specified by the nState nFlags. */
    void AddressManager::get_addresses(std::vector<TrustAddress> &vInfo, const uint8_t nFlags)
    {
        vInfo.clear();
        for(const auto &addr : mapTrustAddress)
        {
            /* If the address is on the ban list, skip it. */
            if(mapBanned.find(addr.first) != mapBanned.end())
                continue;

            /* If the address matches the flag, add it to the vector. */
            if(addr.second.nState & nFlags)
                vInfo.push_back(addr.second);
        }
    }


    /*  Gets the number of addresses specified by the nState nFlags. */
    uint32_t AddressManager::count(const uint8_t nFlags)
    {
        uint32_t c = 0;
        for(const auto &it : mapTrustAddress)
        {
            /* If the address is on the ban list, skip it. */
            if(mapBanned.find(it.first) != mapBanned.end())
                continue;

            /* If the address matches the flag, increment the count */
            if(it.second.nState & nFlags)
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


    /* Gets the cumulative count of each address nState nFlags. */
    uint32_t AddressManager::total_count(const uint8_t nFlags)
    {
        uint32_t nTotal = 0;
        for(const auto& addr : mapTrustAddress)
        {
            /* If the address is on the ban list, skip it. */
            if(mapBanned.find(addr.first) != mapBanned.end())
                continue;

            /* Sum up the total stats of each category */
            if(nFlags & ConnectState::CONNECTED)
                nTotal += addr.second.nConnected;
            if(nFlags & ConnectState::DROPPED)
                nTotal += addr.second.nDropped;
            if(nFlags & ConnectState::FAILED)
                nTotal += addr.second.nFailed;
        }

        return nTotal;
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


    /*  Updates the nState of the given Trust address. */
    void AddressManager::update_state(TrustAddress *pAddr, uint8_t nState)
    {
        uint64_t nTimestamp = runtime::unifiedtimestamp(true);

        switch(nState)
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
              pAddr->nLastSeen = nTimestamp;
              pAddr->nState = nState;
              break;

          /* Dropped State */
          case ConnectState::DROPPED:

              /* If the address is already dropped, don't update */
              if(pAddr->nState == ConnectState::DROPPED)
                  break;

              ++pAddr->nDropped;
              pAddr->nSession = nTimestamp - pAddr->nLastSeen;
              pAddr->nLastSeen = nTimestamp;
              pAddr->nState = nState;
              break;

          /* Failed State */
          case ConnectState::FAILED:
              ++pAddr->nFailed;
              ++pAddr->nFails;
              pAddr->nSession = 0;
              pAddr->nState = nState;
              break;

          /* Unrecognized nState */
          default:
              debug::log(0, FUNCTION, "Unrecognized nState");
              break;
        }
    }
}
