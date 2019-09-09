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
#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/trust.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>

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
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Check for txid parameter. */
            if(params.find("txid") == params.end())
                throw APIException(-50, "Missing txid.");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID.");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Check that the sig chain is mature after the last coinbase/coinstake transaction in the chain. */
            CheckMature(user->Genesis());

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction.");

            /* Submit the transaction payload. */
            TAO::Register::Address hashAccountTo;

            /* If name_to is provided then use this to deduce the register address */
            if(params.find("name") != params.end())
                hashAccountTo = Names::ResolveAddress(params, params["name"].get<std::string>());

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

            /* The debit transaction (if tritium) */
            TAO::Ledger::Transaction txDebit;

            /* The legacy send transaction (if legacy) */
            Legacy::Transaction txSend;

            /* Read the debit transaction. This may be a tritium or legacy transaction so we need to check both database
               and process it accordingly. */
            if(LLD::Ledger->ReadTx(hashTx, txDebit))
            {

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

                    /* Check type. Only debits a coinbase transactions can be credited*/
                    if(nType != TAO::Operation::OP::DEBIT && nType != TAO::Operation::OP::COINBASE)
                        continue;

                    /* Process crediting a debit */
                    if(nType == TAO::Operation::OP::DEBIT)
                    {
                        /* Get the hashFrom from the debit transaction. */
                        TAO::Register::Address hashFrom;
                        contract >> hashFrom;

                        /* Get the hashTo from the debit transaction. */
                        TAO::Register::Address hashTo;
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
                            throw APIException(-41, "Failed to parse object from debit transaction");

                        /* Get the object standard. */
                        uint8_t nStandard = debit.Base();

                        /* In order to know how to process the credit we need to know whether it is a split payment or not.
                        For split payments the hashTo object will be an Asset, and the owner of that asset must be a token.
                        If this is the case, then the caller must supply both an account to receive their payment, and the
                        token account that proves their entitlement to the split of the debit payment. */

                        /* Check to see whether this is a simple debit to an account */
                        if(debit.Base() == TAO::Register::OBJECTS::ACCOUNT)
                        {
                            /* Check that the debit was made to an account that we own */
                            if(debit.hashOwner == user->Genesis())
                            {
                                /* If the user requested a particular object type then check it is that type */
                                std::string strType = params.find("type") != params.end() ? params["type"].get<std::string>() : "";
                                if((strType == "token" && nStandard == TAO::Register::OBJECTS::ACCOUNT))
                                    continue;

                                else if(strType == "account" && nStandard == TAO::Register::OBJECTS::TOKEN)
                                    continue;

                                if(debit.get<uint256_t>("token") != 0)
                                    throw APIException(-51, "Debit transaction is not for a NXS account.  Please use the tokens API for crediting token accounts.");

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
                                    throw APIException(-52, "Failed to parse asset owner object");

                                if(assetOwner.Standard() == TAO::Register::OBJECTS::TOKEN)
                                {
                                    /* This is definitely a split payment so we now need to verify the token account and proof that
                                    were passed in as parameters */
                                    if(hashAccountTo == 0)
                                        throw APIException(-53, "Missing name / address of account to credit");

                                    if(hashProof == 0)
                                        throw APIException(-54, "Missing name_proof / address_proof of token account that proves your credit share.");

                                    /* Retrieve the account that the user has specified to make the payment to and ensure that it is for
                                    the same token as the debit hashFrom */
                                    TAO::Register::Object accountToCredit;
                                    if(!LLD::Register->ReadState(hashAccountTo, accountToCredit))
                                        throw APIException(-55, "Invalid name / address of account to credit. ");

                                    /* Parse the object register. */
                                    if(!accountToCredit.Parse())
                                        throw APIException(-56, "Failed to parse account to credit");

                                    /* Retrieve the account to debit from. */
                                    TAO::Register::Object debitFromObject;
                                    if(!LLD::Register->ReadState(hashFrom, debitFromObject))
                                        continue;

                                    /* Parse the object register. */
                                    if(!debitFromObject.Parse())
                                        throw APIException(-57, "Failed to parse object to debit from");

                                    /* Check that the account being debited from is the same token type as as the account being
                                    credited to*/
                                    if(debitFromObject.get<uint256_t>("token") != 0)
                                        throw APIException(-58, "Debit transaction is not from a NXS account");

                                    if(accountToCredit.get<uint256_t>("token") != 0)
                                        throw APIException(-59, "Account to credit is not a NXS account");

                                    /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                                    TAO::Register::Object proofObject;
                                    if(!LLD::Register->ReadState(hashProof, proofObject))
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
                    /* Process crediting a coinbase transaction */
                    else if(nType == TAO::Operation::OP::COINBASE)
                    {
                        /* Get the genesisHash of the user who mined the coinbase*/
                        uint256_t hashGenesis = 0;
                        contract >> hashGenesis;

                        /* Check that the coinbase was mined by the caller */
                        if(hashGenesis != user->Genesis())
                            throw APIException(-62, "Coinbase transaction mined by different user.");

                        /* Get the amount from the coinbase transaction. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* If the caller has not specified an account to credit then we will use the default account. */
                        if(hashAccountTo == 0)
                        {
                            TAO::Register::Object defaultNameRegister;

                            if(!TAO::Register::GetNameRegister(user->Genesis(), std::string("default"), defaultNameRegister))
                                throw APIException(-63, "Could not retrieve default NXS account to credit.");

                            /* Get the address that this name register is pointing to */
                            hashAccountTo = defaultNameRegister.get<uint256_t>("address");
                        }


                        /* Retrieve the account that the user has specified to credit and ensure that it is a NXS account*/
                        TAO::Register::Object accountToCredit;
                        if(!LLD::Register->ReadState(hashAccountTo, accountToCredit))
                            throw APIException(-55, "Invalid name / address of account to credit. ");

                        /* Parse the object register. */
                        if(!accountToCredit.Parse())
                            throw APIException(-56, "Failed to parse account to credit");

                        /* Check that the account being credited is a NXS account*/
                        if(accountToCredit.get<uint256_t>("token") != 0)
                            throw APIException(-59, "Account to credit is not a NXS account");


                        /* if we passed all of these checks then insert the credit contract into the tx */
                        tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashAccountTo <<  user->Genesis() << nAmount;

                    }

                }
            }
            else if(LLD::Legacy->ReadTx(hashTx, txSend))
            {
                /* Iterate through all TxOut's in the legacy transaction to see which are sends to a sig chain  */
                for(uint32_t nContract = 0; nContract < txSend.vout.size(); ++nContract)
                {
                    /* The TxOut to be checked */
                    const Legacy::TxOut& txout = txSend.vout[nContract];

                    /* The hash of the receiving account. */
                    TAO::Register::Address hashAccount;

                    /* Extract the sig chain account register address  from the legacy script */
                    if(!Legacy::ExtractRegister(txout.scriptPubKey, hashAccount))
                        continue;

                    /* Get the token / account object that the debit was made to. */
                    TAO::Register::Object debit;
                    if(!LLD::Register->ReadState(hashAccount, debit))
                        continue;

                    /* Parse the object register. */
                    if(!debit.Parse())
                        throw APIException(-41, "Failed to parse object from debit transaction");

                    /* Get the object standard. */
                    uint8_t nStandard = debit.Base();

                    /* Check for the owner to make sure this was a send to the current users account */
                    if(debit.hashOwner == user->Genesis())
                    {
                        /* Identify trust migration to create OP::MIGRATE instead of OP::CREDIT */

                        /* Check if output is new trust account (no stake or balance) */
                        if(debit.Standard() == TAO::Register::OBJECTS::TRUST
                                && debit.get<uint64_t>("stake") == 0 && debit.get<uint64_t>("trust") == 0)
                        {
                            /* Need to check for migration.
                             * Trust migration converts a legacy trust key to a trust account register.
                             * It will send all inputs from an existing trust key, with one output to a new trust account.
                             */
                            bool fMigration = false; //if this stays false, not a migration, fall through to OP::CREDIT

                            /* Trust key data we need for OP::MIGRATE */
                            uint32_t nScore;
                            uint512_t hashKey;
                            uint512_t hashLast;

                            /* This loop will only have one iteration. If it breaks out before end, fMigration stays false */
                            while(1)
                            {
                                Legacy::TrustKey trustKey;

                                /* Trust account output must be only output for the transaction */
                                if(nContract != 0 || txSend.vout.size() > 1)
                                    break;

                                /* Retrieve the trust key being converted */
                                if(!Legacy::FindMigratedTrustKey(txSend, trustKey))
                                    break;

                                /* Verify trust key not already converted */
                                hashKey = trustKey.GetHash();
                                if(LLD::Legacy->HasTrustConversion(hashKey))
                                    break;

                                /* Get last trust for the legacy trust key */
                                TAO::Ledger::BlockState stateLast;
                                if(!LLD::Ledger->ReadBlock(trustKey.hashLastBlock, stateLast))
                                    break;

                                /* Last stake block must be at least v5 and coinstake must be a legacy transaction */
                                if(stateLast.nVersion < 5 || stateLast.vtx[0].first != TAO::Ledger::LEGACY)
                                    break;

                                /* Extract the coinstake from the last trust block */
                                Legacy::Transaction txLast;
                                if(!LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast))
                                    break;

                                hashLast = txLast.GetHash();

                                /* Extract the trust score from the coinstake */
                                uint1024_t hashLastBlock;
                                uint32_t nSequence;

                                if(txLast.IsGenesis())
                                    nScore = 0;

                                else if(!txLast.ExtractTrust(hashLastBlock, nSequence, nScore))
                                    break;

                                fMigration = true;
                                break;
                            }

                            /* Everything verified for migration and we have the data we need. Set up OP::MIGRATE */
                            if(fMigration)
                            {
                                /* The amount to migrate */
                                const int64_t nLegacyAmount = txout.nValue;
                                uint64_t nAmount = 0;
                                if(nLegacyAmount > 0)
                                    nAmount = nLegacyAmount;

                                /* Set up the OP::MIGRATE */
                                tx[tx.Size()] << uint8_t(TAO::Operation::OP::MIGRATE) << hashTx << hashAccount << hashKey
                                            << nAmount << nScore << hashLast;

                                continue;
                            }
                        }

                        /* No migration. Use normal credit process */

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
                                throw APIException(-51, "Debit transaction is not for a NXS account.  Please use the tokens API for crediting token accounts.");

                            /* The amount to credit */
                            uint64_t nAmount = txout.nValue;

                            /* if we passed these checks then insert the credit contract into the tx */
                            tx[tx.Size()] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashAccount <<  TAO::Register::WILDCARD_ADDRESS << nAmount;

                        }
                        else
                            continue;
                    }


                }
            }
            else
            {
                throw APIException(-40, "Previous transaction not found.");
            }


            /* Check that output was found. */
            if(tx.Size() == 0)
                throw APIException(-43, "No valid contracts in debit tx");

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
