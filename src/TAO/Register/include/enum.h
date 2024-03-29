/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_ENUM_H
#define NEXUS_TAO_REGISTER_INCLUDE_ENUM_H

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** REGISTER
         *
         *  State registers that are available for use.
         *
         **/
        struct REGISTER
        {
            enum : uint8_t
            {
                /* RESERVED can't be used as an object. */
                RESERVED   = 0x00,

                /* This type of register cannot have the data changed */
                READONLY   = 0x01,

                /* This type of register can only be appended to. */
                APPEND     = 0x02,

                /* This type of register is just raw data that can be changed. */
                RAW        = 0x03,

                /* This type of register has its data fields enforced by operations layer. */
                OBJECT     = 0x04,

                /* This type of register is special and for system update only. */
                SYSTEM     = 0x05,
            };

            /** STATE
             *
             *  Helper function to detect if state register.
             *
             *  @param[in] nType The enum type to deduce if state or not.
             *
             **/
            __attribute__((const)) static inline uint8_t STATE(const uint8_t nType)
            {
                return (nType == READONLY || nType == APPEND || nType == RAW);
            }
        };


        /** SYSTEM
         *
         *  System registers with reserved indexes.
         *
         **/
        struct SYSTEM
        {
            enum : uint8_t
            {
                /* RESERVED can't be used as system register. */
                RESERVED  = 0x00,

                /* This type of register is a system register holding global trust. */
                TRUST     = 0x01,

                /* LIMIT is defined as 256 values. */
                LIMIT     = 0xff
            };
        };


        /** OBJECT
         *
         *  Object registers that are available and standardized.
         *
         **/
        struct OBJECTS
        {
            enum : uint8_t
            {
                /* Non-Standard Object (User Defined Type). */
                NONSTANDARD  = 0x11,

                /* Account Object Register. */
                ACCOUNT      = 0x12,

                /* Token Object Register. */
                TOKEN        = 0x13,

                /* Trust Object Register. */
                TRUST        = 0x14,

                /* Name Object Register. */
                NAME         = 0x15,

                /* Namespace Object Register. */
                NAMESPACE    = 0x16,

                /* Crypto Object Register. */
                CRYPTO       = 0x17
            };
        };


        /** Object register data types. **/
        struct TYPES
        {
            enum : uint8_t
            {
                UNSUPPORTED = 0x00,

                //RESERVED to 0x0f
                UINT8_T     = 0x01,
                UINT16_T    = 0x02,
                UINT32_T    = 0x03,
                UINT64_T    = 0x04,
                UINT256_T   = 0x05,
                UINT512_T   = 0x06,
                UINT1024_T  = 0x07,
                STRING      = 0x08,
                BYTES       = 0x09,

                //by default everything is read-only
                MUTABLE     = 0xff
            };
        };


        /** STATES
         *
         *  The states for the register script in transaction.
         *
         **/
        struct STATES
        {
            enum : uint8_t
            {
                /* RESERVED can't be used as a state. */
                RESERVED   = 0x00,

                /* Pre-State - for marking register as a pre-state in register scripts. */
                PRESTATE   = 0x01,

                /* Post-State - for recording the checksum of register post-state. */
                POSTSTATE  = 0x02
            };
        };


        /** NAMESPACE
         *
         *  The NAMESPACE namespace to hold constants for Namespace Object registers
         *
         **/
        namespace NAMESPACE //XXX: because of previous code formats, we want this to be in a struct.
        {
            /* Constant used to indicate a global namespace */
            const std::string GLOBAL = "~GLOBAL~";
        }
    }
}

#endif
