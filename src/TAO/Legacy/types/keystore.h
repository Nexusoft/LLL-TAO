/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_KEYSTORE_H
#define NEXUS_TAO_LEGACY_TYPES_KEYSTORE_H

namespace Wallet
{
    /** A virtual base class for key stores */
    class CKeyStore
    {
    protected:
        mutable std::recursive_mutex cs_KeyStore;

    public:
        virtual ~CKeyStore() {}

        // Add a key to the store.
        virtual bool AddKey(const ECKey& key) =0;

        // Check whether a key corresponding to a given address is present in the store.
        virtual bool HaveKey(const NexusAddress &address) const =0;
        virtual bool GetKey(const NexusAddress &address, ECKey& keyOut) const =0;
        virtual void GetKeys(std::set<NexusAddress> &setAddress) const =0;
        virtual bool GetPubKey(const NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const;
        virtual bool AddCScript(const CScript& redeemScript) =0;
        virtual bool HaveCScript(const uint256_t &hash) const =0;
        virtual bool GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const =0;

        virtual bool GetSecret(const NexusAddress &address, CSecret& vchSecret, bool &fCompressed) const
        {
            LLC::ECKey key;
            if (!GetKey(address, key))
                return false;

            vchSecret = key.GetSecret(fCompressed);
            return true;
        }
    };

    typedef std::map<NexusAddress, std::pair<CSecret, bool> > KeyMap;
    typedef std::map<uint256_t, CScript > ScriptMap;

    /** Basic key store, that keeps keys in an address->secret map */
    class CBasicKeyStore : public CKeyStore
    {
    protected:
        KeyMap mapKeys;
        ScriptMap mapScripts;

    public:
        bool AddKey(const LLC::ECKey& key);


        bool HaveKey(const NexusAddress &address) const
        {
            bool result;
            {
                LOCK(cs_KeyStore);
                result = (mapKeys.count(address) > 0);
            }
            return result;
        }


        void GetKeys(std::set<NexusAddress> &setAddress) const
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


        bool GetKey(const NexusAddress &address, LLC::ECKey &keyOut) const
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

        virtual bool AddCScript(const CScript& redeemScript);
        virtual bool HaveCScript(const uint256_t &hash) const;
        virtual bool GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const;
    };

}

#endif
