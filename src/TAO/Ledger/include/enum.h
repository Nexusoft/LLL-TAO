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

#include <Util/include/args.h>

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
                /** A system genesis is pre-pended with byte 0x00. SYSTEM genesis cannot be made by any users, only the system. **/
                SYSTEM      = 0x00,
            };


            /* Main Network Genesis types available. */
            namespace MAINNET
            {
                enum
                {
                    /** a mainnet genesis has to be pre-pended with byte 0xa2. **/
                    USER        = 0xa1,

                    /** An owner genesis is a sigchain that owns a hybrid network. **/
                    OWNER       = 0xa2,
                };


                /* Hybrid Network Genesis types available. */
                namespace HYBRID
                {
                    enum
                    {
                        /** A mainnet genesis has to be pre-pended with byte 0xa2. **/
                        USER        = 0xa3,
                    };
                }


                /* Sister Network Genesis types available. */
                namespace SISTER
                {
                    enum
                    {
                        /** a mainnet genesis has to be pre-pended with byte 0xa2. **/
                        USER        = 0xa4,
                    };
                }
            }


            /* Test Network Genesis types available. */
            namespace TESTNET
            {
                enum
                {
                    /** a testnet genesis has to be pre-pended with byte 0xb1. **/
                    USER        = 0xb1,

                    /** An owner genesis is a sigchain that owns a hybrid network. **/
                    OWNER       = 0xb2,
                };


                /* Hybrid Network Genesis types available. */
                namespace HYBRID
                {
                    enum
                    {
                        /** a testnet hybrid has to be pre-pended with byte 0xb3. **/
                        USER        = 0xb3,
                    };
                }


                /* Sister Network Genesis types available. */
                namespace SISTER
                {
                    enum
                    {
                        /** a testnet sister has to be pre-pended with byte 0xb4. **/
                        USER        = 0xb4,
                    };
                }
            }


            /** UserType
             *
             *  Method to handle switching the genesis leading byte for the network user sigchains.
             *
             **/
            inline uint8_t UserType()
            {
                /* Check if we are in hybrid mode. */
                if(config::fHybrid.load())
                    return (config::fTestNet.load() ? uint8_t(GENESIS::TESTNET::HYBRID::USER) : uint8_t(GENESIS::MAINNET::HYBRID::USER));

                /* Check if we are in sister mode. */
                if(config::fSister.load())
                    return (config::fTestNet.load() ? uint8_t(GENESIS::TESTNET::SISTER::USER) : uint8_t(GENESIS::MAINNET::SISTER::USER));

                /* Regular users for the mother network. */
                return (config::fTestNet.load() ? uint8_t(GENESIS::TESTNET::USER) : uint8_t(GENESIS::MAINNET::USER));
            }


            /** OwnerType
             *
             *  Method to handle switching the genesis leading byte for network owner sigchains. Only available for mother network.
             *
             **/
            inline uint8_t OwnerType()
            {
                return (config::fTestNet.load() ? uint8_t(GENESIS::TESTNET::OWNER) : uint8_t(GENESIS::MAINNET::OWNER));
            }
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

                /* Wipe the memory states when writing. */
                ERASE       = 0x05,

                /* Trigger remote lookups for -client mode. */
                LOOKUP      = 0x06,
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
