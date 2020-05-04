/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/invoices.h>
#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/user_types.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Transfers an item. */
        json::json Invoices::Cancel(const json::json& params, bool fHelp)
        {
            /* First ensure that transaction version 2 active, as the conditions required for invoices were not enabled until v2 */
            const uint32_t nCurrent = TAO::Ledger::CurrentTransactionVersion();
            if(nCurrent < 2 || (nCurrent == 2 && !TAO::Ledger::TransactionVersionActive(runtime::unifiedtimestamp(), 2)))
                throw APIException(-254, "Invoices API not yet active.");

            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID.");

            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");

            /* Get the invoice object register . */
            TAO::Register::State state;
            if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-241, "Invoice not found");

            /* Ensure that it is an invoice register */
            if(state.nType != TAO::Register::REGISTER::READONLY)
                throw APIException(-242, "Data at this address is not an invoice");

            /* Deserialize the leading byte of the state data to check the data type */
            uint16_t type;
            state >> type;

            /* Check that the state is an invoice */
            if(type != USER_TYPES::INVOICE)
                throw APIException(-242, "Data at this address is not an invoice");

            /* Deserialize the invoice */
            json::json invoice = InvoiceToJSON(params, state, hashRegister);

            /* The recipient genesis hash */
            uint256_t hashRecipient;

            /* Deserialize the recipient hash from the invoice data */
            hashRecipient.SetHex(invoice["recipient"].get<std::string>());

            /* Get the invoice status so that we can validate that we are allowed to cancel it */
            std::string strStatus = get_status(state, hashRecipient);

            /* Validate the current invoice status */
            if(strStatus == "PAID")
                throw APIException(-245, "Cannot cancel an invoice that has already been paid");
            else if(strStatus == "CANCELLED")
                throw APIException(-246, "Cannot cancel an invoice that has already been cancelled");

            /* The transaction ID to cancel */
            uint512_t hashTx;

            /* The contract ID to cancel */
            uint32_t nContract = 0;

            /* Look up the transaction ID & contract ID of the transfer so that we can void it */
            if(!get_tx(hashRecipient, hashRegister, hashTx, nContract))
                throw APIException(-247, "Could not find invoice transfer transaction");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* The transaction to be voided */
            TAO::Ledger::Transaction txVoid;

            /* Read the debit transaction. */
            if(LLD::Ledger->ReadTx(hashTx, txVoid))
            {
                /* Check that the transaction belongs to the caller */
                if( txVoid.hashGenesis != user->Genesis())
                    throw APIException(-172, "Cannot void a transaction that does not belong to you.");

                /* Process the contract and attempt to void it */
                TAO::Operation::Contract voidContract;

                if(VoidContract(txVoid[nContract], nContract, voidContract))
                    tx[tx.Size()] = voidContract;
            }
            else
            {
                throw APIException(-40, "Previous transaction not found.");
            }


            /* Check that output was found. */
            if(tx.Size() == 0)
                throw APIException(-174, "Transaction contains no contracts that can be voided");

            /* Add the fee */
            BuildWithFee(tx);

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
