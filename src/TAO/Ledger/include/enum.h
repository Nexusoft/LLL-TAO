/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_ENUM_H
#define NEXUS_TAO_LEDGER_INCLUDE_ENUM_H

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Enumeration for transaction types. **/
        enum
        {
            /** Reserved. **/
            RESERVED = 0x00,

            /** Tritium TX. **/
            TRITIUM  = 0x01,

            /** Legacy TX. **/
            LEGACY   = 0x02
        };


        /** State values for a transaction. **/
        namespace STATE
        {
            enum
            {
                /** A transaction is unconfirmed if not connected to chain. **/
                UNCONFIRMED = 0x00,

                /** A transaction is at head if it is the last transaction. **/
                HEAD        = 0x01

            };
        }


        /** Type values for a genesis. These are very important for security to tell the difference from register hashes. **/
        namespace GENESIS
        {
            enum
            {
                /** A mainnet genesis has to be pre-pended with byte 0xa1 **/
                MAINNET     = 0xa1,

                /** a testnet genesis has to be pre-pended with byte 0xa2. **/
                TESTNET     = 0xa2,
            };
        }


        /** Signature types for sigchain. **/
        namespace SIGNATURE
        {
            enum
            {
                /** Reserved. **/
                RESERVED    = 0x00,

                /** FALCON signature scheme. **/
                FALCON      = 0x01,

                /** BRAINPOOL ECDSA curve. **/
                BRAINPOOL   = 0x02,

                /** SECP256K1 ECDSA curve. **/
                SECP256K1   = 0x03
            };
        }


        /** State values for a transaction. **/
        namespace CHANNEL
        {
            enum
            {
                /** Proof of stake channel. **/
                STAKE   = 0x00,

                /** Prime channel. **/
                PRIME   = 0x01,

                /** Hash channel. **/
                HASH    = 0x02,

                /** Private channel. **/
                PRIVATE = 0x03,
            };
        }


        /** FLAGS
         *
         *  The flags on what to do to registers when executing operations.
         *
         **/
        namespace FLAGS
        {
            enum
            {
                /** Reserved. **/
                RESERVED    = 0x00,

                /* Write data to disk on block. */
                BLOCK       = 0x01,

                /* Write data into memory on mempool. */
                MEMPOOL     = 0x02,

                /* Don't write data when calculating fees. */
                FEES        = 0x03,

                /* Special memory when building block. */
                MINER       = 0x04,
            };
        }



        /** TRANSACTION
         *
         *  The type of transaction being put into the block's vtx
         *
         **/
        namespace TRANSACTION
        {
            enum
            {
                /* Legacy transaction. */
                LEGACY     = 0x00,

                /* Tritium Transaction. */
                TRITIUM    = 0x01,

                /* Private hybrid hash. */
                CHECKPOINT = 0x02, //for private chain checkpointing into mainnet blocks.
            };
        }
    }
}

#endif
