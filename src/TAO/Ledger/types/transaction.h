/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H
#define NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H

#include <vector>

#include <LLC/types/uint1024.h>

#include <Util/include/runtime.h>
#include <Util/templates/serialize.h>

namespace TAO
{

    namespace Ledger
    {

        /** Tritium Transaction Object.

            Simple data type class that holds the transaction version, nextHash, and ledger data.

        **/
        class Transaction //transaction header size is 144 bytes
        {
        public:
            /** The transaction version. **/
            uint32_t nVersion;


            /** The sequence identifier. **/
            uint32_t nSequence;


            /** The transaction timestamp. **/
            uint64_t nTimestamp;


            /** The nextHash which can claim the signature chain. */
            uint256_t hashNext;


            /** The genesis ID hash. **/
            uint256_t hashGenesis;


            /** The previous transaction. **/
            uint512_t hashPrevTx;


            /** The data to be recorded in the ledger. **/
            std::vector<uint8_t> vchLedgerData;


            //memory only, to be disposed once fully locked into the chain behind a checkpoint
            //this is for the segregated keys from transaction data.
            std::vector<uint8_t> vchPubKey;
            std::vector<uint8_t> vchSig;


            //flag to determine if tx has been connected
            bool fConnected;


            //memory only read position
            uint32_t nReadPos;


            //serialization methods
            IMPLEMENT_SERIALIZE
            (

                READWRITE(this->nVersion);
                READWRITE(nTimestamp);
                READWRITE(hashNext);

                if(!(nSerType & SER_GENESISHASH)) //genesis hash is not serialized
                    READWRITE(hashGenesis);

                READWRITE(hashPrevTx);
                READWRITE(vchLedgerData);

                if(!(nSerType & SER_GETHASH))
                {
                    READWRITE(vchPubKey);
                    READWRITE(vchSig);
                }

                READWRITE(fConnected);
            )


            /** Default Constructor. **/
            Transaction()
            : nVersion(1)
            , nSequence(0)
            , nTimestamp(runtime::UnifiedTimestamp())
            , hashNext(0)
            , hashGenesis(0)
            , hashPrevTx(0)
            , fConnected(false)
            , nReadPos(0) {}


            /** Operator Overload <<
             *
             *  Serializes data into vchLedgerData
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type> Transaction& operator<<(const Type& obj)
            {
                /* Push the size byte into vector. */
                vchLedgerData.push_back((uint8_t)sizeof(obj));

                /* Push the obj bytes into the vector. */
                vchLedgerData.insert(vchLedgerData.end(), (uint8_t*)&obj, (uint8_t*)&obj + sizeof(obj));

                return *this;
            }


            /** Operator Overload >>
             *
             *  Serializes data into vchLedgerData
             *
             *  @param[out] obj The object to de-serialize from ledger data
             *
             **/
            template<typename Type> Transaction& operator>>(Type& obj)
            {
                /* Get the size from size byte. */
                uint8_t nSize = vchLedgerData[nReadPos];

                /* Create tmp object to prevent double free in std::copy. */
                Type tmp;

                /* Copy the bytes into tmp object. */
                std::copy((uint8_t*)&vchLedgerData[nReadPos + 1], (uint8_t*)&vchLedgerData[nReadPos + 1] + nSize, (uint8_t*)&tmp);

                /* Iterate the read position. */
                nReadPos += nSize + 1;

                /* Set the return value. */
                obj = tmp;

                return *this;
            }


            /** IsValid
             *
             *  Determines if the transaction is a valid transaciton and passes ledger level checks.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool IsValid() const;


            /** IsGenesis
             *
             *  Determines if the transaction is a genesis transaction
             *
             *  @return true if transaction is genesis
             *
             **/
            bool IsGenesis() const;


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


            /** PrevHash
             *
             *  Gets the nextHash from the previous transaction
             *
             *  @return 256-bit hash of previous transaction
             *
             **/
            uint256_t PrevHash() const;


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
