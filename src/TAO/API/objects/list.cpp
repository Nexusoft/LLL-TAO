/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/objects/types/objects.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/list.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generic method to list object registers by sig chain*/
        encoding::json Objects::List(const encoding::json& params, const uint8_t nRegisterType, const uint8_t nObjectType)
        {
            /* JSON return value. */
            encoding::json ret = encoding::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Check for txid. */
            uint512_t hashLast = 0;
            if(params.find("txid") != params.end())
                hashLast.SetHex(params["txid"]);

            /* Watch for destination genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end() && !params["genesis"].get<std::string>().empty())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Check for username. */
            else if(params.find("username") != params.end() && !params["username"].get<std::string>().empty())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());

            /* Check for logged in user.  NOTE: we rely on the GetSession method to check for the existence of a valid session ID
               in the parameters in multiuser mode, or that a user is logged in for single user mode. Otherwise the GetSession
               method will throw an appropriate error. */
            else
                hashGenesis = Commands::Get<Users>()->GetSession(params).GetAccount()->Genesis();

            if(config::fClient.load() && hashGenesis != Commands::Get<Users>()->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Get the params to apply to the response. */
            ExtractList(params, strOrder, nLimit, nOffset);

            /* Get the list of registers owned by this sig chain */
            std::vector<TAO::Register::Address> vAddresses;
            ListRegisters(hashGenesis, vAddresses, hashLast);

            /* Get list of tokenized assets owned by this sig chain */
            ListPartial(hashGenesis, vAddresses);
            if(vAddresses.size() == 0)
                throw APIException(-74, "No registers found");

            /* Read all the registers to that they are sorted by creation time */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::State>> vRegisters;
            GetRegisters(vAddresses, vRegisters);

            /* We pass false for fLookupName if the requested type is a name of namesace object,
               as those are the edge case that do not have a Name object themselves */
            bool fLookupName = false;//nObjectType != TAO::Register::OBJECTS::NAME && nObjectType != TAO::Register::OBJECTS::NAMESPACE;

            /* Add the register data to the response */
            uint32_t nTotal = 0;
            for(const auto& state : vRegisters)
            {
                /* Only include requested register type */
                if(state.second.nType != nRegisterType)
                    continue;

                /* Cast to an Object */
                TAO::Register::Object object(state.second);

                /* Handle for object registers. */
                if(object.nType == TAO::Register::REGISTER::OBJECT)
                {
                    /* parse object so that the data fields can be accessed */
                    if(!object.Parse())
                        throw APIException(-36, "Failed to parse object register");

                    /* Only included requested object types. */
                    if(!(TAO::Register::OBJECTS::BITWISE(object.Standard()) & TAO::Register::OBJECTS::BITWISE(nObjectType)))
                        continue;
                }

                /* Populate the response JSON */
                encoding::json json;
                json["created"]  = state.second.nCreated;
                json["modified"] = state.second.nModified;

                encoding::json data  = TAO::API::ObjectToJSON(params, object, state.first, fLookupName);

                /* Copy the data in to the response after the  */
                json.insert(data.begin(), data.end());

                /* Check to see whether the transaction has had all children filtered out */
                if(data.empty())
                    continue;

                /* Check the offset. */
                if(++nTotal <= nOffset)
                    continue;

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                /* Add this objects json to the response */
                ret.push_back(json);
            }

            return ret;
        }
    }
}
