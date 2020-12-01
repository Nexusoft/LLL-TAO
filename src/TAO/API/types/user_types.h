/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** FLAGS
         *
         *  User-defined state register types (non object registers) allowing the data in RAW/APPEND/READONLY registers
         *  to be identified by the leading byte in the register binary data.
         *
         **/ 
        enum USER_TYPES
        {
            SUPPLY      = 1,
            INVOICE     = 2
        };


    }// end API namespace

}// end TAO namespace