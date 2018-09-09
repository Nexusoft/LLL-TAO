/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H
#define NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H

#include <vector>

#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

namespace TAO
{

    namespace Ledger
    {

        /** Tritium Transaction Object.

            Simple data type class that holds the transaction version, nextHash, and ledger data.

        **/
        class Transaction
        {
        public:
            /** The transaction version. **/
            uint32_t nVersion;


            /** The transaction timestamp. **/
            uint64_t nTimestamp;


            /** The nextHash which can claim the signature chain. */
            uint256_t hashNext;


            /** The genesis ID hash. **/
            uint256_t hashGenesis;


            /** The previous transaction. **/
            uint256_t hashPrevTx;


            /** The data to be recorded in the ledger. **/
            std::vector<uint8_t> vchLedgerData;


            //memory only, to be disposed once fully locked into the chain behind a checkpoint
            //this is for the segregated keys from transaction data.
            std::vector<uint8_t> vchPubKey;
            std::vector<uint8_t> vchSig;


            //serialization methods
            IMPLEMENT_SERIALIZE
            (

                READWRITE(this->nVersion);
                READWRITE(nTimestamp);
                READWRITE(hashNext);
                READWRITE(hashGenesis);
                READWRITE(vchLedgerData);

                if(!(nType & SER_GETHASH))
                {
                    READWRITE(vchPubKey);
                    READWRITE(vchSig);
                }
            )


            /** Default Constructor. **/
            Transaction() : nVersion(0), nTimestamp(0), hashNext(0), hashGenesis(0), hashPrevTx(0) {}


            /** IsValid
             *
             *  Determines if the transaction is a valid transaciton and passes ledger level checks.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool IsValid() const;


            /** GetHash
             *
             *  Gets the hash of the transaction object.
             *
             *  @return 512-bit unsigned integer of hash.
             *
             **/
            uint512_t GetHash() const;


            /** NextHash
             *
             *  Sets the Next Hash from the key
             *
             *  @param[in] hashSecret The secret phrase to generate the keys.
             *
             **/
            void NextHash(uint512_t hashSecret);


            /** Sign
             *
             *  Signs the transaction with the private key and sets the public key
             *
             *  @param[in] hashSecret The secret phrase to generate the keys.
             *
             **/
             bool Sign(uint512_t hashSecret);



             /** Print
              *
              * Prints the object to the console.
              *
              **/
             void print() const;

        };
    }
}

#endif
