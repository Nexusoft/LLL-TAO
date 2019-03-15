/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <stdexcept>
#include <vector>

#include <LLC/types/uint1024.h>
#include <LLC/types/typedef.h>

#include <LLC/include/flkey.h>

namespace LLC
{
    /* The default constructor. */
    FLKey::FLKey()
    {

    }

    /* Copy Constructor. */
    FLKey::FLKey(const FLKey& b)
    {

    }


    /* Assignment Operator */
    FLKey& FLKey::operator=(const FLKey& b)
    {

    }


    /* Comparison Operator */
    bool FLKey::operator==(const FLKey& b) const
    {

    }


    /* Reset internal key data. */
    void FLKey::Reset()
    {

    }


    /* Determine if the key is in nullptr state, false otherwise. */
    bool FLKey::IsNull() const
    {

    }


    /* Flag to determine if the key is in compressed form. */
    bool FLKey::IsCompressed() const
    {

    }


    /* Create a new key from the Falcon random PRNG seeds */
    void FLKey::MakeNewKey(bool fCompressed)
    {

    }


    /* Set the key from full private key. (including secret) */
    bool FLKey::SetPrivKey(const CPrivKey& vchPrivKey)
    {

    }


    /* Set the secret phrase / key used in the private key. */
    bool FLKey::SetSecret(const CSecret& vchSecret, bool fCompressed)
    {

    }


    /* Obtain the secret key used in the private key. */
    CSecret FLKey::GetSecret(bool &fCompressed) const
    {

    }


    /* Obtain the private key and all associated data. */
    CPrivKey FLKey::GetPrivKey() const
    {

    }


    /* Returns true on the setting of a public key. */
    bool FLKey::SetPubKey(const std::vector<uint8_t>& vchPubKey)
    {

    }


    /* Returns the Public key in a byte vector. */
    std::vector<uint8_t> FLKey::GetPubKey() const
    {

    }


    /* Based on standard set of byte data as input of any length. */
    bool FLKey::Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig) const
    {

    }


    /* Signature Verification Function */
    bool FLKey::Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig) const
    {

    }


    /* Check if a Key is valid based on a few parameters. */
    bool FLKey::IsValid() const
    {

    }
}
