/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

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
                throw APIException(-50, "Missing txid.");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID.");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-16, "Account has not been unlocked for transactions.");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction.");

            /* Submit the transaction payload. */
            TAO::Register::Address hashAccountTo;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* If name_to is provided then use this to deduce the register address */
                hashAccountTo = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashAccountTo.SetBase58(params["address"].get<std::string>());

            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* Check for the proof parameter parameter. */
            TAO::Register::Address hashProof;
            if(params.find("name_proof") != params.end())
            {
                /* If name_proof is provided then use this to deduce the register address */
                hashProof = Names::ResolveAddress(params, params["name_proof"].get<std::string>());
            }
            else if(params.find("address_proof") != params.end())
                hashProof.SetBase58(params["address_proof"].get<std::string>());

            /* Read the debit transaction that is being credited. */
            TAO::Ledger::Transaction txDebit;
            if(!LLD::Ledger->ReadTx(hashTx, txDebit, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-40, "Previous transaction not found.");

            /* Loop through all transactions. */
            int32_t nCurrent = -1;
            for(uint32_t nContract = 0; nContract < txDebit.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& contract = txDebit[nContract];

                /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                contract.Reset();

                /* Get the operation byte. */
                uint8_t nType = 0;
                contract >> nType;

                /* Check for validate and condition. */
                switch(nType)
                {
                    case TAO::Operation::OP::VALIDATE:
                    {
                        /* Seek through validate. */
                        contract.Seek(68);
                        contract >> nType;

                        break;
                    }

                    case TAO::Operation::OP::CONDITION:
                    {
                        /* Get new operation. */
                        contract >> nType;
                    }
                }

                /* Check type. */
                if(nType != TAO::Operation::OP::DEBIT)
                    continue;

                /* Get the hashFrom from the debit transaction. */
                TAO::Register::Address hashFrom;
                contract >> hashFrom;

                /* Get the hashTo from the debit transaction. */
                TAO::Register::Address hashTo;
                contract >> hashTo;

                /* Get the amount to respond to. */
                uint64_t nAmount = 0;
                contract >> nAmount;

                /* Check for wildcard. */
                if(hashTo == TAO::Register::WILDCARD_ADDRESS)
                {
                    /* if we passed these checks then insert the credit contract into the tx */
                    tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashAccountTo << hashFrom << nAmount;

                    continue;
                }

                /* Get the token / account object that the debit was made to. */
                TAO::Register::Object objectTo;
                if(!LLD::Register->ReadState(hashTo, objectTo, TAO::Ledger::FLAGS::MEMPOOL))
                    continue;

                /* Parse the object register. */
                if(!objectTo.Parse())
                    throw APIException(-41, "Failed to parse object from debit transaction");

                /* Get the object standard. */
                uint8_t nStandard = objectTo.Base();

                /* In order to know how to process the credit we need to know whether it is a split payment or not.
                   for split payments the hashTo object will be an Asset, and the owner of that asset must be a token.
                   If this is the case, then the caller must supply both an account to receive their payment, and the
                   token account that proves their entitlement to the split of the debit payment. */

                /* Check to see whether this is a simple debit to an account or token */
                if(objectTo.Base() == TAO::Register::OBJECTS::ACCOUNT || objectTo.Base() == TAO::Register::OBJECTS::TOKEN)
                {
                    /* Check that the debit was made to an account that we own */
                    if(objectTo.hashOwner == user->Genesis())
                    {
                        /* If the user requested a particular object type then check it is that type */
                        std::string strType = params.find("type") != params.end() ? params["type"].get<std::string>() : "";
                        if((strType == "token" && nStandard == TAO::Register::OBJECTS::ACCOUNT))
                            continue;

                        else if(strType == "account" && nStandard == TAO::Register::OBJECTS::TOKEN)
                            continue;

                        if(objectTo.get<uint256_t>("token") == 0)
                                throw APIException(-120, "Debit transaction is for a NXS account.  Please use the Finance API for crediting NXS accounts.");

                        /* if we passed these checks then insert the credit contract into the tx */
                        tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashTo <<  hashFrom << nAmount;

                    }
                    else
                        continue;
                }
                else
                {
                    /* If the debit has not been made to an account that we own then we need to see whether it is a split payment.
                       This can be identified by the objectTo being an asset AND the owner being a token.  If this is the case
                       then the hashProof provided by the caller must be an account for the same token as the asset holder,
                       and hashAccountTo must be an account for the same token as the hashFrom account.*/
                    if(nStandard == TAO::Register::OBJECTS::NONSTANDARD)
                    {
                        /* retrieve the owner and check that it is a token */
                        TAO::Register::Object assetOwner;
                        if(!LLD::Register->ReadState(objectTo.hashOwner, assetOwner, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Parse the object register. */
                        if(!assetOwner.Parse())
                            throw APIException(-52, "Failed to parse asset owner object");

                        if(assetOwner.Standard() == TAO::Register::OBJECTS::TOKEN)
                        {
                            /* This is definitely a split payment so we now need to verify the token account and proof that
                               were passed in as parameters */
                            if(hashAccountTo == 0)
                                throw APIException(-53, "Missing name / address of account to credit.");

                            if(hashProof == 0)
                                throw APIException(-54, "Missing name_proof / address_proof of token account that proves your credit share.");

                            /* Retrieve the account that the user has specified to make the payment to and ensure that it is for
                               the same token as the debit hashFrom */
                            TAO::Register::Object accountToCredit;
                            if(!LLD::Register->ReadState(hashAccountTo, accountToCredit, TAO::Ledger::FLAGS::MEMPOOL))
                                throw APIException(-55, "Invalid name / address of account to credit. ");

                            /* Parse the object register. */
                            if(!accountToCredit.Parse())
                                throw APIException(-56, "Failed to parse account to credit.");

                            /* Retrieve the account to debit from. */
                            TAO::Register::Object debitFromObject;
                            if(!LLD::Register->ReadState(hashFrom, debitFromObject, TAO::Ledger::FLAGS::MEMPOOL))
                                continue;

                            /* Parse the object register. */
                            if(!debitFromObject.Parse())
                                throw APIException(-57, "Failed to parse object to debit from.");

                            /* Check that the account being debited from is the same token type as as the account being
                               credited to*/
                            if(debitFromObject.get<uint256_t>("token") != accountToCredit.get<uint256_t>("token"))
                                throw APIException(-121, "Account to credit is not for the same token as the debit.");


                            /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                            TAO::Register::Object proofObject;
                            if(!LLD::Register->ReadState(hashProof, proofObject, TAO::Ledger::FLAGS::MEMPOOL))
                                continue;

                            /* Parse the object register. */
                            if(!proofObject.Parse())
                                throw APIException(-60, "Failed to parse proof object.");

                            /* Check that the proof is an account for the same token as the asset owner */
                            if(proofObject.get<uint256_t>("token") != assetOwner.get<uint256_t>("token"))
                                throw APIException(-61, "Proof account is for a different token than the asset token.");

                            /* Calculate the partial amount we want to claim based on our share of the proof tokens */
                            uint64_t nPartial = (proofObject.get<uint64_t>("balance") * nAmount) / assetOwner.get<uint64_t>("supply");

                            /* if we passed all of these checks then insert the credit contract into the tx */
                            tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashAccountTo <<  hashProof << nPartial;

                        }
                        else
                            continue;

                    }
                    else
                        continue;
                }

            }

            /* Check that output was found. */
            if(nCurrent == -1)
                throw APIException(-43, "No valid contracts in debit tx.");

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction.");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept.");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            return ret;
        }
    }
}
