/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <assert.h>

#include <Legacy/types/secret.h>
#include <Legacy/types/enum.h>

#include <Util/include/args.h>

namespace Legacy
{

    /* Sets from a CSecret phrase (byte vector in secure allocator) */
    NexusSecret::NexusSecret(const LLC::CSecret& vchSecret, bool fCompressed)
    {
        SetSecret(vchSecret, fCompressed);
    }


    /* Sets from a CSecret phrase (byte vector in secure allocator) */
    void NexusSecret::SetSecret(const LLC::CSecret& vchSecret, bool fCompressed)
    {
        assert(vchSecret.size() == 72);
        SetData(128 + (config::fTestNet ? PUBKEY_ADDRESS_TEST : PUBKEY_ADDRESS), &vchSecret[0], vchSecret.size());
        if (fCompressed)
            vchData.push_back(1);
    }


    /* Gets the CSecret phrase (byte vector in secure allocator) */
    LLC::CSecret NexusSecret::GetSecret(bool &fCompressedOut)
    {
        LLC::CSecret vchSecret;
        vchSecret.resize(72);
        memcpy(&vchSecret[0], &vchData[0], 72);
        fCompressedOut = vchData.size() == 73;
        return vchSecret;
    }


    /* Check if object passes validity tests */
    bool NexusSecret::IsValid() const
    {
        bool fExpectTestNet = false;
        switch(nVersion)
        {
            case (128 + PUBKEY_ADDRESS):
                break;

            case (128 + PUBKEY_ADDRESS_TEST):
                fExpectTestNet = true;
                break;

            default:
                return false;
        }
        return fExpectTestNet == config::fTestNet && (vchData.size() == 72 || (vchData.size() == 73 && vchData[72] == 1));
    }


    /* Sets the secret from a Base58 encoded c-style string. */
    bool NexusSecret::SetString(const char* pszSecret)
    {
        return CBase58Data::SetString(pszSecret) && IsValid();
    }


    /* Sets the secret from a Base58 encoded std::string. */
    bool NexusSecret::SetString(const std::string& strSecret)
    {
        return SetString(strSecret.c_str());
    }

}
