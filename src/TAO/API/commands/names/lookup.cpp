/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/names.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Cancel an active order in market. */
    encoding::json Names::Lookup(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract our address for name lookup. */
        if(!CheckParameter(jParams, "name", "string"))
            throw Exception(-11, "Missing Parameter [name]");

        /* Build our response object. */
        const TAO::Register::Address hashAddress =
            Names::ResolveAddress(jParams, jParams["name"].get<std::string>(), true).ToString();

        /* Check for this in database. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashAddress, tObject, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-12, "Object does not exist");

        /* Build our returned JSON data. */
        encoding::json jRet =
        {
            { "address", hashAddress.ToString() }
        };

        /* Add some register type info. */
        RegisterTypesToJSON(tObject, jRet);

        return jRet;
    }
}
