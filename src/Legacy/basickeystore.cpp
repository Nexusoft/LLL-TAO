/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <Legacy/wallet/basickeystore.h>

namespace Legacy
{

    /* The default constructor. */
    BasicKeyStore::BasicKeyStore()
    : mapKeys    ( )
    , mapScripts ( )
    {
    }


    /* Default Destructor */
    BasicKeyStore::~BasicKeyStore()
    {
    }


    /*  Add a key to the key store. */
    bool BasicKeyStore::AddKey(const LLC::ECKey& key)
    {
        bool fCompressed = false;
        LLC::CSecret secret = key.GetSecret(fCompressed);
        {
            LOCK(cs_basicKeyStore);
            mapKeys[NexusAddress(key.GetPubKey())] = make_pair(secret, fCompressed);
        }
        
        return true;
    }


    /*  Retrieve a key from the key store. */
    bool BasicKeyStore::GetKey(const NexusAddress& address, LLC::ECKey &keyOut) const
    {
        {
            LOCK(cs_basicKeyStore);
            auto mi = mapKeys.find(address);
            if(mi != mapKeys.end())
            {
                keyOut.Reset();
                keyOut.SetSecret((*mi).second.first, (*mi).second.second);
                return true;
            }
        }
        return false;
    }


    /*  Retrieve the set of public addresses for all keys currently present in the key store. */
    void BasicKeyStore::GetKeys(std::set<NexusAddress>& setAddress) const
    {
        setAddress.clear();
        {
            LOCK(cs_basicKeyStore);

            setAddress.clear();

            for(auto &mapEntry : mapKeys)
                setAddress.insert(mapEntry.first);

        }
    }


    /*  Check whether a key corresponding to a given address is present in the store. */
    bool BasicKeyStore::HaveKey(const NexusAddress& address) const
    {
        bool result;
        {
            LOCK(cs_basicKeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }


    /*  Add a script to the key store. */
    bool BasicKeyStore::AddScript(const Script& redeemScript)
    {
        {
            LOCK(cs_basicKeyStore);
            mapScripts[LLC::SK256(redeemScript)] = redeemScript;
        }
        return true;
    }


    /*  Retrieve a script from the key store. */
    bool BasicKeyStore::GetScript(const uint256_t& hash, Script &redeemScriptOut) const
    {
        {
            LOCK(cs_basicKeyStore);
            ScriptMap::const_iterator mi = mapScripts.find(hash);
            if(mi != mapScripts.end())
            {
                redeemScriptOut = (*mi).second;
                return true;
            }
        }
        return false;
    }


    /*  Check whether a script is present in the store. */
    bool BasicKeyStore::HaveScript(const uint256_t& hash) const
    {
        bool result;
        {
            LOCK(cs_basicKeyStore);
            result = (mapScripts.count(hash) > 0);
        }
        return result;
    }

}
