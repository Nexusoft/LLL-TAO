
/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/finance.h>
#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>

#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a summary of balance information for all NXS accounts belonging to the currently logged in signature chain */
        json::json Finance::GetBalances(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* The user genesis hash */
            uint256_t hashGenesis = user->Genesis();

            /* The confirmed balance from the state at the last block*/
            uint64_t nBalance = 0;

            /* The available balance including mempool transactions (outgoing debits) */
            uint64_t nAvailable = 0;

            /* The sum of all debits that are confirmed but not credited */
            uint64_t nPending = 0;

            /* The sum of all incoming debits that are not yet confirmed or credits we have made that are not yet confirmed*/
            uint64_t nUnconfirmed = 0;

            /* The sum of all unconfirmed outcoing debits */
            uint64_t nUnconfirmedOutgoing = 0;

            /* The amount currently being staked */
            uint64_t nStake = 0;

            /* The sum of all immature coinbase transactions */
            uint64_t nImmatureMined = 0;

            /* The sum of all immature coinstake transactions */
            uint64_t nImmatureStake = 0;

            /* vector of NXS accounts */
            std::vector<TAO::Register::Object> vAccounts;

            /* The trust account */
            TAO::Register::Object trust;

            /* First get the list of registers owned by this sig chain so we can work out which ones are NXS accounts */
            std::vector<uint256_t> vRegisters;
            if(!ListRegisters(hashGenesis, vRegisters))
                throw APIException(-74, "No registers found");

            /* Iterate through each register we own */
            for(const auto& hashRegister : vRegisters)
            {
                /* Get the register from the register DB */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashRegister, object)) // note we don't include mempool state here as we want the confirmed
                    continue;

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    continue;

                /* Check that this is an account */
                if(object.Base() != TAO::Register::OBJECTS::ACCOUNT )
                    continue;

                /* Check the account is a NXS account */
                if(object.get<uint256_t>("token") != 0)
                    continue;
                
                /* Add it to our list of accounts */
                vAccounts.push_back(object);

                /* Cache this if it is the trust account */
                if( object.Standard() == TAO::Register::OBJECTS::TRUST)
                    trust = object;
            }

            /* Iterate through accounts and sum the balances */
            for(const auto& account : vAccounts)
            {
                nBalance += account.get<uint64_t>("balance");
            }

            /* Find all pending debits to NXS accounts */
            nPending = GetPending(hashGenesis, 0);

            /* Get unconfirmed debits coming in and credits we have made */
            nUnconfirmed = GetUnconfirmed(hashGenesis, 0, false);

            /* Get all new debits that we have made */
            nUnconfirmedOutgoing = GetUnconfirmed(hashGenesis, 0, true);

            /* Calculate the available balance which is the last confirmed balance minus and mempool debits */
            nAvailable = nBalance - nUnconfirmedOutgoing;

            /* Get immature mined / staked */
            GetImmature(hashGenesis, nImmatureMined, nImmatureStake);

            /* Get the stake amount */
            nStake = trust.get<uint64_t>("stake");

            /* Populate the response object */
            ret["available"] = (double)nAvailable / TAO::Ledger::NXS_COIN;
            ret["pending"] = (double)nPending / TAO::Ledger::NXS_COIN;
            ret["unconfirmed"] = (double)nUnconfirmed / TAO::Ledger::NXS_COIN;
            ret["stake"] = (double)nStake / TAO::Ledger::NXS_COIN;
            ret["immature_mined"] = (double)nImmatureMined / TAO::Ledger::NXS_COIN;
            ret["immature_stake"] = (double)nImmatureStake / TAO::Ledger::NXS_COIN;

            return ret;
        }
    }
}