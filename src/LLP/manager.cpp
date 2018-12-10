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
#include <Util/include/runtime.h>
#include <Util/include/debug.h>
#include <algorithm>

namespace LLP
{

    /* Default constructor */
    AddressManager::AddressManager()
    : pDatabase(new LLD::AddressDB)
    , mapAddr()
    , mapInfo()
    {
        if(!pDatabase)
            debug::error(FUNCTION "Failed to allocate memory for AddressManager\n", __PRETTY_FUNCTION__);
    }


    /* Default destructor */
    AddressManager::~AddressManager()
    {
        for(auto it = mapAddr.begin(); it != mapAddr.end(); ++it)
            pDatabase->WriteAddress(it->first, it->second);

        for(auto it = mapInfo.begin(); it != mapInfo.end(); ++it)
            pDatabase->WriteInfo(it->first, it->second);
    }


    /*  Gets a list of addresses in the manager */
    std::vector<Address> AddressManager::GetAddresses(const uint8_t flags)
    {
        std::vector<Address> vAddr;
        std::vector<AddressInfo> vInfo;

        std::unique_lock<std::mutex> lk(mutex);

        vInfo = get_info(flags);
        for(auto it = vInfo.begin(); it != vInfo.end(); ++it)
        {
            auto it2 = mapAddr.find(it->nHash);

            if(it2 != mapAddr.end())
                vAddr.push_back(it2->second);
        }

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

        if(mapAddr.find(hash) == mapAddr.end())
        {
            mapAddr[hash] = addr;
            mapInfo[hash] = AddressInfo(&addr);
        }

        AddressInfo *pInfo = &mapInfo[hash];
        int64_t timestamp_ms = UnifiedTimestamp(true);

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
            pInfo->nSession = timestamp_ms - pInfo->nLastSeen;
            pInfo->nLastSeen = timestamp_ms;
            pInfo->nState = state;
            break;

        case ConnectState::CONNECTED:
            if(pInfo->nState == ConnectState::CONNECTED)
                break;

            pInfo->nFails = 0;
            ++(pInfo->nConnected);
            pInfo->nSession = 0;
            pInfo->nLastSeen = timestamp_ms;
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

        auto it = mapInfo.find(addr.GetHash());
        if(it != mapInfo.end())
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

        //get a hash from the timestamp
        uint64_t ms = UnifiedTimestamp(true);
        uint64_t hash = LLC::SK64(BEGIN(ms), END(ms));

        /* compute index from an inverse of that hash that will select more
         * from the front of the list */
        uint8_t byte = static_cast<uint8_t>(0xFFFFFFFFFFFFFFF / hash);
        uint8_t size = static_cast<uint8_t>(vInfo.size() - 1);
        uint8_t i = std::min(byte, size);

        //debug::log(5, FUNCTION "Selected: %u %u %u\n", __PRETTY_FUNCTION__, byte, size, i);

        //find the address from the info and return it if it exists
        auto it = mapAddr.find(vInfo[i].nHash);
        if(it == mapAddr.end())
            return false;

        addr = it->second;

        //debug::log(5, FUNCTION "Selected: %s\n", __PRETTY_FUNCTION__, addr.ToString().c_str());

        return true;
    }


    /*  Helper function to get an array of info on the connected states specified
     *  by flags */
    std::vector<AddressInfo> AddressManager::get_info(const uint8_t flags) const
    {
        std::vector<AddressInfo> addrs;
        for(auto it = mapInfo.begin(); it != mapInfo.end(); ++it)
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
