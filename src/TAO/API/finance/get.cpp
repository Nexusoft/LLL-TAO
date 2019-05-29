/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/finance.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/jsonutils.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Finance::Get(const json::json& params, bool fHelp)
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
                throw APIException(-23, "Missing account name/address");

            /* Get the token / account object. */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashRegister, object, TAO::Register::FLAGS::MEMPOOL))
                throw APIException(-24, "Account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-24, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if( nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                throw APIException(-24, "Object is not an account");

            /* Check the account is a NXS account */
            if( object.get<uint256_t>("token_address") != 0)
                throw APIException(-24, "Account is not a NXS account.  Please use the tokens API for accessing non-NXS token accounts.");

            /* Convert the account object to JSON */
            ret = ObjectRegisterToJSON(object, hashRegister);

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
