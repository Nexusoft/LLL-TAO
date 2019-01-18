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

#include <TAO/Operation/include/stream.h>
#include <TAO/Register/include/stream.h>

#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Tritium Transaction.
         *
         *  State of a tritium specific transaction.
         *
         **/
        class Transaction //transaction header size is 144 bytes
        {
        public:

            /** The operations that create post-states. **/
            TAO::Operation::Stream ssOperation;


            /** The register pre-states. **/
            TAO::Register::Stream  ssRegister;


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


            //memory only, to be disposed once fully locked into the chain behind a checkpoint
            //this is for the segregated keys from transaction data.
            std::vector<uint8_t> vchPubKey;
            std::vector<uint8_t> vchSig;


            //memory only read position
            uint32_t nReadPos;


            //serialization methods
            IMPLEMENT_SERIALIZE
            (
                /* Operations layer. */
                READWRITE(ssOperation);

                /* Register layer. */
                READWRITE(ssRegister);

                /* Ledger layer */
                READWRITE(this->nVersion);
                READWRITE(nSequence);
                READWRITE(nTimestamp);
                READWRITE(hashNext);
                READWRITE(hashGenesis);
                READWRITE(hashPrevTx);
                READWRITE(vchPubKey);
                if(!(nSerType & SER_GETHASH))
                    READWRITE(vchSig);
            )


            /** Default Constructor. **/
            Transaction()
            : nVersion(1)
            , nSequence(0)
            , nTimestamp(runtime::unifiedtimestamp())
            , hashNext(0)
            , hashGenesis(0)
            , hashPrevTx(0)
            , nReadPos(0) {}


            /** Operator Overload <<
             *
             *  Serializes data into vchOperations
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type> Transaction& operator<<(const Type& obj)
            {
                /* Serialize to the stream. */
                ssOperation << obj;

                return (*this);
            }


            /** IsValid
             *
             *  Determines if the transaction is a valid transaciton and passes ledger level checks.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool IsValid() const;


            /** Extract Trust
             *
             *  Extract the trust data from the input script.
             *
             *  @param[out] hashLastBlock The last block to extract.
             *  @param[out] nSequence The sequence number of proof of stake blocks.
             *  @param[out] nTrustScore The trust score to extract.
             *
             **/
            bool ExtractTrust(uint1024_t& hashLastBlock, uint32_t& nSequence, uint32_t& nTrustScore) const;


            /** Extract Stake
             *
             *  Extract the stake data from the input script.
             *
             *  @param[out] nStake The amount being staked.
             *
             **/
            bool ExtractStake(uint64_t& nStake) const;


            /** Is Coinbase
             *
             *  Determines if the transaction is a coinbase transaction.
             *
             *  @return true if transaction is a coinbase.
             *
             **/
            bool IsCoinbase() const;


            /** Is Trust
             *
             *  Determines if the transaction is a trust transaction.
             *
             *  @return true if transaction is a coinbase.
             *
             **/
            bool IsTrust() const;


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
