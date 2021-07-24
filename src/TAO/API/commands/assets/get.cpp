/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/assets.h>
#include <TAO/API/names/types/names.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        encoding::json Assets::Get(const encoding::json& jParams, const bool fHelp)
        {
            encoding::json ret;

            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(jParams.find("name") != jParams.end() && !jParams["name"].get<std::string>().empty())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(jParams, jParams["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(jParams.find("address") != jParams.end())
                hashRegister.SetBase58(jParams["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw Exception(-33, "Missing name / address");


            /* Check to see whether the caller has requested a specific data field to return */
            std::string strDataField = "";


            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::Register->ReadObject(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-34, "Asset not found");

            /* Now lets check our expected types match. */
            if(!CheckStandard(jParams, object))
                throw Exception(-49, "Unsupported type for name/address");

            /* Populate the response JSON */
            ret["owner"]    = TAO::Register::Address(object.hashOwner).ToString();
            ret["created"]  = object.nCreated;
            ret["modified"] = object.nModified;

            encoding::json data  =TAO::API::ObjectToJSON(jParams, object, hashRegister);

            /* Copy the asset data in to the response after the type/checksum */
            ret.insert(data.begin(), data.end());

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterFieldname(jParams, ret);

            return ret;
        }
    }
}
