/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_SECRET_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_SECRET_H

namespace Legacy
{

    /** A base58-encoded secret key */
    class NexusSecret : public CBase58Data
    {
    public:
        void SetSecret(const CSecret& vchSecret, bool fCompressed)
        {
            assert(vchSecret.size() == 72);
            SetData(128 + (fTestNet ? NexusAddress::PUBKEY_ADDRESS_TEST : NexusAddress::PUBKEY_ADDRESS), &vchSecret[0], vchSecret.size());
            if (fCompressed)
                vchData.push_back(1);
        }

        CSecret GetSecret(bool &fCompressedOut)
        {
            CSecret vchSecret;
            vchSecret.resize(72);
            memcpy(&vchSecret[0], &vchData[0], 72);
            fCompressedOut = vchData.size() == 73;
            return vchSecret;
        }

        bool IsValid() const
        {
            bool fExpectTestNet = false;
            switch(nVersion)
            {
                case (128 + NexusAddress::PUBKEY_ADDRESS):
                    break;

                case (128 + NexusAddress::PUBKEY_ADDRESS_TEST):
                    fExpectTestNet = true;
                    break;

                default:
                    return false;
            }
            return fExpectTestNet == fTestNet && (vchData.size() == 72 || (vchData.size() == 73 && vchData[72] == 1));
        }

        bool SetString(const char* pszSecret)
        {
            return CBase58Data::SetString(pszSecret) && IsValid();
        }

        bool SetString(const std::string& strSecret)
        {
            return SetString(strSecret.c_str());
        }

        NexusSecret(const CSecret& vchSecret, bool fCompressed)
        {
            SetSecret(vchSecret, fCompressed);
        }

        NexusSecret()
        {
        }
    };

}

#endif
