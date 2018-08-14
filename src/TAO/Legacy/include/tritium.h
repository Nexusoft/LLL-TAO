/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_TRITIUM_H
#define NEXUS_TAO_LEDGER_INCLUDE_TRITIUM_H

namespace TAO
{

    namespace Ledger 
    {

        class CTritiumTransaction
        {

            /** The transaction version for extensibility. **/
            unsigned int nVersion;

            /** The computed hash of the next key in the series. **/
            uint256 hashNext;

            /** The genesis ID of this signature chain. **/
            uint256 hashGenesis;

            /** The serialized byte code that is to be encoded in the chain. **/
            CData vchLedgerData; //1260 bytes max size (MTU for IPv6)

            /** MEMORY ONLY: The Binary data of the Public Key revealed in this transaction. **/
            mutable std::vector<unsigned char> vchPubKey;

            /** MEMORY ONLY: The Binary data of the Signature revealed in this transaction. **/
            mutable std::vector<unsigned char> vchSignature;

            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(hashNext);
                READWRITE(FLATDATA(vchLedgerData));

                if(!(nType & SER_LLD)) {
                READWRITE(vchPubKey);
                READWRITE(vchSignature);
                }
            }


            //TODO: Get this indexing right
            uint512 GetPrevHash() const
            {
                //return LLC::HASH::SK256(vchPubKey); //INDEX
            }


            uint512 GetHash() const
            {
                return LLC::HASH::SK512(BEGIN(nVersion), END(vchLedgerData));
            }


            /** IsValid
            *
            *  Checks if the transaction is valid. Only called on validation process, not to be run from disk since it requires memory only data.
            *
            *   @return Returns whether the key is valid.
            */
            bool IsValid()
            {
                /* Generate the key object to check signature. */
                LLC::CKey key(NID_brainpoolP512t1, 64);
                key.SetPubKey(vchPubKey);

                /* Verify that the signature is valid. */
                if(!key.Verify(GetHash().GetBytes(), vchSignature))
                    return false;

                return true;
            }

        };

    }

}

#endif
