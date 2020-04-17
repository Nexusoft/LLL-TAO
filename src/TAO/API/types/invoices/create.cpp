/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/types/address.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/user_types.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/constants.h>
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

        /* Creates a new invoice. */
        json::json Invoices::Create(const json::json& params, bool fHelp)
        {
            /* First ensure that transaction version 2 active, as the conditions required for invoices were not enabled until v2 */
            const uint32_t nCurrent = TAO::Ledger::CurrentTransactionVersion();
            if(nCurrent < 2 || (nCurrent == 2 && !TAO::Ledger::TransactionVersionActive(runtime::unifiedtimestamp(), 2)))
                throw APIException(-254, "Invoices API not yet active.");

            /* The response JSON */
            json::json ret;

            /* The JSON representation of the invoice that we store in the register */
            json::json invoice;

            /* The genesis hash of the recipient */
            uint256_t hashRecipient = 0;

            /* The account to invoice must be paid to */
            TAO::Register::Address hashAccount ;

            /* The invoice total in fractional tokens */
            double dTotal = 0;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check whether the caller has provided the account name parameter. */
            if(params.find("account_name") != params.end())
                /* If name is provided then use this to deduce the register address */
                hashAccount = Names::ResolveAddress(params, params["account_name"].get<std::string>());
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("account") != params.end())
                hashAccount.SetBase58(params["account"].get<std::string>());
            /* Fail if no required parameters supplied. */
            else
                throw APIException(-227, "Missing payment account name / address");

            /* Validate the payment account */
            /* Get the account object. */
            TAO::Register::Object account;
            if(!LLD::Register->ReadState(hashAccount, account))
                throw APIException(-13, "Account not found");

            /* Parse the account object register. */
            if(!account.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Check the object standard. */
            if(account.Standard() != TAO::Register::OBJECTS::ACCOUNT )
                throw APIException(-65, "Object is not an account");

            /* The token that this invoice should be transacted in */
            TAO::Register::Address hashToken = account.get<uint256_t>("token");

            /* The decimals for this token type */
            uint8_t nDecimals = GetDecimals(account);
            
            /* Check for recipient parameter. */
            if(params.find("recipient") != params.end())
                hashRecipient.SetHex(params["recipient"].get<std::string>());
            else if(params.find("recipient_username") != params.end())
                hashRecipient = TAO::Ledger::SignatureChain::Genesis(params["recipient_username"].get<std::string>().c_str());
            else
                throw APIException(-229, "Missing recipient");

            /* Check that the recipient genesis hash exists. If not, in client mode and we should ask a peer for the genesis */
            if(config::fClient.load() && !LLD::Ledger->HasGenesis(hashRecipient))
            {
                 /* Check tritium server enabled. */
                if(LLP::TRITIUM_SERVER)
                {
                    memory::atomic_ptr<LLP::TritiumNode>& pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        /* Request the genesis hash from the peer. */
                        debug::log(1, FUNCTION, "CLIENT MODE: Requesting GET::GENESIS for ", hashRecipient.SubString());

                        LLP::TritiumNode::BlockingMessage(10000, pNode, LLP::ACTION::GET, uint8_t(LLP::TYPES::GENESIS), hashRecipient);

                        debug::log(1, FUNCTION, "CLIENT MODE: GET::GENESIS received for ", hashRecipient.SubString());
                    }
                }

                /* Now check again after asking the peer for the genesis */
                if(!LLD::Ledger->HasGenesis(hashRecipient)) 
                    throw APIException(-230, "Recipient user does not exist");
            }
            else if(!LLD::Ledger->HasGenesis(hashRecipient))
            { 
                throw APIException(-230, "Recipient user does not exist");
            }

            /* Check that the recipient isn't the sender */
            if(hashRecipient == user->Genesis())
                throw APIException(-244, "Cannot send invoice to self");

            /* Add the mandatroy invoice fields to the invoice JSON */
            invoice["account"] = hashAccount.ToString();
            invoice["recipient"] = hashRecipient.ToString();

            /* Add all other non-mandatory fields that the caller has provided */
            for(auto it = params.begin(); it != params.end(); ++it)
            {
                /* Skip any incoming parameters that are keywords used by this API method*/
                    if(it.key() == "pin"
                    || it.key() == "PIN"
                    || it.key() == "session"
                    || it.key() == "name"
                    || it.key() == "account"
                    || it.key() == "account_name"
                    || it.key() == "recipient"
                    || it.key() == "recipient_username"
                    || it.key() == "amount"
                    || it.key() == "token"
                    || it.key() == "items")
                    {
                        continue;
                    }

                    /* add the field to the invoice */
                    invoice[it.key()] = it.value();
            }

            
            /* Parse the invoice items details */
            if(params.find("items") == params.end() || !params["items"].is_array())
                throw APIException(-232, "Missing items");
            
            /* Check items is not empty */
            json::json items = params["items"];
            if(items.empty())
                throw APIException(-233, "Invoice must include at least one item");

            /* Iterate the items to validate them*/
            for(auto it = items.begin(); it != items.end(); ++it)
            {
                /* check that mandatory fields have been provided */
                if(it->find("unit_amount") == it->end())
                    throw APIException(-235, "Missing item unit amount.");

                if(it->find("units") == it->end())
                    throw APIException(-238, "Missing item number of units.");

                /* Parse the values out of the definition json*/
                std::string strUnitAmount =  (*it)["unit_amount"].get<std::string>();
                std::string strUnits =  (*it)["units"].get<std::string>();

                /* The item Unit Amount */
                double dUnitAmount = 0;

                /* The item number of units */
                uint64_t nUnits = 0;

                /* Attempt to convert the supplied value to a double, catching argument/range exceptions */
                try
                {
                    dUnitAmount = std::stod(strUnitAmount) ;
                }
                catch(const std::invalid_argument& e)
                {
                    throw APIException(-236, "Invalid item unit amount.");
                }
                catch(const std::out_of_range& e)
                {
                    throw APIException(-236, "Invalid item unit amount.");
                }

                if(dUnitAmount == 0)
                    throw APIException(-237, "Item unit amount must be greater than 0.");

                /* Attempt to convert the supplied value to a 64-bit unsigned integer, catching argument/range exceptions */
                try
                {
                    nUnits = std::stoull(strUnits);
                }
                catch(const std::invalid_argument& e)
                {
                    throw APIException(-239, "Invalid item number of units.");
                }
                catch(const std::out_of_range& e)
                {
                    throw APIException(-239, "Invalid item number of units.");
                }

                if(nUnits == 0)
                    throw APIException(-240, "Item units must be greater than 0.");

                /* work out the total for this item */
                double dItemTotal = dUnitAmount * nUnits;

                /* add this to the invoice total */
                dTotal += dItemTotal;

                /* Once we have validated the mandatory fields, add this item to the invoice JSON */
                invoice["items"].push_back(*it);
            }

            /* Add the invoice amount and token */
            invoice["amount"] = dTotal;
            invoice["token"] = hashToken.ToString();

            /* Once validated, add the items to the invoice */
            invoice["items"] = items;

            /* Calculate the amount to pay in token units */
            uint64_t nTotal = dTotal * pow(10, nDecimals);
        
        
            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Generate a random hash for this objects register address */
            TAO::Register::Address hashRegister = TAO::Register::Address(TAO::Register::Address::READONLY);

            /* name of the object, default to blank */
            std::string strName = "";

            /* DataStream to help us serialize the data. */
            DataStream ssData(SER_REGISTER, 1);

            /* First add the leading 2 bytes to identify the state data */
            ssData << (uint16_t) USER_TYPES::INVOICE;

            /* Then the raw data */
            ssData << invoice.dump();

            /* Check the data size */
            if(ssData.size() > TAO::Register::MAX_REGISTER_SIZE)
                throw APIException(-242, "Data exceeds max register size");

            /* The contract ID being worked on */
            uint32_t nContract = 0;

            /* Add the invoice creation contract. */
            tx[nContract++] << (uint8_t)TAO::Operation::OP::CREATE << hashRegister << (uint8_t)TAO::Register::REGISTER::READONLY << ssData.Bytes();

            /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
            if(params.find("name") != params.end())
                tx[nContract++] = Names::CreateName(user->Genesis(), params["name"].get<std::string>(), "", hashRegister);

            /* Add the transfer contract */
            tx[nContract] << uint8_t(TAO::Operation::OP::CONDITION) << (uint8_t)TAO::Operation::OP::TRANSFER << hashRegister << hashRecipient << uint8_t(TAO::Operation::TRANSFER::CLAIM);
            
            /* Add the payment conditions.  The condition is essentially that the claim must include a conditional debit for the
               invoice total being made to the payment account */
            TAO::Operation::Stream compare;
            compare << uint8_t(TAO::Operation::OP::DEBIT) << uint256_t(0) << hashAccount << nTotal << uint64_t(0);

            /* Conditions. */
            tx[nContract] <= uint8_t(TAO::Operation::OP::GROUP);
            tx[nContract] <= uint8_t(TAO::Operation::OP::CALLER::OPERATIONS) <= uint8_t(TAO::Operation::OP::CONTAINS);
            tx[nContract] <= uint8_t(TAO::Operation::OP::TYPES::BYTES) <= compare.Bytes();
            tx[nContract] <= uint8_t(TAO::Operation::OP::AND);
            tx[nContract] <= uint8_t(TAO::Operation::OP::CALLER::PRESTATE::VALUE) <= std::string("token");
            tx[nContract] <= uint8_t(TAO::Operation::OP::EQUALS);
            tx[nContract] <= uint8_t(TAO::Operation::OP::TYPES::UINT256_T) <= hashToken; // ensure the token paid matches the invoice
            tx[nContract] <= uint8_t(TAO::Operation::OP::UNGROUP);

            tx[nContract] <= uint8_t(TAO::Operation::OP::OR);

            tx[nContract] <= uint8_t(TAO::Operation::OP::GROUP);
            tx[nContract] <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
            tx[nContract] <= uint8_t(TAO::Operation::OP::EQUALS);
            tx[nContract] <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);
            tx[nContract] <= uint8_t(TAO::Operation::OP::UNGROUP);

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-30, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }
    }
}
