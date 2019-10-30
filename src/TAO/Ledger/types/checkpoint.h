/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/
#pragma once

#include <LLC/types/typedef.h>
#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {
        class Checkpoint
        {
        public:
            
            /* The version of this checkpoint */
            uint32_t nVersion;

            /* The hash being checkpointed */
            uint512_t hashCheckpoint;

            /* Time the checkpoint was created */
            uint64_t nTimestamp;

            /* Nonce proving the PoW required to store this checkpoint on chain */
            uint64_t nNonce;


            /** The default constructor. **/
            Checkpoint();


            /** Copy constructor. **/
            Checkpoint(const Checkpoint& checkpoint);


            /** Move constructor. **/
            Checkpoint(Checkpoint&& checkpoint) noexcept;


            /** Copy assignment. **/
            Checkpoint& operator=(const Checkpoint& checkpoint);


            /** Move assignment. **/
            Checkpoint& operator=(Checkpoint&& checkpoint) noexcept;


            /** Default Destructor **/
            virtual ~Checkpoint();

            /** ProofHash
             *
             *  Get the Proof Hash of the checkpoint. Used to verify the of proof work .
             *
             *  @return Returns a 512-bit proof hash.
             *
             **/
            uint512_t ProofHash() const;


            /** GetHash
             *
             *  Get the Hash of the checkpoint.
             *
             *  @return Returns a 512-bit block hash.
             *
             **/
            uint512_t GetHash() const;

        };

    } /* End Ledger namespace */

} /* End TAO namespace */