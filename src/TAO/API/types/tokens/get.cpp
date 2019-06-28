/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/tokens.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/json.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Tokens::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address.
             * Fail if no required parameters supplied. */
            if(params.find("name") != params.end())
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name / address");

            /* Get the token / account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-122, "Token/account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard == TAO::Register::OBJECTS::ACCOUNT || nStandard == TAO::Register::OBJECTS::TRUST)
            {
                /* If the user requested a particular object type then check it is that type */
                if(params.find("type") != params.end() && params["type"].get<std::string>() == "token")
                    throw APIException(-123, "Object is not a token");

                /* Convert the account object to JSON */
                ret = ObjectToJSON(params, object, hashRegister);

            }
            else if(nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                /* If the user requested a particular object type then check it is that type */
                if(params.find("type") != params.end() && params["type"].get<std::string>() == "account")
                    throw APIException(-126, "Object is not an account");

                /* Convert the token object to JSON */
                ret = ObjectToJSON(params, object, hashRegister);
            }
            else
                throw APIException(-122, "Token/account not found");

            /* Populate the response JSON */
            ret["owner"]    = object.hashOwner.GetHex();
            ret["created"]  = object.nCreated;
            ret["modified"] = object.nModified;

            json::json data  =TAO::API::ObjectToJSON(params, object, hashRegister);

            /* Copy the asset data in to the response after the type/checksum */
            ret.insert(data.begin(), data.end());


            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            if(params.find("fieldname") != params.end())
            {
                /* First get the fieldname from the response */
                std::string strFieldname =  params["fieldname"].get<std::string>();

                /* Iterate through the response keys */
                for(auto it = ret.begin(); it != ret.end(); ++it)
                    /* If this key is not the one that was requested then erase it */
                    if(it.key() != strFieldname)
                        ret.erase(it);
            }

            return ret;
        }
    }
}
