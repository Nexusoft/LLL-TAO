/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <assert.h>

#include "include/address.h"

#include "../../../LLC/hash/SK.h"
#include "../../../Util/include/args.h"


namespace Legacy
{


    /* Constructor Set the hash256 */
    NexusAddress::NexusAddress(uint256_t hash256In)
    {
        SetHash256(hash256In);
    }


    /* Constructor Set by the public key */
    NexusAddress::NexusAddress(const std::vector<uint8_t>& vchPubKey)
    {
        SetPubKey(vchPubKey);
    }


    /* Constructor Set by a std::string */
    NexusAddress::NexusAddress(const std::string& strAddress)
    {
        SetString(strAddress);
    }


    /* Constructor Set by a c-style string */
    NexusAddress::NexusAddress(const char* pszAddress)
    {
        SetString(pszAddress);
    }


    /* Sets the generated hash for address */
    void NexusAddress::SetHash256(const uint256_t& hash256)
    {
        SetData(fTestNet ? PUBKEY_ADDRESS_TEST : PUBKEY_ADDRESS, &hash256, 32);
    }


    /* Sets the public key by hashing it to generate address hash */
    void NexusAddress::SetPubKey(const std::vector<uint8_t>& vchPubKey)
    {
        SetHash256(LLC::SK256(vchPubKey));
    }


    /* Sets the address based on the hash of the script */
    void NexusAddress::SetScriptHash256(const uint256_t& hash256)
    {
        SetData(fTestNet ? SCRIPT_ADDRESS_TEST : SCRIPT_ADDRESS, &hash256, 32);
    }


    /* Preliminary check to ensure the address is mapped to a valid public key or script */
    bool NexusAddress::IsValid() const
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


    /* Check of version byte to see if address is P2SH */
    bool NexusAddress::IsScript() const
    {
        if (!IsValid())
            return false;
        if (fTestNet)
            return nVersion == SCRIPT_ADDRESS_TEST;
        return nVersion == SCRIPT_ADDRESS;
    }


    /* Get the hash from byte vector */
    uint256_t NexusAddress::GetHash256() const
    {
        assert(vchData.size() == 32);
        uint256_t hash256;
        memcpy(&hash256, &vchData[0], 32);
        return hash256;
    }
}
