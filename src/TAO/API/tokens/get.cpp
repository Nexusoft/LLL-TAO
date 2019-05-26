/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/tokens.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

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
                hashRegister = RegisterAddressFromName(params, "token", params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());
            else
                throw APIException(-23, "Missing memory address");

            /* Get the token / account object. */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashRegister, object))
                throw APIException(-24, "No token/account found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-24, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* If the user requested a particular object type then check it is that type */
                if(params.find("type") != params.end() && params["type"].get<std::string>() == "token")
                    throw APIException(-24, "Requested object is not a token");

                /* Get the identifier */
                uint256_t nIdentifier = object.get<uint256_t>("identifier");

                /* If the identifier is 0 then return the value "NXS" for clarity rather than 0 */
                if( nIdentifier == 0)
                    ret["identifier"] = "NXS";
                else
                    ret["identifier"] = object.get<uint256_t>("identifier").GetHex();
                
                /* Handle the digits.  The digits represent the maximum number of decimal places supported by the token
                   Therefore, to convert the internal value to a floating point value we need to reduce the internal value
                   by 10^digits  */
                uint64_t nDigits = GetTokenOrAccountDigits(object);

                ret["balance"]    = (double)object.get<uint64_t>("balance") / pow(10, nDigits);

            }
            else if(nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                /* If the user requested a particular object type then check it is that type */
                if(params.find("type") != params.end() && params["type"].get<std::string>() == "account")
                    throw APIException(-24, "Requested object is not an account");

                /* Handle the digits.  The digits represent the maximum number of decimal places supported by the token
                   Therefore, to convert the internal value to a floating point value we need to reduce the internal value
                   by 10^digits  */
                uint64_t nDigits = GetTokenOrAccountDigits(object);

                ret["identifier"]       = object.get<uint256_t>("identifier").GetHex();
                ret["balance"]          = (double) object.get<uint64_t>("balance") / pow(10, nDigits);
                ret["maxsupply"]        = (double) object.get<uint64_t>("supply") / pow(10, nDigits);
                ret["currentsupply"]    = (double) (object.get<uint64_t>("supply")
                                        - object.get<uint64_t>("balance")) / pow(10, nDigits);
                ret["digits"]           = nDigits;
            }
            else
                throw APIException(-27, "Unknown object register");

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */            
            if(params.find("fieldname") != params.end())
            {
                /* First get the fieldname from the response */
                std::string strFieldname =  params["fieldname"].get<std::string>();
                
                /* Iterate through the response keys */
                for (auto it = ret.begin(); it != ret.end(); ++it)
                    /* If this key is not the one that was requested then erase it */
                    if( it.key() != strFieldname)
                        ret.erase(it);
            }

            return ret;
        }
    }
}
