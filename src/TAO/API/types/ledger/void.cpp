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

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Reverses a debit or transfer transaction that the caller has made */
        json::json Ledger::VoidTransaction(const json::json& params, bool fHelp)
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
            
            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* The transaction to be voided */
            TAO::Ledger::Transaction txVoid;
            
            /* Read the debit transaction. */
            if(LLD::Ledger->ReadTx(hashTx, txVoid))
            { 
                /* Check that the transaction belongs to the caller */
                if( txVoid.hashGenesis != user->Genesis())
                    throw APIException(-172, "Cannot void a transaction that does not belong to you.");

                /* Loop through all transactions. */
                int32_t nCurrent = -1;
                for(uint32_t nContract = 0; nContract < txVoid.Size(); ++nContract)
                {
                    /* Get the contract. */
                    const TAO::Operation::Contract& contract = txVoid[nContract];

                    /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                    contract.Reset();

                    /* Get the operation byte. */
                    uint8_t nType = 0;
                    contract >> nType;

                    /* Ensure that it is a debit or transfer */
                    if(nType != TAO::Operation::OP::DEBIT && nType != TAO::Operation::OP::TRANSFER)
                        continue;

                    /* Process crediting a debit */
                    if(nType == TAO::Operation::OP::DEBIT)
                    {
                        /* Get the hashFrom from the debit transaction. This is the account we are going to return the credit to*/
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

                        /* Check to see whether there are any partial credits already claimed against the debit */
                        uint64_t nClaimed = 0;
                        if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL))
                            nClaimed = 0; 

                        /* Check that there is something to be claimed */
                        if(nClaimed == nAmount)
                            throw APIException(-173, "Cannot void debit transaction as it has already been fully credited by all recipients");

                        /* Reduce the amount to credit by the amount already claimed */
                        nAmount -= nClaimed;

                        /* Insert the credit contract into the tx */
                        tx[++nCurrent] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashFrom <<  hashFrom << nAmount;
                        
                    }
                    /* Process voiding a transfer */
                    else if(nType == TAO::Operation::OP::TRANSFER)
                    {
                        /* Get the address of the asset being transferred from the transaction. */
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* Get the genesis hash (recipient) of the transfer*/
                        uint256_t hashGenesis = 0;
                        contract >> hashGenesis;

                        /* Read the force transfer flag */
                        uint8_t nForceFlag = 0;
                        contract >> nForceFlag;

                        /* Ensure this wasn't a forced transfer (which requires no Claim) */
                        if(nForceFlag == TAO::Operation::TRANSFER::FORCE)
                            continue;

                        /* Insert the claim contract into the tx. */
                        tx[++nCurrent] << (uint8_t)TAO::Operation::OP::CLAIM << hashTx << uint32_t(nContract) << hashAddress;
                    }
                    else
                    {
                        // skip this contract as it is not a debit or transfer
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
                throw APIException(-174, "Transaction contains no contracts that can be voided");

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
