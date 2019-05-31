/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create an asset or digital item. */
        json::json Tokens::Credit(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for txid parameter. */
            if(params.find("txid") == params.end())
                throw APIException(-25, "Missing TxID.");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID.");

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions.");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction.");

            /* Submit the transaction payload. */
            uint256_t hashTo = 0;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* If name_to is provided then use this to deduce the register address */
                hashTo = RegisterAddressFromName( params, "token", params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashTo.SetHex(params["address"].get<std::string>());
            else
                throw APIException(-22, "Missing to name or address");

            /* Get the transaction id. */
            uint512_t hashTx = 0;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* Get the contract-id. */
            int32_t nContract = 0;

            /* The total value of the credit. */
            uint64_t nAmount = 0;

            /* Check for data parameter. */
            uint256_t hashProof = 0;
            if(params.find("name_proof") != params.end())
            {

                /* If name_proof is provided then use this to deduce the register address */
                hashProof = RegisterAddressFromName( params, "token", params["name_proof"].get<std::string>());

            }

            /* Handle RAW hex-encoded proof address. */
            else if(params.find("proof") != params.end())
                hashProof.SetHex(params["proof"].get<std::string>());

            /* Search for a proof if none is supplied. */
            else
            {
                /* Read the previous transaction. */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashTx, txPrev))
                    throw APIException(-23, "Previous transaction not found.");

                /* Loop through all transactions. */
                for(nContract = txPrev.Size() - 1; nContract >= 0; --nContract)
                {
                    /* Get the contract. */
                    const TAO::Operation::Contract& contract = txPrev[nContract];

                    /* Get the operation byte. */
                    uint8_t nType = 0;
                    contract >> nType;

                    /* Check type. */
                    if(nType != TAO::Operation::OP::DEBIT)
                        continue;

                    /* Get the hashFrom from the previous transaction. */
                    uint256_t hashFrom = 0;
                    contract >> hashFrom;

                    /* Get the hashFrom from the previous transaction. */
                    uint256_t hashTo = 0;
                    contract >> hashTo;

                    /* Get the amount to respond to. */
                    contract >> nAmount;

                    /* Get the token / account object that we are crediting to. This is required as we need to determine the
                       digits specification from the token definition, in order to convert the user supplied amount to the
                       internal amount */
                    TAO::Register::Object object;
                    if(!LLD::regDB->ReadState(hashTo, object))
                        continue;

                    /* Check for the owner. */
                    if(object.hashOwner == user->Genesis())
                    {
                        /* Parse the object register. */
                        if(!object.Parse())
                            throw APIException(-24, "Object failed to parse");

                        /* Get the object standard. */
                        uint8_t nStandard = object.Standard();

                        /* Check the object standard. */
                        if(nStandard == TAO::Register::OBJECTS::TOKEN || nStandard == TAO::Register::OBJECTS::ACCOUNT)
                        {
                            /* If the user requested a particular object type then check it is that type */
                            std::string strType = params.find("type") != params.end() ? params["type"].get<std::string>() : "";
                            if((strType == "token" && nStandard == TAO::Register::OBJECTS::ACCOUNT))
                                continue;

                            else if(strType == "account" && nStandard == TAO::Register::OBJECTS::TOKEN)
                                continue;
                        }
                        else
                            continue;

                        /* Set the proof if found. */
                        hashProof = hashFrom;

                        break;
                    }
                }

                /* Check that output was found. */
                if(nContract == -1)
                    throw APIException(-24, "no valid contracts in previous tx");

            }

            /* Submit the payload object. */
            tx[0] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashProof << hashTo << nAmount;

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-26, "Operations failed to execute.");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction.");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept.");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            return ret;
        }
    }
}
