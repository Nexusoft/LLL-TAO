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
#include <TAO/Register/include/names.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Credit to a NXS account. */
        json::json Finance::Credit(const json::json& params, bool fHelp)
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

            /* Lock the signature chain. */
            LOCK(user->CREATE_MUTEX);

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions.");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction.");

            /* Submit the transaction payload. */
            uint256_t hashAccountTo = 0;

            /* If name_to is provided then use this to deduce the register address */
            if(params.find("name") != params.end())
                hashAccountTo = AddressFromName(params, params["name"].get<std::string>());

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashAccountTo.SetHex(params["address"].get<std::string>());

            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* Check for the proof parameter parameter. */
            uint256_t hashProof = 0;
            if(params.find("name_proof") != params.end())
            {
                /* If name_proof is provided then use this to deduce the register address */
                hashProof = AddressFromName( params, params["name_proof"].get<std::string>());
            }
            else if(params.find("address_proof") != params.end())
                hashProof.SetHex(params["address_proof"].get<std::string>());

            /* Read the previous transaction. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(hashTx, txPrev))
                throw APIException(-23, "Previous transaction not found.");

            /* Loop through all transactions. */
            int32_t nCurrent = -1;
            for(uint32_t nContract = 0; nContract < txPrev.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& contract = txPrev[nContract];

                /* Get the operation byte. */
                uint8_t nType = 0;
                contract >> nType;

                /* Check type. Only debits a coinbase transactions can be credited*/
                if(nType != TAO::Operation::OP::DEBIT && nType != TAO::Operation::OP::COINBASE)
                    continue;

                /* Process crediting a debit */
                if(nType == TAO::Operation::OP::DEBIT)
                {
                    /* Get the hashFrom from the debit transaction. */
                    uint256_t hashFrom = 0;
                    contract >> hashFrom;

                    /* Get the hashTo from the debit transaction. */
                    uint256_t hashTo = 0;
                    contract >> hashTo;

                    /* Get the amount to respond to. */
                    uint64_t nAmount = 0;
                    contract >> nAmount;

                    /* Get the token / account object that the debit was made to. */
                    TAO::Register::Object debit;
                    if(!LLD::Register->ReadState(hashTo, debit))
                        continue;

                    /* Parse the object register. */
                    if(!debit.Parse())
                        throw APIException(-24, "Failed to parse object from debit transaction");

                    /* Get the object standard. */
                    uint8_t nStandard = debit.Base();

                    /* In order to know how to process the credit we need to know whether it is a split payment or not.
                    for split payments the hashTo object will be an Asset, and the owner of that asset must be a token.
                    If this is the case, then the caller must supply both an account to receive their payment, and the
                    token account that proves their entitlement to the split of the debit payment. */

                    /* Check for the owner. If this is the current user then it must be a payment to an account/token and
                    therefore not a split payment. */
                    if(debit.hashOwner == user->Genesis())
                    {
                        /* Check the object base to see whether it is an account. */
                        if(debit.Base() == TAO::Register::OBJECTS::ACCOUNT)
                        {
                            /* If the user requested a particular object type then check it is that type */
                            std::string strType = params.find("type") != params.end() ? params["type"].get<std::string>() : "";
                            if((strType == "token" && nStandard == TAO::Register::OBJECTS::ACCOUNT))
                                continue;

                            else if(strType == "account" && nStandard == TAO::Register::OBJECTS::TOKEN)
                                continue;

                            if(debit.get<uint256_t>("token") != 0)
                                throw APIException(-24, "Debit transacton is not for a NXS account.  Please use the tokens API for crediting token accounts.");

                            /* if we passed these checks then insert the credit contract into the tx */
                            tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashTo <<  hashFrom << nAmount;

                        }
                        else
                            continue;
                    }
                    else
                    {
                        /* If the debit has not been made to an account that we own then we need to see whether it is a split payment.
                        This can be identified by the debit being an asset AND the owner being a token.  If this is the case
                        then the hashProof provided by the caller must be an account for the same token as the asset holder,
                        and hashAccountTo must be an account for the same token as the hashFrom account.*/
                        if(nStandard == TAO::Register::OBJECTS::NONSTANDARD)
                        {
                            /* retrieve the owner and check that it is a token */
                            TAO::Register::Object assetOwner;
                            if(!LLD::Register->ReadState(debit.hashOwner, assetOwner))
                                continue;

                            /* Parse the object register. */
                            if(!assetOwner.Parse())
                                throw APIException(-25, "Failed to parse asset owner object");

                            if(assetOwner.Standard() == TAO::Register::OBJECTS::TOKEN)
                            {
                                /* This is definitely a split payment so we now need to verify the token account and proof that
                                were passed in as parameters */
                                if(hashAccountTo == 0)
                                    throw APIException(-25, "Missing name / address of account to credit");

                                if(hashProof == 0)
                                    throw APIException(-25, "Missing name_proof / address_proof of token account that proves your credit share.");

                                /* Retrieve the account that the user has specified to make the payment to and ensure that it is for
                                the same token as the debit hashFrom */
                                TAO::Register::Object accountToCredit;
                                if(!LLD::Register->ReadState(hashAccountTo, accountToCredit))
                                    throw APIException(-25, "Invalid name / address of account to credit. ");

                                /* Parse the object register. */
                                if(!accountToCredit.Parse())
                                    throw APIException(-24, "Failed to parse account to credit");

                                /* Retrieve the account to debit from. */
                                TAO::Register::Object debitFromObject;
                                if(!LLD::Register->ReadState(hashFrom, debitFromObject))
                                    continue;

                                /* Parse the object register. */
                                if(!debitFromObject.Parse())
                                    throw APIException(-24, "Failed to parse object to debit from");

                                /* Check that the account being debited from is the same token type as as the account being
                                credited to*/
                                if(debitFromObject.get<uint256_t>("token") != 0)
                                    throw APIException(-24, "Debit transaction is not from a NXS account");

                                if(accountToCredit.get<uint256_t>("token") != 0)
                                    throw APIException(-24, "Account to credit is not a NXS account");

                                /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                                TAO::Register::Object proofObject;
                                if(!LLD::Register->ReadState(hashProof, proofObject))
                                    continue;

                                /* Parse the object register. */
                                if(!proofObject.Parse())
                                    throw APIException(-24, "Failed to parse proof object ");

                                /* Check that the proof is an account for the same token as the asset owner */
                                if(proofObject.get<uint256_t>("token") != assetOwner.get<uint256_t>("token"))
                                    throw APIException(-24, "Failed to proof account is for a different token than the asset token.");

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
                /* Process crediting a coinbase transaction */
                else if(nType == TAO::Operation::OP::COINBASE)
                {
                    /* Get the genesisHash of the user who mined the coinbase*/
                    uint256_t hashGenesis = 0;
                    contract >> hashGenesis;

                    /* Check that the coinbase was mined by the caller */
                    if(hashGenesis != user->Genesis())
                        throw APIException(-25, "Coinbase transaction mined by different user.");

                    /* Get the amount from the coinbase transaction. */
                    uint64_t nAmount = 0;
                    contract >> nAmount;

                    /* The proof for coinbase is the user's genesis hash, as you can only credit a coinbase that you mined */
                    hashProof = user->Genesis();

                    /* If the caller has not specified an account to credit then we will use the default account. */
                    if(hashAccountTo == 0)
                    {
                        TAO::Register::Object defaultNameRegister;

                        if(!TAO::Register::GetNameRegister(user->Genesis(), std::string("default"), defaultNameRegister))
                            throw APIException(-25, "Could not retrieve default NXS account to credit.");

                        /* Get the address that this name register is pointing to */
                        hashAccountTo = defaultNameRegister.get<uint256_t>("address");
                    }


                    /* Retrieve the account that the user has specified to credit and ensure that it is a NXS account*/
                    TAO::Register::Object accountToCredit;
                    if(!LLD::Register->ReadState(hashAccountTo, accountToCredit))
                        throw APIException(-25, "Invalid name / address of account to credit. ");

                    /* Parse the object register. */
                    if(!accountToCredit.Parse())
                        throw APIException(-24, "Failed to parse account to credit");

                    /* Check that the account being credited is a NXS account*/
                    if(accountToCredit.get<uint256_t>("token") != 0)
                        throw APIException(-24, "Account to credit is not a NXS account");


                    /* if we passed all of these checks then insert the credit contract into the tx */
                    tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashAccountTo <<  hashProof << nAmount;

                }

            }

            /* Check that output was found. */
            if(nCurrent == -1)
                throw APIException(-24, "no valid contracts in previous tx");

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-26, "Transaction failed to build");

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
