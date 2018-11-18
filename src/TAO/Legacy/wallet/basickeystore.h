/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_BASICKEYSTORE_H
#define NEXUS_LEGACY_WALLET_BASICKEYSTORE_H

#include <map>
#include <set>
#include <utility>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/wallet/keystore.h>

/* forward declarations */    
namespace LLC 
{
    class CSecret;
    class ECKey;
}

namespace Legacy 
{
    
    /* forward declarations */    
    namespace Types
    {
        class NexusAddress;
        class CScript;
    }

    namespace Wallet
    {
        typedef std::map<Legacy::Types::NexusAddress, std::pair<LLC::CSecret, bool> > KeyMap;
        typedef std::map<uint256_t, Legacy::Types::CScript > ScriptMap;

        /** Basic key store, that keeps keys in an address->secret map */
        class CBasicKeyStore : public CKeyStore
        {
        protected:
            KeyMap mapKeys;
            ScriptMap mapScripts;

        public:
            bool AddKey(const LLC::ECKey& key);
            bool HaveKey(const Legacy::Types::NexusAddress &address) const
            {
                bool result;
                {
                    LOCK(cs_KeyStore);
                    result = (mapKeys.count(address) > 0);
                }
                return result;
            }
            void GetKeys(std::set<Legacy::Types::NexusAddress> &setAddress) const
            {
                setAddress.clear();
                {
                    LOCK(cs_KeyStore);
                    KeyMap::const_iterator mi = mapKeys.begin();
                    while (mi != mapKeys.end())
                    {
                        setAddress.insert((*mi).first);
                        mi++;
                    }
                }
            }
            bool GetKey(const Legacy::Types::NexusAddress &address, LLC::ECKey &keyOut) const
            {
                {
                    LOCK(cs_KeyStore);
                    KeyMap::const_iterator mi = mapKeys.find(address);
                    if (mi != mapKeys.end())
                    {
                        keyOut.Reset();
                        keyOut.SetSecret((*mi).second.first, (*mi).second.second);
                        return true;
                    }
                }
                return false;
            }
            virtual bool AddCScript(const Legacy::Types::CScript& redeemScript);
            virtual bool HaveCScript(const uint256_t &hash) const;
            virtual bool GetCScript(const uint256_t &hash, Legacy::Types::CScript& redeemScriptOut) const;
        };

    }
}

#endif
