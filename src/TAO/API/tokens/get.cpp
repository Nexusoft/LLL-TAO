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

            /* Get the history. */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashRegister, object))
                throw APIException(-24, "No object found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-24, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* Get the identifier */
                uint256_t nIdentifier = object.get<uint256_t>("identifier");

                /* If the identifier is 0 then return the value "NXS" for clarity rather than 0 */
                if( nIdentifier == 0)
                    ret["identifier"] = "NXS";
                else
                    ret["identifier"] = object.get<uint256_t>("identifier").GetHex();
                
                ret["balance"]    = object.get<uint64_t>("balance");

                //TODO: handle the digits value
                //read token object register by identifier
                //divide the balance by this total figures element
            }
            else if(nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                ret["identifier"]       = object.get<uint256_t>("identifier").GetHex();
                ret["balance"]          = object.get<uint64_t>("balance");
                ret["maxsupply"]        = object.get<uint64_t>("supply");
                ret["currentsupply"]    = object.get<uint64_t>("supply")
                                        - object.get<uint64_t>("balance");
            }
            else
                throw APIException(-27, "Unknown object register");

            return ret;
        }
    }
}
