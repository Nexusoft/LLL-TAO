/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/jsonutils.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a list of assets owned by a signature chain. */
        json::json Users::Assets(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;// = json::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Watch for destination genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else if(!config::fAPISessions.load() && mapSessions.count(0))
                hashGenesis = mapSessions[0]->Genesis();
            else
                throw APIException(-25, "Missing Genesis or Username");

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* Get the list of registers owned by this sig chain */
            std::vector<uint256_t> vRegisters = GetRegistersOwnedBySigChain(hashGenesis);

            uint32_t nTotal = 0;
            /* Add the register data to the response */
            for(const auto& hashRegister : vRegisters)
            {
                /* Get the asset from the register DB.  We can read it as an Object and then check its nType to determine
                   whether or not it is an asset. */
                TAO::Register::Object object;
                if(!LLD::regDB->ReadState(hashRegister, object))
                    throw APIException(-24, "Asset not found");

                /* Only include raw and non-standard object types (assets)*/
                if( object.nType != TAO::Register::REGISTER::APPEND 
                    && object.nType != TAO::Register::REGISTER::RAW 
                    && object.nType != TAO::Register::REGISTER::OBJECT)
                {
                    continue;
                }

                if(object.nType == TAO::Register::REGISTER::OBJECT)
                {
                    /* parse object so that the data fields can be accessed */
                    object.Parse();

                    if( object.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                        continue;
                }

                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;
                    
                /* Convert the object to JSON */
                ret.push_back(TAO::API::ObjectRegisterToJSON(object, hashRegister));


            }

            return ret;
        }
    }
}
