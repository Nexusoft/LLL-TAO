/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <TAO/Legacy/keystore/basic.h>

namespace Legacy
{

    /* Add a key to this keystore. */
    bool CBasicKeyStore::AddKey(const LLC::ECKey& key)
    {
        bool fCompressed = false;
        LLC::CSecret secret = key.GetSecret(fCompressed);
        {
            LOCK(cs_KeyStore);
            mapKeys[NexusAddress(key.GetPubKey())] = make_pair(secret, fCompressed);
        }
        return true;
    }


    /* Checks if a key exists in the keystore. */
    bool CBasicKeyStore::HaveKey(const NexusAddress &address) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }


    /* Gets keys from this keystore. */
    void CBasicKeyStore::GetKeys(std::set<NexusAddress> &setAddress) const
    {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            std::map<NexusAddress, std::pair<LLC::CSecret, bool> >::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end())
            {
                setAddress.insert((*mi).first);
                mi++;
            }
        }
    }


    /* Get a single key from the keystore by address. */
    bool CBasicKeyStore::GetKey(const NexusAddress &address, LLC::ECKey &keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            std::map<NexusAddress, std::pair<LLC::CSecret, bool> >::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut.Reset();
                keyOut.SetSecret((*mi).second.first, (*mi).second.second);
                return true;
            }
        }
        return false;
    }


    /* Add a script to this keystore. */
    bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
    {
        {
            LOCK(cs_KeyStore);
            mapScripts[LLC::SK256(redeemScript)] = redeemScript;
        }

        return true;
    }


    /* Check the keystore if a script exists. */
    bool CBasicKeyStore::HaveCScript(const uint256_t &hash) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapScripts.count(hash) > 0);
        }
        return result;
    }


    /* Get a script from the keystore. */
    bool CBasicKeyStore::GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const
    {
        {
            LOCK(cs_KeyStore);
            std::map<uint256_t, CScript>::const_iterator mi = mapScripts.find(hash);
            if (mi != mapScripts.end())
            {
                redeemScriptOut = (*mi).second;
                return true;
            }
        }
        return false;
    }
}
