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
            TRITIUM = 0x00,
            LEGACY  = 0x01
        };


        /** State values for a transaction. **/
        struct STATE
        {
            enum
            {
                /** A transaction is unconfirmed if not connected to chain. **/
                UNCONFIRMED = 0x00,

                /** A transaction is at head if it is the last transaction. **/
                HEAD        = 0x01

            };
        };


        /** Signature types for sigchain. **/
        struct SIGNATURE
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
                /** Reserved. **/
                RESERVED    = 0x00,

                /* Write data to disk on block. */
                BLOCK       = 0x01,

                /* Write data into memory on mempool. */
                MEMPOOL     = 0x02
            };
        };
    }
}

#endif
