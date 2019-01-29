/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_ENUM_H
#define NEXUS_TAO_REGISTER_INCLUDE_ENUM_H

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** OBJECT
         *
         *  Object registers that are available for use.
         *
         **/
        struct OBJECT
        {
            enum
            {
                /* RESERVED can't be used as an object. */
                RESERVED   = 0x00,

                /* This type of register cannot have the data changed */
                READONLY = 0x01,

                /* This type of register can only be appended to. */
                APPEND   = 0x02,

                /* This type of register is just raw data that can be changed. */
                RAW      = 0x03,

                /* This type of register handles general accounts and DEBITS / CREDITS */
                ACCOUNT  = 0x04,

                /* This type of register to hold token parameters. */
                TOKEN    = 0x05
            };
        };


        /** STATES
         *
         *  The states for the register script in transaction.
         *
         **/
        struct STATES
        {
            enum
            {
                /* RESERVED can't be used as a state. */
                RESERVED   = 0x00,

                /* Pre-State - for marking register as a pre-state in register scripts. */
                PRESTATE   = 0x01,

                /* Post-State - for recording the checksum of register post-state. */
                POSTSTATE  = 0x02,

                /* Validation - for recording events in validation registers. */
                VALIDATION = 0x03
            };
        };


        /** FLAGS
         *
         *  The flags on what to do to registers when executing operations.
         *
         **/
        struct FLAGS
        {
            enum
            {
                /* Calculate the register's pre-states. */
                PRESTATE  = (1 << 1),

                /* Calculate the register post-state checksum. */
                POSTSTATE = (1 << 2),

                /* Write the registers post-state to database. */
                WRITE     = (1 << 3),

                /* Write the states into a mempool for checking pre-block. */
                MEMPOOL   = (1 << 4)
            };
        };
    }
}

#endif
