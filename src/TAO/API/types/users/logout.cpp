/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/types/users.h>
#include <TAO/API/include/sessionmanager.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Login to a user account. */
        json::json Users::Logout(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Check for username parameter. */
            if(config::fMultiuser.load() && params.find("session") == params.end())
                throw APIException(-12, "Missing Session ID");

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint256_t nSession = 0;
            if(config::fMultiuser.load())
                nSession.SetHex(params["session"].get<std::string>());

            if(!GetSessionManager().Has(nSession))
                throw APIException(-141, "Already logged out");

            /* The genesis of the user logging out */
            uint256_t hashGenesis = GetSessionManager().Get(nSession).GetAccount()->Genesis();

            /* Disconnect all P2P connections on logout */
            if(LLP::P2P_SERVER)
            {
                for(uint16_t nThread = 0; nThread < LLP::P2P_SERVER->MAX_THREADS; ++nThread)
                {
                    /* Get the data threads. */
                    LLP::DataThread<LLP::P2PNode>* dt = LLP::P2P_SERVER->DATA_THREADS[nThread];

                    /* Lock the data thread. */
                    uint16_t nSize = static_cast<uint16_t>(dt->CONNECTIONS->size());

                    /* Loop through connections in data thread. */
                    for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
                    {
                        /* Get the connection */
                        auto& connection = dt->CONNECTIONS->at(nIndex);

                        /* Skip over inactive connections. */
                        if(connection != nullptr && connection->Connected())
                        {
                            /* Check that the connection is from this genesis hash  */
                            if(connection->hashGenesis != hashGenesis)
                                continue;

                            /* Send the terminate message to peer for graceful termination */
                            connection->PushMessage(LLP::P2P::ACTION::TERMINATE, connection->nSession);
                        }
                    }
                }
            }

            /* If not using multi-user then we need to send a deauth message to all peers */
            if(!config::fMultiuser.load())
            {
                /* Generate an DEAUTH message to send to all peers */
                DataStream ssMessage = LLP::TritiumNode::GetAuth(false);

                /* Check whether it is valid before relaying it to all peers */
                if(ssMessage.size() > 0)
                    LLP::TRITIUM_SERVER->_Relay(uint8_t(LLP::Tritium::ACTION::DEAUTH), ssMessage);
            }


            /* Remove the session from the notifications processor */
            if(NOTIFICATIONS_PROCESSOR)
                NOTIFICATIONS_PROCESSOR->Remove(nSession);

            /* If this is session 0 and stake minter is running when logout, stop it */
            TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();
            if(nSession==0 && stakeMinter.IsStarted())
                stakeMinter.Stop();

            /* Delete the sigchan. */
            {
                /* Lock the signature chain in case another process attempts to create a transaction . */
                LOCK(GetSessionManager().Get(nSession).CREATE_MUTEX);

                GetSessionManager().Remove(nSession);

            }


            ret["success"] = true;
            return ret;
        }
    }
}
