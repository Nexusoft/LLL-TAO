/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/tokens.h>

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

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
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
                ret["identifier"] = object.get<uint32_t>("identifier");
                ret["balance"]    = object.get<uint64_t>("balance");

                //TODO: handle the digits value
                //read token object register by identifier
                //divide the balance by this total figures element
            }
            else if(nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                ret["identifier"]       = object.get<uint32_t>("identifier");
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
