/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/user_types.h>

#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>

#include <TAO/API/include/json.h>
#include <TAO/API/users/types/users.h>
#include <TAO/API/invoices/types/invoices.h>

#include <TAO/Operation/include/enum.h>


#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generic method to list object registers by sig chain*/
        json::json Users::Invoices(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret = json::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

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
                hashGenesis = users->GetSession(params).GetAccount()->Genesis();

            if(config::fClient.load() && hashGenesis != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Get the params to apply to the response. */
            GetListParams(params, strOrder, nLimit, nOffset);

            /* Get the list of registers owned by this sig chain */
            std::vector<TAO::Register::Address> vAddresses;
            ListRegisters(hashGenesis, vAddresses);

            /* Get any registers that have been transferred to this user but not yet paid (claimed) */
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vUnclaimed;
            Users::get_unclaimed(hashGenesis, vUnclaimed);

            /* Add the unclaimed register addresses to the list */
            for(const auto& unclaimed : vUnclaimed)
                vAddresses.push_back(std::get<2>(unclaimed));

            /* For efficiency we can remove any addresses that are not read only registers */
            vAddresses.erase(std::remove_if(vAddresses.begin(), vAddresses.end(),
                                            [](const TAO::Register::Address& address){return !address.IsReadonly();}),
                                            vAddresses.end());

            /* Read all the registers to that they are sorted by creation time */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::State>> vRegisters;
            GetRegisters(vAddresses, vRegisters);

            /* Add the register data to the response */
            uint32_t nTotal = 0;
            for(const auto& state : vRegisters)
            {
                /* Only include read only register type */
                if(state.second.nType != TAO::Register::REGISTER::READONLY)
                    continue;

                /* Deserialize the leading byte of the state data to check that it is an invoice */
                uint16_t nType = 0;
                state.second >> nType;

                /* Check that we are filtering the correct registers denoted by type value. */
                if(nType != TAO::API::USER_TYPES::INVOICE)
                    continue;

                /* The invoice JSON data */
                json::json invoice = Invoices::InvoiceToJSON(params, state.second, state.first);

                /* Check the offset. */
                if(++nTotal <= nOffset)
                    continue;

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                /* Add the invoice json to the response */
                ret.push_back(invoice);

            }

            return ret;
        }
    }
}
