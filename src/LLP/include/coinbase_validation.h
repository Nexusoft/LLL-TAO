/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Ledger/types/tritium.h>
#include <TAO/Operation/types/contract.h>
#include <TAO/Operation/include/enum.h>

#include <cstdint>


namespace LLP
{
    /** CoinbaseValidation
     *
     *  Shared helpers used by BOTH mining lanes (Legacy port 8323 in miner.cpp and
     *  Stateless port 9323 in stateless_miner_connection.cpp) to validate the
     *  producer's coinbase contract stream layout before deep-stack acceptance.
     *
     *  This is the first piece of the "Shared Helpers Lane Unification" — the same
     *  malformed-coinbase detection logic was previously duplicated in both lanes,
     *  with only the rejection-packet emission differing per framing.
     *
     *  See docs/BURST_BLOCK_COINBASE_PRIMITIVE_OVERFLOW.md for the underlying bug.
     **/
    namespace CoinbaseValidation
    {
        /** Expected size of every well-formed OP::COINBASE contract stream:
         *      1 (opcode) + 32 (hashGenesis) + 8 (nCredit) + 8 (nExtraNonce) = 49 bytes.
         **/
        static constexpr uint64_t COINBASE_STREAM_SIZE = 49;


        /** Result of scanning a block's producer for malformed coinbase contracts. **/
        struct MalformedCoinbase
        {
            /** True if at least one malformed coinbase contract was detected. **/
            bool malformed = false;

            /** Index of the first malformed contract slot in the producer. **/
            uint32_t contract_index = 0;

            /** Actual byte size of the malformed contract's operation stream. **/
            uint64_t actual_size = 0;
        };


        /** DetectMalformedCoinbase
         *
         *  Scan the block's producer transaction for any non-empty contract whose
         *  primitive is OP::COINBASE but whose serialized operation stream length is
         *  not exactly COINBASE_STREAM_SIZE.  Returns the first malformed slot found.
         *
         *  This catches the cache-hit + producer-finalization residue bug fixed in
         *  CreateProducer (defense in depth), as well as any future regression that
         *  emits a partial or duplicated coinbase stream.
         *
         *  @param[in] block  The fully assembled solved block to inspect.
         *
         *  @return MalformedCoinbase{malformed=true, ...} on the first violating
         *          slot; otherwise MalformedCoinbase{} (malformed=false).
         **/
        inline MalformedCoinbase DetectMalformedCoinbase(const TAO::Ledger::TritiumBlock& block)
        {
            const uint32_t nProducerContracts = block.producer.Size();
            for(uint32_t nContract = 0; nContract < nProducerContracts; ++nContract)
            {
                const TAO::Operation::Contract& rContract = block.producer[nContract];
                if(rContract.Empty())
                    continue;

                if(rContract.Primitive() == TAO::Operation::OP::COINBASE
                && rContract.Operations().size() != COINBASE_STREAM_SIZE)
                {
                    MalformedCoinbase result;
                    result.malformed      = true;
                    result.contract_index = nContract;
                    result.actual_size    = rContract.Operations().size();
                    return result;
                }
            }

            return MalformedCoinbase{};
        }
    }
}
