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

            /* Attempt to deduce the register address from name. */
            if(params.find("name") != params.end())
                hashRegister = AddressFromName(params, params["name"].get<std::string>());

            /* Get the RAW address from hex. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());
            else
                throw APIException(-23, "Missing account name/address");

            /* Get the token / account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-24, "Account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-24, "Object failed to parse");

            /* Check the object standard. */
            uint8_t nStandard = object.Standard();
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                throw APIException(-24, "Object is not an account");

            /* Check the account is a NXS account */
            if(object.get<uint256_t>("token") != 0)
                throw APIException(-24, "Account is not a NXS account.  Please use the tokens API for accessing non-NXS token accounts.");

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            if(params.find("fieldname") != params.end())
            {
                /* First get the fieldname from the response */
                std::string strFieldname =  params["fieldname"].get<std::string>();

                /* Get temp JSON. */
                json::json temp = ObjectToJSON(params, object, hashRegister);
                for(auto it = temp.begin(); it != temp.end(); ++it)
                {
                    /* If this key is not the one that was requested then erase it */
                    if(it.key() == strFieldname)
                        ret[it.key()] = it.value();
                }
            }
            else
                ret = ObjectToJSON(params, object, hashRegister);

            return ret;
        }
    }
}
