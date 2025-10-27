/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

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

                /** ED448-Goldilocks EdDSA curve. **/
                ED448       = 0x03
            };

            /* Track a mapping to enum. */
            static inline uint8_t TYPE(const std::string& strType)
            {
                /* Check for brainpool standard. */
                if(strType == "brainpool")
                    return SIGNATURE::BRAINPOOL;

                /* Check for falcon standard. */
                if(strType == "falcon")
                    return SIGNATURE::FALCON;

                /* Check for brainpool standard. */
                if(strType == "ed448")
                    return SIGNATURE::ED448;

                return SIGNATURE::RESERVED;
            }
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
                LOOKUP      = 0x06,

                /* Force a remote lookup for -client mode. */
                FORCED      = 0x07,

                /* Write data into memory on sanitize. */
                SANITIZE    = 0x08,
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


        /** Struct to hold the authorization enumerated values for building an authorization script. **/
        struct AUTH
        {
            /** Core validation types. **/
            struct TYPES
            {
                enum : uint8_t
                {
                    //RESERVED to 0x7f
                    UINT8     = 0x01,
                    UINT16    = 0x02,
                    UINT32    = 0x03,
                    UINT64    = 0x04,
                    UINT256   = 0x05,
                    UINT512   = 0x06,
                    UINT1024  = 0x07,
                    STRING    = 0x08,
                    BYTES     = 0x09,

                    //nexthash types
                    NEXTHASH  = 0x0a,
                    RECOVERY  = 0x0b,

                    //signature types
                    FALCON    = 0x0c, //FALCON QR DSA
                    ED448     = 0x0d, //ED448-Goldilocks

                    //keys and signatures
                    PUBKEY    = 0x0e,
                    SIGNATURE = 0x0f
                };
            };


            /** Core comparison operations. **/
            struct OP
            {
                enum : uint8_t
                {
                    //RESERVED from 0x10 to 0x1f
                    EQUALS        = 0x10,
                    LESSTHAN      = 0x11,
                    GREATERTHAN   = 0x12,
                    NOTEQUALS     = 0x13,
                    CONTAINS      = 0x14,
                    LESSEQUALS    = 0x15,
                    GREATEREQUALS = 0x16,

                    AND         = 0x17,
                    OR          = 0x18,
                    GROUP       = 0x19,
                    UNGROUP     = 0x1a,

                    //RESERVED 0x 20 to to 0x2f
                    ADD         = 0x20,
                    SUB         = 0x21,
                    DIV         = 0x22,
                    MUL         = 0x23,
                    MOD         = 0x24,
                    INC         = 0x25,
                    DEC         = 0x26,
                    EXP         = 0x27,

                    SUBDATA     = 0x28,
                    CAT         = 0x29,
                };
            };


            /** Authorization Hashes operations. **/
            struct CRYPTO
            {
                enum : uint8_t
                {
                    SK256  = 0x30, //Skein-Keccak 256 by default
                    SHA3   = 0x31, //SHA3 256-bit by default
                };
            }

            /** Primitive OP::WRITE operation. **/
            struct WRITE
            {
                static const uint16_t ENABLED = (1 << 1);

                /** Input parameters for OP::WRITE. **/
                enum : uint8_t
                {
                    ADDRESS = 0x40,
                    DATA    = 0x41,
                };
            };

            /** Primitive OP::CREATE operation. **/
            struct CREATE
            {
                static const uint16_t ENABLED = (1 << 2);

                /** Input parameters for OP::CREATE. **/
                enum : uint8_t
                {
                    ADDRESS = 0x42,
                    TYPE    = 0x43,
                    DATA    = 0x44,
                };
            };

            /** Primitive OP::TRANSFER operation. **/
            struct TRANSFER
            {
                static const uint16_t ENABLED = (1 << 3);

                /** Input parameters for OP::TRANSFER. **/
                enum : uint8_t
                {
                    ADDRESS   = 0x43,
                    RECIPIENT = 0x44,
                    TYPE      = 0x45,
                };
            };

            /** Primitive OP::CLAIM operation. **/
            struct CLAIM
            {
                static const uint16_t ENABLED = (1 << 4);

                /** Input parameters for OP::CLAIM. **/
                enum : uint8_t
                {
                    TRANSACTION  = 0x46, //the transaction that we are claiming from
                    ADDRESS      = 0x47,
                };
            };

            /** Primitive OP::COINBASE operation. **/
            struct COINBASE
            {
                static const uint16_t ENABLED = (1 << 5);

                /** Input parameters for OP::COINBASE. **/
                enum : uint8_t
                {
                    GENESIS      = 0x47,
                };
            };

            /** Primitive OP::TRUST operation. **/
            struct TRUST
            {
                static const uint16_t ENABLED = (1 << 6);

                /** Input parameters for OP::COINBASE. **/
                enum : uint8_t
                {
                    TRUST  = 0x48,
                    CHANGE = 0x49,
                    REWARD = 0x4a,
                };
            };
        };
    }
}
