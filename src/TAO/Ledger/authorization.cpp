/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Register/types/basevm.h>

namespace TAO::Ledger
{
    /* Copy constructor. */
    Authorization::Authorization(const Authorization& tAuthorization)
    : ssAuthorization (tAuthorization.ssAuthorization)
    , vEvaluate       ( ) //we don't copy our memory stacks over
    , nCost           (0)
    {
    }


    /* Move constructor. */
    Authorization::Authorization(Authorization&& tAuthorization)
    : ssAuthorization (std::move(tAuthorization.ssAuthorization))
    , vEvaluate       ( ) //we don't copy our memory stacks over
    , nCost           (0)
    {
    }


    /* Default Destructor */
    ~Authorization::Authorization()
    {
    }


    /* Default constructor. */
    Authorization::Authorization(const TAO::Ledger::Stream& ssStreamIn)
    : ssAuthorization (ssStreamIn)
    , vEvaluate       ( )
    , nCost           (0)
    {
    }


    /* Evaluate the validation script. */
    bool Authorized()
    {

    }

    /* Get a value from the register virtual machine. */
     bool get_value(TAO::Register::Value& vRet)
     {

     }
}
