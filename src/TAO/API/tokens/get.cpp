/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/tokens/types/tokens.h>

#include <TAO/API/include/json.h>
#include <TAO/API/include/build.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    json::json Tokens::Get(const json::json& params, bool fHelp)
    {
        /* Get the Register address. */
        const TAO::Register::Address hashRegister = ExtractAddress(params);

        /* Get the token / account object. */
        TAO::Register::Object object;
        if(!LLD::Register->ReadObject(hashRegister, object, TAO::Ledger::FLAGS::LOOKUP))
            throw APIException(-122, "Token/account not found");

        /* Check the object standard. */
        if(object.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw APIException(-122, "Token/account not found");

        /* If the user requested a particular object type then check it is that type */
        if(params.find("type") != params.end() && params["type"].get<std::string>() == "account")
            throw APIException(-65, "Object is not an account");

        /* Build our response object. */
        json::json jRet  = ObjectToJSON(params, object, hashRegister);
        jRet["owner"]    = TAO::Register::Address(object.hashOwner).ToString();
        jRet["created"]  = object.nCreated;
        jRet["modified"] = object.nModified;

        return jRet;
    }
}
