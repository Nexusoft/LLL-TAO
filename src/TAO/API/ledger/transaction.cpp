/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/ledger.h>

#include <LLD/include/global.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/API/include/utils.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Retrieves the transaction for the given hash. */
        json::json Ledger::Transaction(const json::json& params, bool fHelp)
        {
            /* Check for the transaction hash parameter. */
            if(params.find("hash") == params.end())
                throw APIException(-25, "Missing hash");


            /* Extract the hash out of the JSON params*/
            uint512_t hash;
            hash.SetHex(params["hash"].get<std::string>());


            uint32_t nTransactionVerbosity = 2; /* Default to verbosity 2 which includes the hash and basic data */
            if( params.count("txverbose") > 0 && IsAllDigit(params["txverbose"].get<std::string>()))
                nTransactionVerbosity = std::stoul(params["txverbose"].get<std::string>());


            /* keep the verbosity level no less than 2.  Also translate a calling level of 1 to 2 (add 1 to it)
             so that we can use the generic TransactionToJSON method which uses a 0-4 range */
            nTransactionVerbosity = nTransactionVerbosity < 1 ? 2 : nTransactionVerbosity +  1;


            /* Declare the tritium and legacy transaction objects ready to attempt to load them from disk*/
            TAO::Ledger::Transaction txTritium;
            Legacy::Transaction txLegacy;

            //NOTE: this code here is messy, needs better formatting
            TAO::Ledger::BlockState blockState;


            /* Read the block state from the the ledger DB using the transaction hash index */
            LLD::legDB->ReadBlock(hash, blockState);


            /* Declare the JSON return object */
            json::json ret;


            /* Get the transaction either from disk or mempool.
               First try to see if it is a tritium tx in the leger db*/
            if(TAO::Ledger::mempool.Get(hash, txTritium) || LLD::legDB->ReadTx(hash, txTritium))
            {
                ret = TAO::API::TransactionToJSON (txTritium, blockState, nTransactionVerbosity);
            }

            /* If it is not a tritium transaction then see if it is a legacy tx in the legacy DB */
            else if(TAO::Ledger::mempool.Get(hash, txLegacy) || LLD::legacyDB->ReadTx(hash, txLegacy))
            {
                ret = TAO::API::TransactionToJSON (txLegacy, blockState, nTransactionVerbosity);
            }

            else
            {
                throw APIException(-28, "Invalid or unknown transaction");
            }

            return ret;
        }

    } /* End API namespace */

}/* End Ledger namespace */
