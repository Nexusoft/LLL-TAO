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

        namespace OP
        {
            /** Primitive Operations. **/
            enum
            {
                //used for pattern matching
                WILDCARD   = 0x00,

                //register operations
                WRITE      = 0x01,
                CREATE     = 0x02,
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

                FEE        = 0x17, //to pay fees to network

                //internal funding
                AMBASSADOR = 0x20,
                DEVELOPER  = 0x21,

                //consensus operations
                ACK        = 0x30, //a vote to credit trust towards a proposal
                NACK       = 0x31, //a vote to withdrawl trust from a proposal

                //conditional operations
                VALIDATE   = 0x40,
                CONDITION  = 0x41,

                //legacy operations
                LEGACY     = 0x50,
                MIGRATE    = 0x51,

                //0x31 = 0x6f RESERVED

                //final reserved ENUM
                RESERVED   = 0xff
            };


            /** Core validation types. **/
            namespace TYPES
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
            }


            /** Core validation operations. **/
            enum
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
            namespace REGISTER
            {
                enum
                {
                    CREATED       = 0xb0,
                    MODIFIED      = 0xb1,
                    OWNER         = 0xb2,
                    TYPE          = 0xb3,
                    STATE         = 0xb4,

                    //object registers
                    VALUE         = 0xb5
                };
            }


            /** Caller Values (The conditional script caller). **/
            namespace CALLER
            {
                enum
                {
                    GENESIS      = 0xc0,
                    TIMESTAMP    = 0xc1,
                    OPERATIONS   = 0xc2,
                    CONDITIONS   = 0xc3,
                };

                /** Register pre-state values. **/
                namespace PRESTATE
                {
                    enum
                    {
                        CREATED       = 0xc4,
                        MODIFIED      = 0xc5,
                        OWNER         = 0xc6,
                        TYPE          = 0xc7,
                        STATE         = 0xc8,

                        //object registers
                        VALUE         = 0xc9
                    };
                }
            }


            /** Source contract values. **/
            namespace CONTRACT
            {
                enum
                {
                    GENESIS      = 0xca,
                    TIMESTAMP    = 0xcb,
                    OPERATIONS   = 0xcc,
                    CONDITIONS   = 0xcd,
                };
            }


            /* Ledger Layer State Values. */
            namespace LEDGER
            {
                enum
                {
                    HEIGHT        = 0xd0,
                    SUPPLY        = 0xd1,
                    TIMESTAMP     = 0xd2
                };
            }



            /* Cryptographic operations. */
            namespace CRYPTO
            {
                enum
                {
                    SK256        = 0xe0,
                    SK512        = 0xe1
                };
            }

        }


        /** Transfer operation enumeration. **/
        namespace TRANSFER
        {
            enum
            {
                /* Transfer requiring claim. */
                CLAIM = 0xfa,

                /* Forced Transfer. */
                FORCE = 0xfb

                //TODO: possibly sanitize transfer to USER or ASSET
            };
        }

        /* Warnings namespace */
        namespace WARNINGS
        {
            enum
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
            };
        }
    }
}

#endif
