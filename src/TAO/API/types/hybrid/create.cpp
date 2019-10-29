/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Creates a checkpoint for the hybrid network on the mainnet work*/
        json::json Hybrid::Create(const json::json& params, bool fHelp)
        {
            json::json ret;


            /* Check for txid parameter. */
            if(params.find("txid") == params.end())
                throw APIException(-50, "Missing txid.");

            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());


            /* TODO */
            /* Create the Checkpoint object */
            /* Set the txid */
  
            /* Build a JSON response object. */
            ret["txid"]  = hashTx.ToString();
            //ret["nonce"] = checkpoint.nNonce.ToString();

            return ret;
        }
    }
}
