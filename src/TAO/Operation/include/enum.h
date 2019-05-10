/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_ENUM_H
#define NEXUS_TAO_OPERATION_INCLUDE_ENUM_H

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        //NETF - ADS - Application Development Standard - Document to define new applicaqtion programming interface calls required by developers
        //NETF - NOS - Nexus Operation Standard - Document to define operation needs, formal design, and byte slot, and NETF engineers to develop the CASE statement
        //NETF - ORS - Object Register Standard - Document to define a specific object register for purpose of ADS standards, with NOS standards being capable of supporting methods


        struct OP
        {
            /** Primitive Operations. **/
            enum
            {
                //used for pattern matching
                WILDCARD   = 0x00,

                //register operations
                WRITE      = 0x01,
                REGISTER   = 0x02,
                AUTHORIZE  = 0x03, //to be determined how this will work
                TRANSFER   = 0x04,
                CLAIM      = 0x05,
                APPEND     = 0x06,

                //financial operations
                DEBIT      = 0x10,
                CREDIT     = 0x11,
                COINBASE   = 0x12,
                GENESIS    = 0x13, //for proof of stake
                TRUST      = 0x14, //for proof of stake
                LOCK       = 0x15, //to lock funds in stake
                UNLOCK     = 0x16, //to unlock funds from stake

                //internal funding
                AMBASSADOR = 0x20,
                DEVELOPER  = 0x21,

                //consensus operations
                ACK        = 0x30, //a vote to credit trust towards a proposal
                NACK       = 0x31, //a vote to withdrawl trust from a proposal

                //conditional operations
                VALIDATE    = 0x40,
                REQUIRE     = 0x41,

                //0x31 = 0x6f RESERVED
            };


            /** Core validation types. **/
            struct TYPES
            {
                enum
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
            enum
            {
                //RESERVED to 0x8f
                EQUALS      = 0x80,
                LESSTHAN    = 0x81,
                GREATERTHAN = 0x82,
                NOTEQUALS   = 0x83,
                CONTAINS    = 0x84,


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


                //RESERVED to 0x2f
                AND         = 0xa0,
                OR          = 0xa1,
                IF          = 0xa2,
            };


            /** Register layer state values. **/
            struct REGISTER
            {
                enum
                {
                    TIMESTAMP     = 0xb0,
                    OWNER         = 0xb1,
                    TYPE          = 0xb2,
                    STATE         = 0xb3,

                    //object registers
                    VALUE         = 0xb4
                };
            };


            /** Caller Values (The validation script caller). **/
            struct CALLER
            {
                enum
                {
                    GENESIS      = 0xc0,
                    TIMESTAMP    = 0xc1,
                    OPERATIONS   = 0xc2,
                };
            };


            /* Ledger Layer State Values. */
            struct LEDGER
            {
                enum
                {
                    HEIGHT        = 0xd0,
                    SUPPLY        = 0xd1,
                    TIMESTAMP     = 0xd2
                };
            };



            /* Cryptographic operations. */
            struct CRYPTO
            {
                enum
                {
                    SK256        = 0xe0,
                    SK512        = 0xe1
                };
            };


            /* Global state values. */
            struct GLOBAL
            {
                enum
                {
                    UNIFIED       = 0xf0
                };
            };
        };
    }
}

#endif
