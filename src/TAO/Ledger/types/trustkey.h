/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_TRUST_H
#define NEXUS_TAO_LEDGER_INCLUDE_TRUST_H

#include <vector>

#include <LLC/hash/macro.h>
#include <LLC/hash/SK.h>
#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        class TritiumBlock;


        /** @class TrustKey
         *
         *  Class to Store a legacy Trust Key and its Interest Rate.
         *
         **/
        class TrustKey
        {
        public:
            /** The Public Key associated with this Trust Key. **/
            std::vector<uint8_t> vchPubKey;


            /** Trust Key version **/
            int32_t nVersion;


            /** Hash of legacy block containing the Genesis transaction for this Trust Key **/
            uint1024_t hashGenesisBlock;


            /** Hash of the Genesis transaction for this Trust Key **/
            uint512_t hashGenesisTx;


            /** Timestamp of the Genesis transaction for this Trust Key **/
            int32_t nGenesisTime;


            /** Previous Blocks Vector to store list of blocks for this Trust Key. **/
            mutable std::vector<uint1024_t> hashPrevBlocks;


            /** Constructor
             *
             *  Initializes a null Trust Key.
             *
             **/
            TrustKey();


            /** Constructor
             *
             *  Initializes a Trust Key.
             *
             *  @param[in] vchPubKeyIn Public key for this Trust Key
             *  @param[in] hashBlockIn The hashGenesisBlock value for this Trust Key
             *  @param[in] hashTxIn The hashGenesisTx value for this Trust Key
             *  @param[in] nTimeIn The nGenesisTime value for this Trust Key
             *
             **/
            TrustKey(const std::vector<uint8_t> vchPubKeyIn, const uint1024_t hashBlockIn, const uint512_t hashTxIn, const int32_t nTimeIn);


            /* Define Serialization/Deserialization for Trust Key */
            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(vchPubKey);
                READWRITE(hashGenesisBlock);
                READWRITE(hashGenesisTx);
                READWRITE(nGenesisTime);
            )


            /** SetNull
             *
             *  Set the Trust Key data to Null (uninitialized) values.
             *
             **/
            void SetNull();


            /** GetAge
             *
             *  Retrieve how old the Trust Key is at a given point in time.
             *
             *  @param[in] nTime The timestamp of the end time for determining age
             *
             *  @return Elapsed time between Trust Key Genesis timestamp and the requested nTime, or 0 if nTime < nGenesisTime
             *
             **/
            uint64_t GetAge(const uint32_t nTime) const;


            /** GetBlockAge
             *
             *  Retrieve the time since this Trust Key last generated a Proof of Stake block.
             *
             *  @param[in] nTime The timestamp of the end time for determining age
             *
             *  @return Elapsed time between last block genereated and the requested nTime
             *
             **/
            uint64_t GetBlockAge(const uint32_t nTime) const;


            /** GetHash
             *
             *  Retrieve hash of a Trust Key to Verify the Key's Root.
             *
             *  @return Hash of Public Key associated with this Trust Key
             *
             **/
            inline uint512_t GetHash() const { return LLC::SK512(vchPubKey, BEGIN(hashGenesisBlock), END(nGenesisTime)); }


            /** IsExpired
             *
             *  Determine if a key is expired at a given point in time.
             *  Expiration only applies to nVersion=4 or earlier trust keys, though this method may be called for any.
             *
             *  @param[in] nTime The timestamp of the point in time to check for expiration
             *
             *  @return true if Trust Key expired, false otherwise
             *
             */
            bool IsExpired(uint32_t nTime) const;


            /** IsNull
             *
             *  Check if the Trust Key data contains Null (uninitialized) values.
             *
             *  @return true if Trust Key uninitialized, false otherwise
             *
             */
            inline bool IsNull() const
            {
                return (hashGenesisBlock == 0 && hashGenesisTx == 0 && nGenesisTime == 0 && vchPubKey.empty());
            }


            /** CheckGenesis
             *
             *  Check the Genesis Transaction of this Trust Key.
             *
             *  @param[in] block The block containing the Genesis Coinstake transaction for this Trust Key
             *
             *  @return true if block contains valid Genesis transaction for this Trust Key, false otherwise
             *
             */
            bool CheckGenesis(const TritiumBlock block) const;


            /** ToString
             *
             *  Generate a string representation of this Trust Key.
             *
             *  @return data for this Trust Key as string
             *
             */
            std::string ToString();


            /** Print
             *
             *  Print a string representation of this Trust Key to the debug log.
             *
             */
            void Print();

        };
    }
}

#endif
