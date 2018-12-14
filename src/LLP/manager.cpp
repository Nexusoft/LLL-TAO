/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/manager.h>
#include <LLD/include/address.h>
#include <LLD/include/version.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>
#include <Util/include/debug.h>
#include <Util/templates/serialize.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace LLP
{

    /* Default constructor */
    AddressManager::AddressManager(uint16_t nPort)
    : pDatabase(new LLD::AddressDB(nPort, "r+"))
    , mapAddrInfo()
    {
        if(!pDatabase)
            debug::error(FUNCTION "Failed to allocate memory for AddressManager", __PRETTY_FUNCTION__);
    }


    /* Default destructor */
    AddressManager::~AddressManager()
    {
        if(pDatabase)
        {
            delete pDatabase;
            pDatabase = 0;
        }
    }


    /*  Gets a list of addresses in the manager */
    std::vector<Address> AddressManager::GetAddresses(const uint8_t flags)
    {
        std::vector<AddressInfo> vAddrInfo;
        std::vector<Address> vAddr;

        std::unique_lock<std::mutex> lk(mut);

        vAddrInfo = get_info(flags);

        for(auto it = vAddrInfo.begin(); it != vAddrInfo.end(); ++it)
            vAddr.push_back(static_cast<Address>(*it));

        return vAddr;
    }


    /*  Gets a list of address info in the manager */
    std::vector<AddressInfo> AddressManager::GetInfo(const uint8_t flags)
    {
        std::unique_lock<std::mutex> lk(mut);
        return get_info(flags);
    }


    /*  Adds the address to the manager and sets the connect state for that address */
    void AddressManager::AddAddress(const Address &addr, const uint8_t state)
    {
        uint64_t hash = addr.GetHash();

        std::unique_lock<std::mutex> lk(mut);

        if(mapAddrInfo.find(hash) == mapAddrInfo.end())
            mapAddrInfo[hash] = AddressInfo(addr);

        AddressInfo *pInfo = &mapAddrInfo[hash];
        int64_t ms = UnifiedTimestamp(true);

        switch(state)
        {
        case ConnectState::NEW:
            break;

        case ConnectState::FAILED:
            ++(pInfo->nFailed);
            ++(pInfo->nFails);
            pInfo->nSession = 0;
            pInfo->nState = state;
            break;

        case ConnectState::DROPPED:
            if(pInfo->nState == ConnectState::DROPPED)
                break;

            ++(pInfo->nDropped);
            pInfo->nSession = ms - pInfo->nLastSeen;
            pInfo->nLastSeen = ms;
            pInfo->nState = state;
            break;

        case ConnectState::CONNECTED:
            if(pInfo->nState == ConnectState::CONNECTED)
                break;

            pInfo->nFails = 0;
            ++(pInfo->nConnected);
            pInfo->nSession = 0;
            pInfo->nLastSeen = ms;
            pInfo->nState = state;
            break;

        default:
            break;
        }

        debug::log(5, FUNCTION "%s:%u ", __PRETTY_FUNCTION__,
            addr.ToString().c_str(), addr.GetPort());

        print();

        /* update the LLD Database with a new entry */
        pDatabase->WriteAddressInfo(hash, *pInfo);
    }


    /*  Adds the addresses to the manager and sets the connect state for that address */
    void AddressManager::AddAddresses(const std::vector<Address> &addrs, const uint8_t state)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
            AddAddress(addrs[i], state);
    }


    /*  Finds the managed address info and sets the latency experienced by
     *  that address. */
    void AddressManager::SetLatency(uint32_t lat, const Address &addr)
    {
        uint64_t hash = addr.GetHash();
        std::unique_lock<std::mutex> lk(mut);

        auto it = mapAddrInfo.find(hash);
        if(it != mapAddrInfo.end())
            it->second.nLatency = lat;
    }


    /*  Select a good address to connect to that isn't already connected. */
    bool AddressManager::StochasticSelect(Address &addr)
    {
        std::unique_lock<std::mutex> lk(mut);

        /*put unconnected address info scores into a vector and sort */
        uint8_t flags = ConnectState::NEW | ConnectState::FAILED | ConnectState::DROPPED;

        std::vector<AddressInfo> vInfo = get_info(flags);

        if(!vInfo.size())
            return false;

        std::sort(vInfo.begin(), vInfo.end());

        std::reverse(vInfo.begin(), vInfo.end());

        uint256_t nRand = LLC::GetRand256();
        uint32_t nHash = LLC::SK32(BEGIN(nRand), END(nRand));

        /*select an index with a good random weight bias toward the front of the list */
        uint32_t nSelect = ((std::numeric_limits<uint64_t>::max() /
            std::max((uint64_t)std::pow(nHash, 1.95) + 1, (uint64_t)1)) - 3) % vInfo.size();

        addr = vInfo[nSelect];

        return true;
    }


    /*  Read the address database into the manager */
    void AddressManager::ReadDatabase()
    {
        std::unique_lock<std::mutex> lk(mut);

        std::vector<std::vector<uint8_t> > keys = pDatabase->GetKeys();

        uint32_t s = static_cast<uint32_t>(keys.size());


        for(uint32_t i = 0; i < s; ++i)
        {
            std::string str;
            uint64_t nKey;
            AddressInfo addr_info;

            //printf("%s", HexStr(keys[i].begin(), keys[i].end()).c_str());

            DataStream ssKey(keys[i], SER_LLD, LLD::DATABASE_VERSION);
            ssKey >> str;
            ssKey >> nKey;

            //debug::log(5, "reading %s %" PRIu64 " ", str.c_str(), nKey);

            pDatabase->ReadAddressInfo(nKey, addr_info);

            uint64_t nHash = addr_info.GetHash();

            mapAddrInfo[nHash] = addr_info;
        }

        printf("keys.size() = %u", s);

        print();
    }


    /*  Write the addresses from the manager into the address database */
    void AddressManager::WriteDatabase()
    {
        std::unique_lock<std::mutex> lk(mut);

        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
            pDatabase->WriteAddressInfo(it->first, it->second);


        printf("keys.size() = %u", (uint32_t)pDatabase->GetKeys().size());

        print();
    }


    /*  Helper function to get an array of info on the connected states specified
     *  by flags */
    std::vector<AddressInfo> AddressManager::get_info(const uint8_t flags) const
    {
        std::vector<AddressInfo> addrs;
        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
        {
            if(it->second.nState & flags)
                addrs.push_back(it->second);
        }

        return addrs;
    }

    /*  Helper function to get the number of addresses of the connect type */
    uint32_t AddressManager::get_current_count(const uint8_t flags) const
    {
        return static_cast<uint32_t>(get_info(flags).size());
    }

    uint32_t AddressManager::get_total_count(const uint8_t flags) const
    {
        std::vector<AddressInfo> vInfo = get_info();
        uint32_t total = 0;
        uint32_t s = static_cast<uint32_t>(vInfo.size());

        for(uint32_t i = 0; i < s; ++i)
        {
            AddressInfo &addr = vInfo[i];

            if(flags & ConnectState::CONNECTED)
                total += addr.nConnected;
            if(flags & ConnectState::DROPPED)
                total += addr.nDropped;
            if(flags & ConnectState::FAILED)
                total += addr.nFailed;
        }
        return total;
    }

    void AddressManager::print() const
    {
        debug::log(3, "C=%u D=%u F=%u | TC=%u TD=%u TF=%u | size=%lu",
         get_current_count(ConnectState::CONNECTED),
         get_current_count(ConnectState::DROPPED),
         get_current_count(ConnectState::FAILED),
         get_total_count(ConnectState::CONNECTED),
         get_total_count(ConnectState::DROPPED),
         get_total_count(ConnectState::FAILED),
         mapAddrInfo.size());
    }

}
