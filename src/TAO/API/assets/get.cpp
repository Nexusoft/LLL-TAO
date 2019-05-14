/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/assets.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Assets::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = RegisterAddressFromName( params, "asset", params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing name / address");


            /* Check to see whether the caller has requested a specific data field to return */
            std::string strDataField = "";
            
            if(params.find("datafield") != params.end())
                strDataField =  params["datafield"].get<std::string>();

            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashRegister, object))
                throw APIException(-24, "Asset not found");

            /* parse object so that the data fields can be accessed */
            object.Parse();

            /* Convert the object to JSON */
            ret = TAO::API::ObjectRegisterToJSON(object, strDataField);

            return ret;
        }
    }
}
