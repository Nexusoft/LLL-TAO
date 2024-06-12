/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/lookup.h>
#include <TAO/API/types/exception.h>

#include <TAO/API/include/json.h>

#include <TAO/Register/types/address.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Lookup(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Build our array object. */
        const encoding::json jList =
            Operators::Array(jParams, jResult);

        /* Keep a set to filter by unique. */
        std::set<TAO::Register::Address> setAddresses;

        /* Loop through to calculate sum. */
        encoding::json jRet = encoding::json::array();
        for(const auto& jItem : jList)
        {
            /* Handle for floats. */
            if(!jItem.is_string())
                throw Exception(-123, "[", jItem.type_name(), "] unsupported for operator [lookup]");

            /* Build a register address from string. */
            TAO::Register::Address addrLookup =
                TAO::Register::Address(jItem.get<std::string>());

            /* Check that we have a valid address encoding. */
            if(!addrLookup.IsValid())
                throw Exception(-123, "Register address filter required for operator [lookup]");

            /* Check for unique. */
            if(setAddresses.count(addrLookup))
                continue;

            /* Read our register address. */
            TAO::Register::Object tRegister;
            if(!LLD::Register->ReadObject(addrLookup, tRegister, TAO::Ledger::FLAGS::LOOKUP))
                continue;

            /* Build our register object to JSON encoding. */
            const encoding::json jRegister =
                TAO::API::RegisterToJSON(tRegister, addrLookup);

            /* Add to our return values. */
            jRet.push_back(jRegister);

            /* Add to our unique filter. */
            setAddresses.insert(addrLookup);
        }

        return jRet;
    }
}
