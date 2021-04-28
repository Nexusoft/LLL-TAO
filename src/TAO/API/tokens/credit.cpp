/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/global.h>

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
            /* Check for txid parameter. */
            if(params.find("txid") == params.end())
                throw APIException(-50, "Missing txid.");

            /* Extract some parameters from input data. */
            const TAO::Register::Address hashCredit = ExtractAddress(params, "to");
            const TAO::Register::Address hashProof  = ExtractAddress(params, "proof");

            /* Get the transaction id. */
            const uint512_t hashTx = uint512_t(params["txid"].get<std::string>());

            /* Read the debit transaction that is being credited. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-40, "Previous transaction not found.");

            /* Loop through all transactions. */
            std::vector<TAO::Operation::Contract> vContracts(0);
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& contract = tx[nContract];

                /* Reset the contract to the position of the primitive. */
                if(contract.Primitive() != TAO::Operation::OP::DEBIT)
                    continue;

                /* Get the operation byte. */
                contract.SeekToPrimitive(true); //skip our primitive

                /* Get the hashFrom from the debit transaction. */
                TAO::Register::Address hashFrom;
                contract >> hashFrom;

                /* Get the hashCredit from the debit transaction. */
                TAO::Register::Address hashTo;
                contract >> hashTo;

                /* Get the amount to respond to. */
                uint64_t nAmount = 0;
                contract >> nAmount;

                /* Check for wildcard. */
                if(hashTo == TAO::Register::WILDCARD_ADDRESS)
                {
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

                /* Check to see whether this is a simple debit to a token */
                const uint8_t nParent = objTo.Base();
                if(nParent == TAO::Register::OBJECTS::ACCOUNT)
                {
                    /* Check that the debit was made to an account that we own */
                    if(objTo.hashOwner == users->GetSession(params).GetAccount()->Genesis())
                    {
                        /* Create our new contract now. */
                        TAO::Operation::Contract contract;
                        contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                        contract << hashCredit << hashFrom << nAmount;

                        /* Push to our contract queue. */
                        vContracts.push_back(contract);
                    }
                    else
                        continue;
                }

                /* If addressed to non-standard object, this could be a tokenized debit, so do some checks to find out. */
                else if(nParent == TAO::Register::OBJECTS::NONSTANDARD)
                {
                    /* Check that the owner is a token. */
                    if(objTo.hashOwner.GetType() == TAO::Register::Address::TOKEN)
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

                    /* Read our crediting account. */
                    TAO::Register::Object objCredit;
                    if(!LLD::Register->ReadObject(hashCredit, objCredit, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Retrieve the account to debit from. */
                    TAO::Register::Object objFrom;
                    if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Check that the account being debited from is the same token type as credit. */
                    if(objFrom.get<uint256_t>("token") != objCredit.get<uint256_t>("token"))
                        throw APIException(-121, "Account to credit is not for the same token as the debit.");

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

            /* Check that output was found. */
            if(vContracts.empty())
                throw APIException(-43, "No valid contracts in debit tx.");

            /* Build a JSON response object. */
            json::json jRet;
            jRet["txid"] = BuildAndAccept(params, vContracts).ToString();

            return jRet;
        }
    }
}
