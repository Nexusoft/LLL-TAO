/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H
#define NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H

#include <vector>

#include <TAO/Operation/types/contract.h>

#include <TAO/Register/types/stream.h>
#include <TAO/Register/include/enum.h>

#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        class BlockState;


        /** Transaction
         *
         *  A Tritium Transaction.
         *  Stores state of a tritium specific transaction.
         *
         *  transaction header size is 144 bytes
         *
         **/
        class Transaction
        {
            /** For disk indexing on contract. **/
            std::vector<TAO::Operation::Contract> vContracts;

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


            /** The next transaction. **/
            uint512_t hashNextTx;


            /** The key type. **/
            uint8_t nKeyType;


            /** The next key type. **/
            uint8_t nNextType;


            //memory only, to be disposed once fully locked into the chain behind a checkpoint
            //this is for the segregated keys from transaction data.
            std::vector<uint8_t> vchPubKey;
            std::vector<uint8_t> vchSig;


            /** Serialization **/
            IMPLEMENT_SERIALIZE
            (
                /* Contracts layers. */
                READWRITE(vContracts);

                /* Ledger layer */
                READWRITE(nVersion);
                READWRITE(nSequence);
                READWRITE(nTimestamp);
                READWRITE(hashNext);
                READWRITE(hashGenesis);
                READWRITE(hashPrevTx);

                /* Only write hash next when to disk. */
                if(!(nSerType & SER_GETHASH) && (nSerType & SER_LLD))
                    READWRITE(hashNextTx);

                READWRITE(nKeyType);
                READWRITE(nNextType);
                READWRITE(vchPubKey);

                /* Handle for when not getting hash. */
                if(!(nSerType & SER_GETHASH))
                {
                    READWRITE(vchSig);

                    /* When reading and writing transaciton, build memory only data for contracts. */
                    for(auto& contract : vContracts)
                    {
                        contract.hashCaller = hashGenesis;
                        contract.nTimestamp = nTimestamp;
                    }
                }
            )


            /** Default Constructor. **/
            Transaction()
            : vContracts()
            , nVersion(1)
            , nSequence(0)
            , nTimestamp(runtime::unifiedtimestamp())
            , hashNext(0)
            , hashGenesis(0)
            , hashPrevTx(0)
            , hashNextTx(0)
            , nKeyType(0)
            , nNextType(0)
            , vchPubKey()
            , vchSig()
            {}


            /** Operator Overload >
             *
             *  Used for sorting transactions by sequence.
             *
             **/
            bool operator>(const Transaction& tx) const
            {
                return nSequence > tx.nSequence;
            }


             /** Operator Overload <
              *
              *  Used for sorting transactions by sequence.
              *
              **/
            bool operator<(const Transaction& tx) const
            {
                return nSequence < tx.nSequence;
            }


            /** Operator Overload []
             *
             *  Access for the contract operator overload.
             *  This is for read-only objects.
             *
             **/
            const Contract& operator[](const uint32_t n) const
            {
                /* Check contract bounds. */
                if(n >= vContracts.size())
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "Contract read out of bounds"));

                /* Check timestamp memory values. */
                if(vContracts[n].nTimestamp != nTimestamp)
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "contract timestamp mismatch"));

                /* Check caller memory values. */
                if(vContracts[n].hashCaller != hashGenesis)
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "contract caller mismatch"));

                return vContracts[n];
            }


            /** Operator Overload []
             *
             *  Write access fot the contract operator overload.
             *  This handles writes to create new contracts.
             *
             **/
            Contract& operator[](const uint32_t n)
            {
                /* Allocate a new contract if on write. */
                if(n >= vContracts.size())
                    vContracts.resize(n + 1);

                /* Set the caller hash. */
                vContracts[n].hashCaller  = hashGenesis;

                /* Set the contract timestamp. */
                vContracts[n].nTimestamp  = nTimestamp;

                return vContracts[n];
            }


            /** Check
             *
             *  Determines if the transaction is a valid transaciton and passes ledger level checks.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool Check() const;


            /** Accept
             *
             *  Accept a transaction object into the main chain.
             *
             *  @param[in] nFlags Flag to tell whether transaction is a mempool check.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool Accept() const;


            /** Connect
             *
             *  Connect a transaction object to the main chain.
             *
             *  @param[in] nFlags Flag to tell whether transaction is a mempool check.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool Connect(const uint8_t nFlags = TAO::Register::FLAGS::WRITE) const;


            /** Disconnect
             *
             *  Disconnect a transaction object to the main chain.
             *
             *  @param[in] nFlags Flag to tell whether transaction is a mempool check.
             *
             *  @return true if transaction is valid.
             *
             **/
            bool Disconnect(const uint8_t nFlags = TAO::Register::FLAGS::WRITE) const;


            /** IsCoinbase
             *
             *  Determines if the transaction is a coinbase transaction.
             *
             *  @return true if transaction is a coinbase.
             *
             **/
            bool IsCoinbase() const;


            /** IsTrust
             *
             *  Determines if the transaction is a trust transaction.
             *
             *  @return true if transaction is a coinbase.
             *
             **/
            bool IsTrust() const;


            /** IsHead
             *
             *  Determines if the transaction is at head of chain.
             *
             *  @return true if transaction is at head.
             *
             **/
            bool IsHead() const;


            /** Confirmed
             *
             *  Determines if the transaction is confirmed in the chain.
             *
             *  @return true if transaction is confirmed.
             *
             **/
            bool IsConfirmed() const;


            /** IsPrivate
             *
             *  Determines if the transaction is for a private block.
             *
             *  @return true if transaction is a coinbase.
             *
             **/
            bool IsPrivate() const;


            /** IsGenesis
             *
             *  Determines if the transaction is a genesis transaction
             *
             *  @return true if transaction is genesis
             *
             **/
            bool IsGenesis() const;


            /** IsFirst
             *
             *  Determines if the transaction is the first transaction
             *  in a signature chain
             *
             *  @return true if transaction is first
             *
             **/
            bool IsFirst() const;


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
             *  @param[in] nType The type of key to create with.
             *
             **/
            void NextHash(const uint512_t& hashSecret, const uint8_t nType);


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
             bool Sign(const uint512_t& hashSecret);


             /** print
              *
              *  Prints the object to the console.
              *
              **/
             void print() const;


             /** ToString
             *
             *  Create a transaction string
             *
             *  @return The string value to return;
             *
             **/
            std::string ToString() const;


             /** ToStringShort
             *
             *  Short form of the debug output.
             *
             *  @return The string value to return;
             *
             **/
            std::string ToStringShort() const;


            /** GetTxTypeString
             *
             *  User readable description of the transaction type.
             *
             *  @return User readable description of the transaction type;
             *
             **/
            std::string GetTxTypeString() const;

        };
    }
}

#endif
