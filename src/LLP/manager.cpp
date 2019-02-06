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
#include <Util/include/debug.h>
#include <Util/templates/serialize.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace LLP
{
    /* Default constructor */
    AddressManager::AddressManager(uint16_t port)
    : mapTrustAddress()
    , this_addr()
    {
        this_addr.SetPort(port);

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
        LOCK(mut);

        return mapTrustAddress.empty();
    }


    /*  Get a list of addresses in the manager that have the flagged state. */
    void AddressManager::GetAddresses(std::vector<BaseAddress> &vBaseAddr, const uint8_t flags)
    {
        std::vector<TrustAddress> vTrustAddr;

        /* Critical section: Get the flagged trust addresses. */
        {
            LOCK(mut);
            get_addresses(vTrustAddr, flags);
        }

        /*clear the passed in vector. */
        vBaseAddr.clear();

        /* build out base address vector */
        for(auto it = vTrustAddr.begin(); it != vTrustAddr.end(); ++it)
        {
            const BaseAddress &base_addr = *it;
            vBaseAddr.push_back(base_addr);
        }
    }


    /* Gets a list of trust addresses from the manager. */
    void AddressManager::GetAddresses(std::vector<TrustAddress> &vTrustAddr, const uint8_t flags)
    {
        LOCK(mut);
        get_addresses(vTrustAddr, flags);
    }


    /* Gets the trust address count that have the specified flags */
    uint32_t AddressManager::Count(const uint8_t flags)
    {
        LOCK(mut);
        return count(flags);
    }


    /*  Adds the address to the manager and sets it's connect state. */
    void AddressManager::AddAddress(const BaseAddress &addr,
                                    const uint8_t state)
    {
        /* Reject adding invalid addresses. */
        if(!addr.IsValid())
        {
            BaseAddress addrCopy = addr; //non-const copy for ToString
            debug::log(3, FUNCTION, "Invalid Address ", addrCopy.ToString());
            return;
        }

        uint64_t hash = addr.GetHash();
        uint64_t ms = runtime::unifiedtimestamp(true);

        LOCK(mut);

        if(mapTrustAddress.find(hash) == mapTrustAddress.end())
            mapTrustAddress[hash] = addr;

        TrustAddress &trust_addr = mapTrustAddress[hash];

        /* Set the port number to match this server */
        trust_addr.SetPort(this_addr.GetPort());

        switch(state)
        {
        /* New State */
        case ConnectState::NEW:
            break;

        /* Connected State */
        case ConnectState::CONNECTED:

            /* If the address is already connected, don't update */
            if(trust_addr.nState == ConnectState::CONNECTED)
                break;

            ++trust_addr.nConnected;
            trust_addr.nSession = 0;
            trust_addr.nFails = 0;
            trust_addr.nLastSeen = ms;
            trust_addr.nState = state;
            break;

        /* Dropped State */
        case ConnectState::DROPPED:

            /* If the address is already dropped, don't update */
            if(trust_addr.nState == ConnectState::DROPPED)
                break;

            ++trust_addr.nDropped;
            trust_addr.nSession = ms - trust_addr.nLastSeen;
            trust_addr.nLastSeen = ms;
            trust_addr.nState = state;
            break;

        /* Failed State */
        case ConnectState::FAILED:
            ++trust_addr.nFailed;
            ++trust_addr.nFails;
            trust_addr.nSession = 0;
            trust_addr.nState = state;
            break;

        /* Unrecognized state */
        default:
            debug::log(0, FUNCTION, "Unrecognized state");
            break;
        }
    }


    /*  Adds the addresses to the manager and sets their state. */
    void AddressManager::AddAddresses(const std::vector<BaseAddress> &addrs,
                                      const uint8_t state)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
            AddAddress(addrs[i], state);
    }


    /*  Adds the addresses to the manager and sets their state. */
    void AddressManager::AddAddresses(const std::vector<std::string> &addrs,
                                      const uint8_t state)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
        {
            /* Create a DNS lookup address to resolve to IP address. */
            BaseAddress lookup_address = BaseAddress(addrs[i], this_addr.GetPort(), true);

            AddAddress(lookup_address, state);
        }
    }


    /*  Removes an address from the manager if it exists. */
    void AddressManager::RemoveAddress(const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        std::unique_lock<std::mutex> lk(mut);

        auto it = mapTrustAddress.find(hash);

        /* Erase from the map if the address was found. */
        if(it != mapTrustAddress.end())
            mapTrustAddress.erase(hash);
    }


    /*  Adds the seed node addresses to the addressmanager if they aren't
     *  already in there. */
    void AddressManager::AddSeedAddresses(bool testnet)
    {
        std::vector<std::string> seeds;

        /* Add the testnet seed nodes if testnet flag is enabled. */
        if(testnet)
            seeds = DNS_SeedNodes_Testnet;
        else
            seeds = DNS_SeedNodes;

        AddAddresses(seeds);
        debug::log(3, seeds.size(), " seed nodes added");
    }


    /*  Determines if the address manager has the address or not. */
    bool AddressManager::Has(const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        std::unique_lock<std::mutex> lk(mut);

        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
            return true;

        return false;
    }


    /*  Finds the trust address and sets it's updated latency. */
    void AddressManager::SetLatency(uint32_t lat, const BaseAddress &addr)
    {
        uint64_t hash = addr.GetHash();
        std::unique_lock<std::mutex> lk(mut);

        auto it = mapTrustAddress.find(hash);
        if(it != mapTrustAddress.end())
            it->second.nLatency = lat;
    }


    /*  Select a good address to connect to that isn't already connected. */
    bool AddressManager::StochasticSelect(BaseAddress &addr)
    {
        std::vector<TrustAddress> vAddresses;
        uint64_t nTimestamp = runtime::unifiedtimestamp();
        uint64_t nRand = LLC::GetRand(nTimestamp);
        uint32_t nHash = LLC::SK32(BEGIN(nRand), END(nRand));
        uint32_t nSelect = 0;

        /* Put unconnected address info scores into a vector and sort them. */
        uint8_t flags = ConnectState::NEW    |
                        ConnectState::FAILED | ConnectState::DROPPED;

        /* Critical section: Get address info for the selected flags. */
        {
            LOCK(mut);
            get_addresses(vAddresses, flags);
        }

        uint32_t s = static_cast<uint32_t>(vAddresses.size());

        if(s == 0)
            return false;

        /* Select an index with a random weighted bias toward the from of the list. */
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
        LOCK(mut);
        return to_string();
    }


    /*  Set the port number for all addresses in the manager. */
    void AddressManager::SetPort(uint16_t port)
    {
        LOCK(mut);

        this_addr.SetPort(port);

        for(auto it = mapTrustAddress.begin(); it != mapTrustAddress.end(); ++it)
            it->second.SetPort(port);
    }


    /*  Returns the address for this node. */
    BaseAddress AddressManager::GetThisAddress() const
    {
        LOCK(mut);

        return this_addr;
    }


    /* Sets the address for this node. */
    void AddressManager::SetThisAddress(const BaseAddress &addr)
    {
        LOCK(mut);

        this_addr = addr;
    }


    /*  Read the address database into the manager. */
    void AddressManager::ReadDatabase()
    {
        { LOCK(mut);

            /* Make sure the map is empty. */
            mapTrustAddress.clear();

            /* Make sure the database exists. */
            if(!pDatabase)
            {
                debug::error(FUNCTION, "database null");
                return;
            }

            /* Get the database keys. */
            std::vector<std::vector<uint8_t> > keys = pDatabase->GetKeys();
            uint32_t s = static_cast<uint32_t>(keys.size());

            /* Load a trust address from each key. */
            for(uint32_t i = 0; i < s; ++i)
            {
                std::string str;
                uint64_t nKey;
                TrustAddress trust_addr;

                /* Create a datastream and deserialize the key/address pair. */
                DataStream ssKey(keys[i], SER_LLD, LLD::DATABASE_VERSION);
                ssKey >> str;

                /* Check for the info string. */
                if(str == "addr")
                {
                    /* Deserialize the key if it is an info type. */
                    ssKey >> nKey;

                    /* Read the trust address. */
                    pDatabase->ReadTrustAddress(nKey, trust_addr);

                    /* Get the hash and load it into the map. */
                    uint64_t nHash = trust_addr.GetHash();
                    mapTrustAddress[nHash] = trust_addr;
                }

                /* Check for the info string. */
                else if(str == "this")
                {
                    /* Deserialize the key if it is an info type. */
                    ssKey >> nKey;

                    /* Read the trust address. */
                    //pDatabase->ReadThisAddress(nKey, this_addr);
                }
            }
        }

        /* Check if the DNS needs update. */
        uint64_t nLastUpdate = 0;
        if(!pDatabase->ReadLastUpdate(nLastUpdate)
        || nLastUpdate + config::GetArg("-dnsupdate", 86400) <= runtime::unifiedtimestamp())
        {
            /* Log out that DNS is updating. */
            debug::log(0, "DNS cache is out of date by ",
                (runtime::unifiedtimestamp() - (nLastUpdate + config::GetArg("-dnsupdate", 86400))),
                " seconds... refreshing");

            /* Add the DNS seeds for this server. */
            runtime::timer timer;
            timer.Start();

            /* Update the Seed addresses. */
            AddSeedAddresses(config::fTestNet);

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
        LOCK(mut);

        /* Make sure the database exists. */
        if(!pDatabase)
        {
            debug::error(FUNCTION, "database null");
            return;
        }

        pDatabase->TxnBegin();

        /* Write the keys and addresses. */
        for(auto it = mapTrustAddress.begin(); it != mapTrustAddress.end(); ++it)
            pDatabase->WriteTrustAddress(it->first, it->second);

        /* Write the address of the server node */
        pDatabase->WriteThisAddress(0, this_addr);
        pDatabase->TxnCommit();
    }


    /*  Gets an array of trust addresses specified by the state flags. */
    void AddressManager::get_addresses(std::vector<TrustAddress> &vInfo, const uint8_t flags)
    {
        vInfo.clear();
        for(auto it = mapTrustAddress.begin(); it != mapTrustAddress.end(); ++it)
        {
            if(it->second.nState & flags)
                vInfo.push_back(it->second);
        }
    }

    /*  Gets the number of addresses specified by the state flags. */
    uint32_t AddressManager::count(const uint8_t flags)
    {
        uint32_t c = 0;
        for(auto it = mapTrustAddress.begin(); it != mapTrustAddress.end(); ++it)
        {
            if(it->second.nState & flags)
                ++c;
        }
        return c;
    }

    /* Gets the cumulative count of each address state flags. */
    uint32_t AddressManager::total_count(const uint8_t flags)
    {
        uint32_t total = 0;

        for(auto it = mapTrustAddress.begin(); it != mapTrustAddress.end(); ++it)
        {
            if(flags & ConnectState::CONNECTED)
                total += it->second.nConnected;
            if(flags & ConnectState::DROPPED)
                total += it->second.nDropped;
            if(flags & ConnectState::FAILED)
                total += it->second.nFailed;
        }

        return total;
    }


    /* Print the current state of the address manager. */
    std::string AddressManager::to_string()
    {
        return debug::safe_printstr(
             "C=", count(ConnectState::CONNECTED),
            " D=", count(ConnectState::DROPPED),
            " F=", count(ConnectState::FAILED),   " |",
            " TC=", total_count(ConnectState::CONNECTED),
            " TD=", total_count(ConnectState::DROPPED),
            " TF=", total_count(ConnectState::FAILED), " |",
            " size=", mapTrustAddress.size());
    }
}
