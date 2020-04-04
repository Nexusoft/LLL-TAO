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

            /* Generate an DEAUTH message to send to all peers.  NOTE We need to do this before we lock the MUTEX to delete
               the sig chain to avoid a deadlock, as the GetAuth method also takes a lock */
            DataStream ssMessage = LLP::TritiumNode::GetAuth(false);

            /* Delete the sigchan. */
            {
                LOCK(MUTEX);

                if(!mapSessions.count(nSession))
                    throw APIException(-141, "Already logged out");

                {
                    /* Lock the signature chain in case another process attempts to create a transaction . */
                    LOCK(CREATE_MUTEX);

                    memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = mapSessions[nSession];

                    user.free();

                    /* Erase the session. */
                    mapSessions.erase(nSession);

                    if(!pActivePIN.IsNull())
                        pActivePIN.free();
                }
            }

            /* If not using multi-user then we need to send a deauth message to all peers */
            if(!config::fMultiuser.load())
            {
                /* Check whether it is valid before relaying it to all peers */
                if(ssMessage.size() > 0)
                    LLP::TRITIUM_SERVER->Relay(uint8_t(LLP::ACTION::DEAUTH), ssMessage.Bytes());

                /* Free up the Auth private key */
                pAuthKey.free();
            }

            /* If stake minter is running when logout, stop it */
            TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();
            if(stakeMinter.IsStarted())
                stakeMinter.Stop();

            ret["success"] = true;
            return ret;
        }
    }
}
