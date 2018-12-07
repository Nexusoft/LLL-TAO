/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <mutex>

#include <TAO/Legacy/wallet/basickeystore.h>

namespace Legacy
{

    /*  Add a key to the key store. */
    bool CBasicKeyStore::AddKey(const ECKey& key)
    {
        bool fCompressed = false;
        CSecret secret = key.GetSecret(fCompressed);
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);
            mapKeys[NexusAddress(key.GetPubKey())] = make_pair(secret, fCompressed);
        }
        return true;
    }


    /*  Retrieve a key from the key store. */
    bool CBasicKeyStore::GetKey(const NexusAddress &address, LLC::ECKey &keyOut) const
    {
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);
            auto mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut.Reset();
                keyOut.SetSecret((*mi).second.first, (*mi).second.second);
                return true;
            }
        }
        return false;
    }


    /*  Retrieve the set of public addresses for all keys currently present in the key store. */
    void CBasicKeyStore::GetKeys(std::set<NexusAddress> &setAddress) const
    {
        setAddress.clear();
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);

            setAddress.clear();

            for (auto &mapEntry : mapKeys)
                setAddress.insert(mapEntry.first);

        }
    }


    /*  Check whether a key corresponding to a given address is present in the store. */
    bool CBasicKeyStore::HaveKey(const NexusAddress &address) const
    {
        bool result;
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }


    /*  Add a script to the key store. */
    bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
    {
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);
            mapScripts[SK256(redeemScript)] = redeemScript;
        }
        return true;
    }


    /*  Retrieve a script from the key store. */
    bool CBasicKeyStore::GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const
    {
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);
            ScriptMap::const_iterator mi = mapScripts.find(hash);
            if (mi != mapScripts.end())
            {
                redeemScriptOut = (*mi).second;
                return true;
            }
        }
        return false;
    }


    /*  Check whether a script is present in the store. */
    bool CBasicKeyStore::HaveCScript(const uint256_t& hash) const
    {
        bool result;
        {
            std::lock_guard<std::recursive_mutex> ksLock(cs_KeyStore);
            result = (mapScripts.count(hash) > 0);
        }
        return result;
    }

}
