/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Simple class to encapsulate a filter clause */
        class Clause
        {
        public:

            /* The field to filter on */
            std::string strField;

            /* The operation code */
            uint8_t nOP;

            /* The value to filter on */
            std::string strValue;

        };


    }// end API namespace

}// end TAO namespace