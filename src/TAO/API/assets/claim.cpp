/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLC/include/random.h>
#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Claim a transferred asset. */
        json::json Assets::Claim(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for txid parameter. */
            if(params.find("txid") == params.end())
                throw APIException(-25, "Missing txid.");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID.");

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions.");

            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction.");

            /* Read the Transfer transaction being claimed. */
            TAO::Ledger::Transaction txTranser;
            if(!LLD::legDB->ReadTx(hashTx, txTranser))
                throw APIException(-23, "Transfer transaction not found.");

            /* Loop through all transactions. */
            int32_t nCurrent = -1;
            for(uint32_t nContract = 0; nContract < txTranser.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& contract = txTranser[nContract];

                /* Get the operation byte. */
                uint8_t nType = 0;
                contract >> nType;

                /* Check type. */
                if(nType != TAO::Operation::OP::TRANSFER)
                    continue;

                /* Get the address of the asset being transferred from the transaction. */
                uint256_t hashAddress = 0;
                contract >> hashAddress;

                /* Get the genesis hash (recipient) of the transfer*/
                uint256_t hashGenesis = 0;
                contract >> hashGenesis;

                /* Ensure that this transfer was meant for this user or that we are claiming back our own transfer */
                if(hashGenesis != tx.hashGenesis && tx.hashGenesis != txTranser.hashGenesis)
                    continue;

                /* Submit the payload object. */
                tx[++nCurrent] << (uint8_t)TAO::Operation::OP::CLAIM << hashTx << uint32_t(nContract) << hashAddress;

                /* If we are claiming a named asset, determine the name from the previous owner's sig chain
                and create a new Name record under our sig chain for the same name */
                TAO::Operation::Contract nameContract = CreateNameFromTransfer(hashTx, user->Genesis());
                /* If the Name contract operation was created then add it to the transaction */
                if(!nameContract.Empty())
                    tx[++nCurrent] = nameContract;

            }


            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            return ret;
        }
    }
}
