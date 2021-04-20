/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/sessionmanager.h>


#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/types/transaction.h>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Loads and resumes the users session from the local DB */
        json::json Users::Load(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Pin parameter. */
            SecureString strPin;

            /* Check for pin parameter. Parse the pin parameter. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw APIException(-129, "Missing PIN");

            if(strPin.size() == 0)
                throw APIException(-135, "Zero-length PIN");


            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Watch for destination genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end() && !params["genesis"].get<std::string>().empty())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Check for username. */
            else if(params.find("username") != params.end() && !params["username"].get<std::string>().empty())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());

            else
                throw APIException(-111, "Missing genesis / username");

            /* Load the session */
            Session& session = GetSessionManager().Load(hashGenesis, strPin);

            /* Check that it was loaded correctly */
            if(session.IsNull())
                throw APIException(-309, "Error loading session.");

            /* Add the session to the notifications processor if it is not already in there*/
            if(NOTIFICATIONS_PROCESSOR && NOTIFICATIONS_PROCESSOR->FindThread(session.ID()) == nullptr)
                NOTIFICATIONS_PROCESSOR->Add(session.ID());

            ret["genesis"] = hashGenesis.ToString();

            ret["session"] = session.ID().ToString();


            /* If in client mode, download the users signature chain transactions asynchronously. */
            if(config::fClient.load()) //XXX: so hacky, NEVER spawn a new thread, always use a thread pool
            {
                std::thread([&]()
                {
                    DownloadSigChain(hashGenesis, true);
                }).detach();
            }

            /* After unlock complete, attempt to start stake minter if unlocked for staking */
            if(session.CanStake())
            {
                TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();

                if(!stakeMinter.IsStarted())
                    stakeMinter.Start();
            }

            return ret;
        }
    }
}
