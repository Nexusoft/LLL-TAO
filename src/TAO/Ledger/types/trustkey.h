/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_TRUSTKEY_H
#define NEXUS_TAO_LEDGER_INCLUDE_TRUSTKEY_H

#include <vector>

#include <LLC/hash/macro.h>
#include <LLC/hash/SK.h>
#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

namespace Legacy
{
    class LegacyBlock;
}

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        class BlockState;

        /** TrustKey
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
            uint32_t nVersion;


            /** Hash of legacy block containing the Genesis transaction for this Trust Key **/
            uint1024_t hashGenesisBlock;


            /** Hash of the Genesis transaction for this Trust Key **/
            uint512_t hashGenesisTx;


            /** Timestamp of the Genesis transaction for this Trust Key **/
            uint32_t nGenesisTime;


            /** The last block that was found by this key. */
            uint1024_t hashLastBlock;


            /** Constructor
             *
             *  Initializes a null Trust Key.
             *
             **/
            TrustKey()
            : vchPubKey()
            , nVersion(1)
            , hashGenesisBlock(0)
            , hashGenesisTx(0)
            , nGenesisTime(0)
            , hashLastBlock(0)
            {

            }


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
            TrustKey(const std::vector<uint8_t> vchPubKeyIn, const uint1024_t hashBlockIn, const uint512_t hashTxIn, const int32_t nTimeIn)
            : vchPubKey(vchPubKeyIn)
            , nVersion(1)
            , hashGenesisBlock(hashBlockIn)
            , hashGenesisTx(hashTxIn)
            , nGenesisTime(nTimeIn)
            , hashLastBlock(0)
            {

            }


            /* Define Serialization/Deserialization for Trust Key */
            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(vchPubKey);
                READWRITE(hashGenesisBlock);
                READWRITE(hashGenesisTx);
                READWRITE(nGenesisTime);
                READWRITE(hashLastBlock);
            )


            /** SetNull
             *
             *  Set the Trust Key data to Null (uninitialized) values.
             *
             **/
            void SetNull();


            /** Age
             *
             *  Retrieve how old the Trust Key is at a given point in time.
             *
             *  @param[in] nTime The timestamp of the end time for determining age
             *
             *  @return Elapsed time between Trust Key Genesis timestamp and the requested nTime, or 0 if nTime < nGenesisTime
             *
             **/
            uint64_t Age(const uint64_t nTime) const;


            /** BlockAge
             *
             *  Retrieve the time since this Trust Key last generated a Proof of Stake block.
             *
             *  @param[in] state The block state to search from
             *
             *  @return Elapsed time between last block genereated and the requested nTime
             *
             **/
            uint64_t BlockAge(const TAO::Ledger::BlockState& state) const;


            /** GetHash
             *
             *  Retrieve hash of a Trust Key to Verify the Key's Root.
             *
             *  @return Hash of Public Key associated with this Trust Key
             *
             **/
            inline uint512_t GetHash() const { return LLC::SK512(vchPubKey, BEGIN(hashGenesisBlock), END(nGenesisTime)); }


            /** Expired
             *
             *  Determine if a key is expired at a given point in time.
             *  Expiration only applies to nVersion=4 or earlier trust keys, though this method may be called for any.
             *
             *  @param[in] state The block state to search from
             *
             *  @return true if Trust Key expired, false otherwise
             *
             */
            bool Expired(const TAO::Ledger::BlockState& state) const;


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
            bool CheckGenesis(const TAO::Ledger::BlockState& block) const;


            /** StakeRate
             *
             *  Retrieves the staking rate (ie, minting rate or interest rate) of the trust key for a given PoS legacy block.
             *
             *  For version v5+ blocks, the trust score is extracted from the block and used
             *  to calculate stake rate. For v4 blocks, the difference between nTime
             *  and the nGenesisTime of the trust key is used (age of trust key). 
             *
             *  @param[in] block The block to check against. Does not need to be available in LLD, can be new candidate block.
             *  @param[in] nTime The time to check against. Ignored for v5+ blocks
             *
             *  @return the stake rate of the trust key at the time the block was generated
             *
             **/
            double StakeRate(const Legacy::LegacyBlock& block, uint32_t nTime) const;


            /** StakeRate
             *
             *  Retrieves the staking rate (ie, minting rate or interest rate) of the trust key for a given PoS block state.
             *
             *  For version v5+ blocks, the trust score is extracted from the block and used
             *  to calculate stake rate. For v4 blocks, the difference between nTime
             *  and the nGenesisTime of the trust key is used (age of trust key). 
             *
             *  @param[in] block The block state to check against. Must be previously connected block stored in LLD.
             *  @param[in] nTime The time to check against. Ignored for v5+ blocks
             *
             *  @return the stake rate of the trust key at the time the block was generated
             *
             **/
            double StakeRate(const TAO::Ledger::BlockState& block, uint32_t nTime) const;


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
