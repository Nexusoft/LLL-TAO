/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_ADDRESS_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_ADDRESS_H

namespace Legacy
{

/** base58-encoded nexus addresses.
* Public-key-hash-addresses have version 55 (or 111 testnet).
* The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
* Script-hash-addresses have version 117 (or 196 testnet).
* The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
*/
class NexusAddress : public CBase58Data
{
public:
    enum
    {
        PUBKEY_ADDRESS = 42,
        SCRIPT_ADDRESS = 104,
        PUBKEY_ADDRESS_TEST = 111,
        SCRIPT_ADDRESS_TEST = 196,
    };

    bool SetHash256(const uint256& hash256)
    {
        SetData(fTestNet ? PUBKEY_ADDRESS_TEST : PUBKEY_ADDRESS, &hash256, 32);
        return true;
    }

    void SetPubKey(const std::vector<uint8_t>& vchPubKey)
    {
        SetHash256(SK256(vchPubKey));
    }

    bool SetScriptHash256(const uint256& hash256)
    {
        SetData(fTestNet ? SCRIPT_ADDRESS_TEST : SCRIPT_ADDRESS, &hash256, 32);
        return true;
    }

    bool IsValid() const
    {
        unsigned int nExpectedSize = 32;
        bool fExpectTestNet = false;
        switch(nVersion)
        {
            case PUBKEY_ADDRESS:
                nExpectedSize = 32; // Hash of public key
                fExpectTestNet = false;
                break;
            case SCRIPT_ADDRESS:
                nExpectedSize = 32; // Hash of CScript
                fExpectTestNet = false;
                break;

            case PUBKEY_ADDRESS_TEST:
                nExpectedSize = 32;
                fExpectTestNet = true;
                break;
            case SCRIPT_ADDRESS_TEST:
                nExpectedSize = 32;
                fExpectTestNet = true;
                break;

            default:
                return false;
        }
        return fExpectTestNet == fTestNet && vchData.size() == nExpectedSize;
    }
    bool IsScript() const
    {
        if (!IsValid())
            return false;
        if (fTestNet)
            return nVersion == SCRIPT_ADDRESS_TEST;
        return nVersion == SCRIPT_ADDRESS;
    }

    NexusAddress()
    {
    }

    NexusAddress(uint256 hash256In)
    {
        SetHash256(hash256In);
    }

    NexusAddress(const std::vector<uint8_t>& vchPubKey)
    {
        SetPubKey(vchPubKey);
    }

    NexusAddress(const std::string& strAddress)
    {
        SetString(strAddress);
    }

    NexusAddress(const char* pszAddress)
    {
        SetString(pszAddress);
    }

    uint256 GetHash256() const
    {
        assert(vchData.size() == 32);
        uint256 hash256;
        memcpy(&hash256, &vchData[0], 32);
        return hash256;
    }
};

}

#endif
