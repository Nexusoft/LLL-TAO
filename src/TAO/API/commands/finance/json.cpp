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

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns the JSON representation of this invoice */
    encoding::json Finance::AccountToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister)
    {
        /* The JSON to return */
        encoding::json jRet =
            RegisterToJSON(rObject, hashRegister);

        /* Check for balance field. */
        if(jRet.find("balance") != jRet.end())
        {
            /* Get our figures to multiply by. */
            const uint64_t nFigures = GetFigures(rObject);

            /* Build an aggregate balance for accounts. */
            uint64_t nAggregate = (jRet["balance"].get<double>() * nFigures);
            if(jRet.find("stake") != jRet.end())
                nAggregate += uint64_t(jRet["stake"].get<double>() * nFigures);

            /* Check for unconfirmed aggregate balance. */
            if(jRet.find("unconfirmed") != jRet.end())
                nAggregate += uint64_t(jRet["unconfirmed"].get<double>() * nFigures);

            /* Add our aggregate value key now. */
            jRet["total"] = FormatBalance(nAggregate, rObject.get<uint256_t>("token"));
        }

        return jRet;
    }
}
