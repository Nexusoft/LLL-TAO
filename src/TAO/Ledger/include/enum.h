/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/args.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Enumeration for transaction types. **/
        enum : uint8_t
        {
            /** Reserved. **/
            RESERVED = 0x00,

            /** Tritium TX. **/
            TRITIUM  = 0x01,

            /** Legacy TX. **/
            LEGACY   = 0x02
        };


        /** State values for a transaction. **/
        struct STATE
        {
            enum : uint8_t
            {
                /** A transaction is unconfirmed if not connected to chain. **/
                UNCONFIRMED = 0x00,

                /** A transaction is at head if it is the last transaction. **/
                HEAD        = 0x01

            };
        };


        /** Type values for a genesis. These are very important for security to tell the difference from register hashes. **/
        struct GENESIS
        {
            enum : uint8_t
            {
                /** A system genesis is pre-pended with byte 0x00. SYSTEM genesis cannot be made by any users, only the system. **/
                SYSTEM      = 0x00,
            };


            /* Main Network Genesis types available. */
            struct MAINNET
            {
                enum : uint8_t
                {
                    /** a mainnet genesis has to be pre-pended with byte 0xa2. **/
                    USER        = 0xa1,

                    /** An owner genesis is a sigchain that owns a hybrid network. **/
                    OWNER       = 0xa2,

                    //placeholder (four new sigchain types unallocated)
                    RESERVED1    = 0xa3,
                    RESERVED2    = 0xa4,
                    RESERVED3    = 0xa5,
                    RESERVED4    = 0xa6,
                };


                /* Hybrid Network Genesis types available. */
                struct HYBRID
                {
                    enum : uint8_t
                    {
                        /** A hybrid genesis has to be pre-pended with byte 0xa7. **/
                        USER         = 0xa7,

                        //placeholder (three new sigchain types unallocated)
                        RESERVED1    = 0xa8,
                        RESERVED2    = 0xa9,
                        RESERVED3    = 0xaa,
                    };
                };


                /* Sister Network Genesis types available. */
                struct SISTER
                {
                    enum : uint8_t
                    {
                        /** a sister genesis has to be pre-pended with byte 0xab. **/
                        USER         = 0xab,

                        //placeholder (three new sigchain types unallocated)
                        RESERVED1    = 0xac,
                        RESERVED2    = 0xad,
                        RESERVED3    = 0xae,
                        RESERVED4    = 0xaf,
                    };
                };
            };


            /* Test Network Genesis types available. */
            struct TESTNET
            {
                enum : uint8_t
                {
                    /** a testnet genesis has to be pre-pended with byte 0xb1. **/
                    USER        = 0xb1,

                    /** An owner genesis is a sigchain that owns a hybrid network. **/
                    OWNER       = 0xb2,

                    //placeholder (four new sigchain types unallocated)
                    RESERVED1    = 0xb3,
                    RESERVED2    = 0xb4,
                    RESERVED3    = 0xb5,
                    RESERVED4    = 0xb6,
                };


                /* Hybrid Network Genesis types available. */
                struct HYBRID
                {
                    enum : uint8_t
                    {
                        /** a testnet hybrid has to be pre-pended with byte 0xb3. **/
                        USER        = 0xb7,

                        //placeholder (three new sigchain types unallocated)
                        RESERVED1    = 0xb8,
                        RESERVED2    = 0xb9,
                        RESERVED3    = 0xba,
                    };
                };


                /* Sister Network Genesis types available. */
                struct SISTER
                {
                    enum : uint8_t
                    {
                        /** a testnet sister has to be pre-pended with byte 0xb4. **/
                        USER         = 0xbb,

                        //placeholder (three new sigchain types unallocated)
                        RESERVED1    = 0xbc,
                        RESERVED2    = 0xbd,
                        RESERVED3    = 0xbe,
                        RESERVED4    = 0xbf,
                    };
                };
            };


            /** UserType
             *
             *  Method to handle switching the genesis leading byte for the network user sigchains.
             *
             **/
            static inline uint8_t UserType()
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
            static inline uint8_t OwnerType()
            {
                return (config::fTestNet.load() ? uint8_t(GENESIS::TESTNET::OWNER) : uint8_t(GENESIS::MAINNET::OWNER));
            }
        };


        /** Signature types for sigchain. **/
        struct SIGNATURE
        {
            enum : uint8_t
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
        };


        /** State values for a transaction. **/
        struct CHANNEL
        {
            enum : uint8_t
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
        };


        /** FLAGS
         *
         *  The flags on what to do to registers when executing operations.
         *
         **/
        struct FLAGS
        {
            enum : uint8_t
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
                LOOKUP      = 0x06, //XXX: we may want to automatically handle this
            };
        };


        /** TRANSACTION
         *
         *  The type of transaction being put into the block's vtx
         *
         **/
        struct TRANSACTION
        {
            enum : uint8_t
            {
                /* Legacy transaction. */
                LEGACY     = 0x00,

                /* Tritium Transaction. */
                TRITIUM    = 0x01,

                /* Private hybrid hash. */
                CHECKPOINT = 0x02, //for private chain checkpointing into mainnet blocks.
            };
        };
    }
}
