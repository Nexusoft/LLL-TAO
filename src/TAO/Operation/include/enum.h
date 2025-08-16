/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <inttypes.h>

#include <map>

/* Global TAO namespace. */
namespace TAO::Operation
{
    //NETF - ADS - Application Development Standard - Document to define new applicaqtion programming interface calls required by developers
    //NETF - NOS - Nexus Operation Standard - Document to define operation needs, formal design, and byte slot, and NETF engineers to develop the CASE statement
    //NETF - ORS - Object Register Standard - Document to define a specific object register for purpose of ADS standards, with NOS standards being capable of supporting methods

    struct OP
    {
        /** Primitive Operations. **/
        enum : uint8_t
        {
            //used for pattern matching
            WILDCARD    = 0x00,

            //register operations
            WRITE       = 0x01,
            CREATE      = 0x02,
            AUTHORIZE   = 0x03, //to be determined how this will work
            TRANSFER    = 0x04,
            CLAIM       = 0x05,
            APPEND      = 0x06,

            //financial operations
            DEBIT       = 0x10,
            CREDIT      = 0x11,
            COINBASE    = 0x12,
            GENESIS     = 0x13, //for proof of stake
            TRUST       = 0x14,
            FEE         = 0x17, //to pay fees to network

            //consensus operations
            ACK         = 0x30, //a vote to credit trust towards a proposal
            NACK        = 0x31, //a vote to withdrawl trust from a proposal

            //conditional operations
            VALIDATE    = 0x40,
            CONDITION   = 0x41,

            //legacy operations
            LEGACY      = 0x50,
            MIGRATE     = 0x51,

            //0x31 = 0x6f RESERVED

            //final reserved ENUM
            RESERVED    = 0xff
        };


        /** Core validation types. **/
        struct TYPES
        {
            enum : uint8_t
            {
                //RESERVED to 0x7f
                UINT8_T     = 0x70,
                UINT16_T    = 0x71,
                UINT32_T    = 0x72,
                UINT64_T    = 0x73,
                UINT256_T   = 0x74,
                UINT512_T   = 0x75,
                UINT1024_T  = 0x76,
                STRING      = 0x77,
                BYTES       = 0x78,
            };
        };


        /** Core validation operations. **/
        enum : uint8_t
        {
            //RESERVED to 0x8f
            EQUALS        = 0x80,
            LESSTHAN      = 0x81,
            GREATERTHAN   = 0x82,
            NOTEQUALS     = 0x83,
            CONTAINS      = 0x84,
            LESSEQUALS    = 0x85,
            GREATEREQUALS = 0x86,


            //RESERVED to 0x9f
            ADD         = 0x90,
            SUB         = 0x91,
            DIV         = 0x92,
            MUL         = 0x93,
            MOD         = 0x94,
            INC         = 0x95,
            DEC         = 0x96,
            EXP         = 0x97,

            SUBDATA     = 0x98,
            CAT         = 0x99,


            //RESERVED to 0x2f
            AND         = 0xa0,
            OR          = 0xa1,
            GROUP       = 0xa2,
            UNGROUP     = 0xa3,
        };


        /** Register layer state values. **/
        struct REGISTER
        {
            enum : uint8_t
            {
                CREATED       = 0xb0,
                MODIFIED      = 0xb1,
                OWNER         = 0xb2,
                TYPE          = 0xb3,
                STATE         = 0xb4,

                //object registers
                VALUE         = 0xb5
            };
        };


        /** Caller Values (The conditional script caller). **/
        struct CALLER
        {
            enum : uint8_t
            {
                GENESIS      = 0xc0,
                TIMESTAMP    = 0xc1,
                OPERATIONS   = 0xc2,
                CONDITIONS   = 0xc3,
            };

            /** Register pre-state values. **/
            struct PRESTATE
            {
                enum : uint8_t
                {
                    CREATED       = 0xc4,
                    MODIFIED      = 0xc5,
                    OWNER         = 0xc6,
                    TYPE          = 0xc7,
                    STATE         = 0xc8,

                    //object registers
                    VALUE         = 0xc9
                };
            };
        };


        /** Source contract values. **/
        struct CONTRACT
        {
            enum : uint8_t
            {
                GENESIS      = 0xca,
                TIMESTAMP    = 0xcb,
                OPERATIONS   = 0xcc,
                CONDITIONS   = 0xcd,
            };
        };


        /* Ledger Layer State Values. */
        struct LEDGER
        {
            enum : uint8_t
            {
                HEIGHT        = 0xd0,
                SUPPLY        = 0xd1,
                TIMESTAMP     = 0xd2
            };
        };


        /* Cryptographic operations. */
        struct CRYPTO
        {
            enum : uint8_t
            {
                SK256        = 0xe0,
                SK512        = 0xe1
            };
        };
    };


    /** Map to hold opcodes that are activated. **/
    const std::map<uint8_t, uint32_t> mapActivated =
    {
        //activated datatypes
        { OP::TYPES::UINT8_T,             1 },
        { OP::TYPES::UINT16_T,            1 },
        { OP::TYPES::UINT32_T,            1 },
        { OP::TYPES::UINT64_T,            1 },
        { OP::TYPES::UINT256_T,           1 },
        { OP::TYPES::UINT512_T,           1 },
        { OP::TYPES::UINT1024_T,          1 },
        { OP::TYPES::STRING,              1 },
        { OP::TYPES::BYTES,               1 },

        //comparison operators
        { OP::EQUALS,                     1 },
        { OP::LESSTHAN,                   1 },
        { OP::GREATERTHAN,                1 },
        { OP::NOTEQUALS,                  1 },
        { OP::CONTAINS,                   1 },
        { OP::LESSEQUALS,                 1 },
        { OP::GREATEREQUALS,              1 },

        //arithmatic operators
        { OP::ADD,                        1 },
        { OP::SUB,                        1 },
        { OP::DIV,                        1 },
        { OP::MUL,                        1 },
        { OP::MOD,                        1 },
        { OP::INC,                        1 },
        { OP::DEC,                        1 },
        { OP::EXP,                        1 },

        //parsing operators
        { OP::SUBDATA,                    1 },
        { OP::CAT,                        1 },

        //logical grouping operators
        { OP::AND,                        1 },
        { OP::OR,                         1 },
        { OP::GROUP,                      1 },
        { OP::UNGROUP,                    1 },

        //register operators
        { OP::REGISTER::CREATED,          1 },
        { OP::REGISTER::MODIFIED,         1 },
        { OP::REGISTER::OWNER,            1 },
        { OP::REGISTER::TYPE,             1 },
        { OP::REGISTER::STATE,            1 },
        { OP::REGISTER::VALUE,            1 },

        //contract that is calling
        { OP::CALLER::GENESIS,            1 },
        { OP::CALLER::TIMESTAMP,          1 },
        { OP::CALLER::OPERATIONS,         1 },
        { OP::CALLER::CONDITIONS,         1 },
        { OP::CALLER::PRESTATE::CREATED,  1 },
        { OP::CALLER::PRESTATE::MODIFIED, 1 },
        { OP::CALLER::PRESTATE::OWNER,    1 },
        { OP::CALLER::PRESTATE::TYPE,     1 },
        { OP::CALLER::PRESTATE::STATE,    1 },
        { OP::CALLER::PRESTATE::VALUE,    1 },

        //contract being invoked
        { OP::CONTRACT::GENESIS,          1 },
        { OP::CONTRACT::TIMESTAMP,        1 },
        { OP::CONTRACT::OPERATIONS,       1 },
        { OP::CONTRACT::CONDITIONS,       1 },

        //ledger statistics operators
        { OP::LEDGER::HEIGHT,             1 },
        { OP::LEDGER::SUPPLY,             1 },
        { OP::LEDGER::TIMESTAMP,          1 },

        //crypto operators
        { OP::CRYPTO::SK256,              1 },
        { OP::CRYPTO::SK512,              1 },
    };


    /** Map to hold opcodes that need to deactivate. **/
    const std::map<uint8_t, uint32_t> mapDeactivated =
    {
        { OP::LEDGER::HEIGHT,    5 },
        { OP::LEDGER::SUPPLY,    5 },
        { OP::LEDGER::TIMESTAMP, 5 },
    };


    /** PLACEHOLDER
     *
     *  This structure holds enumeration values for parameter placeholders for generating a conditional contract.
     *
     **/
    struct PLACEHOLDER
    {
        /* Standard parameter placeholder enumeration. */
        enum : uint8_t
        {
            //marker of beginning range
            RESERVED1 = 0,

            //supports up to maximum of 9 parameters
            _1       = 1,
            _2       = 2,
            _3       = 3,
            _4       = 4,
            _5       = 5,
            _6       = 6,
            _7       = 7,
            _8       = 8,
            _9       = 9,

            //marker of ending range
            RESERVED2 = 10
        };


        /** Valid
         *
         *  Detect if the operation codee is a valid parameter placeholder.
         *
         *  @param[in] nCode The operation code to check against.
         *
         *  @return true if the operation is a valid placeholder.
         *
         **/
        static bool Valid(const uint8_t nCode)
        {
            return (nCode > RESERVED1 && nCode < RESERVED2);
        }
    };


    /** Transfer operation enumeration. **/
    struct TRANSFER
    {
        enum : uint8_t
        {
            /* Transfer requiring claim. */
            CLAIM = 0xfa,

            /* Forced Transfer. */
            FORCE = 0xfb

            //TODO: possibly sanitize transfer to USER or ASSET
        };
    };


    /* Warnings namespace */
    struct WARNINGS
    {
        enum : uint16_t
        {
            /* No warning */
            NONE   = 0x00,

            /* conditions warnings */
            ADD_OVERFLOW    = (1 << 1),
            SUB_OVERFLOW    = (1 << 2),
            INC_OVERFLOW    = (1 << 3),
            DEC_OVERFLOW    = (1 << 4),
            MUL_OVERFLOW    = (1 << 5),
            EXP_OVERFLOW    = (1 << 6),
            DIV_BY_ZERO     = (1 << 7),
            MOD_DIV_ZERO    = (1 << 8),
            COST_OVERFLOW   = (1 << 9),

            /* allocation failures. */
            BAD_ALLOC       = (1 << 10),
        };
    };
}
