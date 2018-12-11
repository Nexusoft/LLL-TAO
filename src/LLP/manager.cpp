/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/manager.h>
#include <LLD/include/address.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>
#include <Util/include/debug.h>
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
            debug::error(FUNCTION "Failed to allocate memory for AddressManager\n", __PRETTY_FUNCTION__);

        ReadDatabase();
    }


    /* Default destructor */
    AddressManager::~AddressManager()
    {
        WriteDatabase();
    }


    /*  Gets a list of addresses in the manager */
    std::vector<Address> AddressManager::GetAddresses(const uint8_t flags)
    {
        std::vector<AddressInfo> vAddrInfo;
        std::vector<Address> vAddr;

        std::unique_lock<std::mutex> lk(mutex);

        vAddrInfo = get_info(flags);

        for(auto it = vAddrInfo.begin(); it != vAddrInfo.end(); ++it)
            vAddr.push_back(static_cast<Address>(it->second));

        return vAddr;
    }


    /*  Gets a list of address info in the manager */
    std::vector<AddressInfo> AddressManager::GetInfo(const uint8_t flags)
    {
        std::unique_lock<std::mutex> lk(mutex);
        return get_info(flags);
    }


    /*  Adds the address to the manager and sets the connect state for that address */
    void AddressManager::AddAddress(const Address &addr, const uint8_t state)
    {
        uint64_t hash = addr.GetHash();

        std::unique_lock<std::mutex> lk(mutex);

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

        debug::log(5, FUNCTION "%s - S=%lu, C=%u, D=%u, F=%u, L=%u, TC=%u, TD=%u, TF=%u, size=%u\n", __PRETTY_FUNCTION__, addr.ToString().c_str(),
         pInfo->nSession, pInfo->nConnected, pInfo->nDropped, pInfo->nFailed, pInfo->nLatency,
         get_count(ConnectState::CONNECTED), get_count(ConnectState::DROPPED), get_count(ConnectState::FAILED), get_count());
    }


    /*  Adds the address to the manager and sets the connect state for that address */
    void AddressManager::AddAddress(const std::vector<Address> &addrs, const uint8_t state)
    {
        debug::log(5, FUNCTION "AddressManager\n", __PRETTY_FUNCTION__);
        for(uint32_t i = 0; i < addrs.size(); ++i)
            AddAddress(addrs[i], state);
    }


    /*  Finds the managed address info and sets the latency experienced by
     *  that address. */
    void AddressManager::SetLatency(uint32_t lat, const Address &addr)
    {
        std::unique_lock<std::mutex> lk(mutex);

        auto it = mapAddrInfo.find(addr.GetHash());
        if(it != mapAddrInfo.end())
        {
            it->second.nLatency = lat;
            //debug::log(5, FUNCTION "%s - S=%lu, C=%u, D=%u, F=%u, L=%u\n", __PRETTY_FUNCTION__, addr.ToString().c_str(),
            // it->second.nSession, it->second.nConnected, it->second.nDropped, it->second.nFailed, it->second.nLatency);
        }
    }


    /*  Select a good address to connect to that isn't already connected. */
    bool AddressManager::StochasticSelect(Address &addr)
    {
        std::unique_lock<std::mutex> lk(mutex);

        //put unconnected address info scores into a vector and sort
        uint8_t flags = ConnectState::NEW | ConnectState::FAILED | ConnectState::DROPPED;

        std::vector<AddressInfo> vInfo = get_info(flags);

        if(!vInfo.size())
            return false;

        std::sort(vInfo.begin(), vInfo.end());

        uint256_t nRand = LLC::GetRand256();
        uint32_t nHash = LLC::SK32(BEGIN(nRand), END(nRand));

        /*select an index with a good random weight bias toward the front of the list */
        uint32_t nSelect = ((std::numeric_limits<uint64_t>::max() /
            std::max((uint64_t)std::pow(nHash, 1.912) + 1, (uint64_t)1)) - 7) % vInfo.size();

        //debug::log(5, FUNCTION "Selected: %u %u %u\n", __PRETTY_FUNCTION__, byte, size, i);

        addr = vInfo[nSelect];

        //debug::log(5, FUNCTION "Selected: %s\n", __PRETTY_FUNCTION__, addr.ToString().c_str());

        return true;
    }


    /*  Read the address database into the manager */
    void AddressManager::ReadDatabase()
    {
        std::vector<std::vector<uint8_t> > keys = pDatabase->GetKeys();

        uint32_t s = static_cast<uint32_t>(keys.size());
        for(uint32_t i = 0; i < s; ++i)
        {
            uint32_t nKey;
            CAddressInfo addr_info;

            DataStream ssKey(keys[i], SER_LLD, DATABASE_VERSION);
            ssKey >> nKey;

            pDatabase->ReadAddressInfo(nKey, addr_info);

            uint64_t nHash = addr_info.GetHash();

            mapAddrInfo[nHash] = addr_info;
        }

    }


    /*  Write the addresses from the manager into the address database */
    void AddressManager::WriteDatabase()
    {
        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
            pDatabase->WriteAddress(it->first, it->second);
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
    uint32_t AddressManager::get_count(const uint8_t flags) const
    {
        return static_cast<uint32_t>(get_info(flags).size());
    }

}
