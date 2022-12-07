/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>

#include <TAO/API/types/commands/finance.h>

#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns the JSON representation of this invoice */
    encoding::json Finance::AccountToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister)
    {
        /* The JSON to return */
        encoding::json jRet =
            RegisterToJSON(rObject, hashRegister);

        /* Get our figures to multiply by. */
        const uint8_t nDecimals = GetDecimals(rObject);

        /* Build our total precision_t object. */
        precision_t dTotal =
            precision_t(0.0, nDecimals).double_t();

        /* Check for the stake field. */
        if(jRet.find("stake") != jRet.end())
        {
            /* Set our total value. */
            dTotal += precision_t(jRet["stake"].dump(), nDecimals);

            /* Make sure decimals are handled correctly. */
            jRet["stake"] = precision_t(jRet["stake"].dump(), nDecimals).double_t();
        }

        /* Check for the balance field. */
        if(jRet.find("balance") != jRet.end())
        {
            /* Set our total value. */
            dTotal += precision_t(jRet["balance"].dump(), nDecimals);

            /* Make sure decimals are handled correctly. */
            jRet["balance"] = precision_t(jRet["balance"].dump(), nDecimals).double_t();
        }

        /* Check for the uncomfirmed field. */
        if(jRet.find("unconfirmed") != jRet.end())
        {
            /* Set our total value. */
            dTotal += precision_t(jRet["unconfirmed"].dump(), nDecimals);

            /* Make sure decimals are handled correctly. */
            jRet["unconfirmed"] = precision_t(jRet["unconfirmed"].dump(), nDecimals).double_t();
        }

        /* Make sure decimals are handled correctly. */
        if(jRet.find("maxsupply") != jRet.end())
            jRet["maxsupply"] = precision_t(jRet["maxsupply"].dump(), nDecimals).double_t();

        /* Make sure decimals are handled correctly. */
        if(jRet.find("currentsupply") != jRet.end())
            jRet["currentsupply"] = precision_t(jRet["currentsupply"].dump(), nDecimals).double_t();

        /* Add our aggregate value key now. */
        jRet["total"] = dTotal.double_t();

        return jRet;
    }
}
