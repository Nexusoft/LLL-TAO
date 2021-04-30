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

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/trust.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>

/* Global TAO namespace. */
namespace TAO::API
{
    json::json Finance::Credit(const json::json& params, bool fHelp)
    {
        /* Check for txid parameter. */
        if(params.find("txid") == params.end())
            throw APIException(-50, "Missing txid.");

        /* Extract some parameters from input data. */
        const TAO::Register::Address hashCredit = ExtractAddress(params, "", "default");

        /* Get the transaction id. */
        const uint512_t hashTx =
            uint512_t(params["txid"].get<std::string>());

        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            users->GetSession(params).GetAccount()->Genesis();

        /* Check for tritium credits. */
        std::vector<TAO::Operation::Contract> vContracts(0);
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Read the debit transaction that is being credited. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-40, "Previous transaction not found.");

            /* Loop through all transactions. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& debit = tx[nContract];

                /* Reset the contract to the position of the primitive. */
                const uint8_t nPrimitive = debit.Primitive();
                debit.SeekToPrimitive(false); //false to skip our primitive

                /* Check for coinbase credits first. */
                if(nPrimitive == TAO::Operation::OP::COINBASE)
                {
                    /* Get the genesisHash of the user who mined the coinbase*/
                    uint256_t hashMiner = 0;
                    debit >> hashMiner;

                    /* Check that the coinbase was mined by the caller */
                    if(hashMiner != hashGenesis)
                        continue;

                    /* Get the amount from the coinbase transaction. */
                    uint64_t nAmount = 0;
                    debit >> nAmount;

                    /* Now lets check our expected types match. */
                    CheckType(params, hashCredit);

                    /* if we passed all of these checks then insert the credit contract into the tx */
                    TAO::Operation::Contract contract;
                    contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                    contract << hashCredit << hashGenesis << nAmount;

                    /* Push to our contract queue. */
                    vContracts.push_back(contract);

                    continue;
                }

                /* Next check for debit credits. */
                else if(nPrimitive == TAO::Operation::OP::DEBIT)
                {
                    /* Get the hashFrom from the debit transaction. */
                    TAO::Register::Address hashFrom;
                    debit >> hashFrom;

                    /* Get the hashCredit from the debit transaction. */
                    TAO::Register::Address hashTo;
                    debit >> hashTo;

                    /* Get the amount to respond to. */
                    uint64_t nAmount = 0;
                    debit >> nAmount;

                    /* Check for wildcard for conditional contract (OP::VALIDATE). */
                    if(hashTo == TAO::Register::WILDCARD_ADDRESS)
                    {
                        /* Check for conditions. */
                        if(!debit.Empty(TAO::Operation::Contract::CONDITIONS)) //XXX: unit tests for this scope in finance unit tests
                        {
                            /* Check for condition. */
                            if(debit.Operations()[0] == TAO::Operation::OP::CONDITION)
                            {
                                /* Read the contract database. */
                                uint256_t hashValidator = 0;
                                if(LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator))
                                {
                                    /* Check that the caller is the claimant. */
                                    if(hashValidator != hashGenesis)
                                        continue;
                                }
                            }
                        }

                        /* Retrieve the account we are debiting from */
                        TAO::Register::Object objFrom;
                        if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Read our crediting account. */
                        TAO::Register::Object objCredit;
                        if(!LLD::Register->ReadObject(hashCredit, objCredit, TAO::Ledger::FLAGS::MEMPOOL))
                            throw APIException(-33, "Incorrect or missing name / address");

                        /* Let's check our credit account is correct token. */
                        if(objFrom.get<uint256_t>("token") != objCredit.get<uint256_t>("token"))
                            throw APIException(-33, "Incorrect or missing name / address");

                        /* Now lets check our expected types match. */
                        CheckType(params, objCredit);

                        /* If we passed these checks then insert the credit contract into the tx */
                        TAO::Operation::Contract contract;
                        contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                        contract << hashCredit << hashFrom << nAmount;

                        /* Push to our contract queue. */
                        vContracts.push_back(contract);

                        continue;
                    }

                    /* Get the token / account object that the debit was made to. */
                    TAO::Register::Object objTo;
                    if(!LLD::Register->ReadObject(hashTo, objTo, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Check for standard credit to account. */
                    const uint8_t nStandardBase = objTo.Base();
                    if(nStandardBase == TAO::Register::OBJECTS::ACCOUNT)
                    {
                        /* Check that the debit was made to an account that we own */
                        if(objTo.hashOwner != hashGenesis)
                            continue;

                        /* Now lets check our expected types match. */
                        CheckType(params, objTo);

                        /* Create our new contract now. */
                        TAO::Operation::Contract contract;
                        contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                        contract << hashTo << hashFrom << nAmount; //we use hashTo from our debit contract instead of hashCredit

                        /* Push to our contract queue. */
                        vContracts.push_back(contract);
                    }

                    /* If addressed to non-standard object, this could be a tokenized debit, so do some checks to find out. */
                    else if(nStandardBase == TAO::Register::OBJECTS::NONSTANDARD)
                    {
                        /* Attempt to get the proof from the parameters. */
                        const TAO::Register::Address hashProof = ExtractAddress(params, "proof");

                        /* Check that the owner is a token. */
                        if(objTo.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                            continue;

                        /* Read our token contract now since we now know it's correct. */
                        TAO::Register::Object objOwner;
                        if(!LLD::Register->ReadObject(objTo.hashOwner, objOwner, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* We shouldn't ever evaluate to true here, but if we do let's not make things worse for ourselves. */
                        if(objOwner.Standard() != TAO::Register::OBJECTS::TOKEN)
                            continue;

                        /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                        TAO::Register::Object objProof;
                        if(!LLD::Register->ReadObject(hashProof, objProof, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Check that the proof is an account for the same token as the asset owner */
                        if(objProof.get<uint256_t>("token") != objOwner.get<uint256_t>("token"))
                            throw APIException(-61, "Proof account is for a different token than the asset token.");

                        /* Retrieve the account to debit from. */
                        TAO::Register::Object objFrom;
                        if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Read our crediting account. */
                        TAO::Register::Object objCredit;
                        if(!LLD::Register->ReadObject(hashCredit, objCredit, TAO::Ledger::FLAGS::MEMPOOL))
                            throw APIException(-33, "Incorrect or missing name / address");

                        /* Check that the account being debited from is the same token type as credit. */
                        if(objFrom.get<uint256_t>("token") != objCredit.get<uint256_t>("token"))
                            throw APIException(-33, "Incorrect or missing name / address");

                        /* Now lets check our expected types match. */
                        CheckType(params, objCredit);

                        /* Calculate the partial amount we want to claim based on our share of the proof tokens */
                        const uint64_t nPartial = (objProof.get<uint64_t>("balance") * nAmount) / objOwner.get<uint64_t>("supply");

                        /* Create our new contract now. */
                        TAO::Operation::Contract contract;
                        contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                        contract << hashCredit << hashProof << nPartial;

                        /* Push to our contract queue. */
                        vContracts.push_back(contract);
                    }
                }
            }
        }
        else if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Read the debit transaction that is being credited. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-40, "Previous transaction not found.");

            /* Iterate through all TxOut's in the legacy transaction to see which are sends to a sig chain  */
            for(uint32_t nContract = 0; nContract < tx.vout.size(); ++nContract)
            {
                /* The TxOut to be checked */
                const Legacy::TxOut& txout = tx.vout[nContract];

                /* Extract the sig chain account register address from the legacy script */
                TAO::Register::Address hashTo;
                if(!Legacy::ExtractRegister(txout.scriptPubKey, hashTo))
                    continue;

                /* Get the token / account object that the debit was made to. */
                TAO::Register::Object objTo;
                if(!LLD::Register->ReadObject(hashTo, objTo))
                    continue;

                /* Check for the owner to make sure this was a send to the current users account */
                const uint8_t nStandard = objTo.Base();
                if(objTo.hashOwner != hashGenesis || nStandard != TAO::Register::OBJECTS::ACCOUNT)
                    continue;

                /* Now lets check our expected types match. */
                CheckType(params, objTo);

                /* Build the response contract now. */
                TAO::Operation::Contract contract;
                contract << uint8_t(TAO::Operation::OP::CREDIT)  << hashTx << uint32_t(nContract);
                contract << hashCredit << TAO::Register::WILDCARD_ADDRESS << uint64_t(txout.nValue);

                /* Push to our contract queue. */
                vContracts.push_back(contract);
            }
        }
        else
            throw APIException(-41, "Invalid transaction-id");

        /* Check that output was found. */
        if(vContracts.empty())
            throw APIException(-43, "No valid contracts in debit tx.");

        /* Build response JSON boilerplate. */
        return BuildResponse(params, hashCredit, vContracts);
    }
}
