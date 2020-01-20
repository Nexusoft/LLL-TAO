/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/types/address.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
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
            /* The response JSON */
            json::json ret;

            /* The JSON representation of the invoice that we store in the register */
            json::json invoice;

            /* The invoice description */
            std::string strDescription;

            /* The invoice number.  NOTE even though this is referred to as a number, in practice it is an alphanumeric string */
            std::string strNumber;

            /* The invoice reference */
            std::string strReference;

            /* Sender details, which can include phone number, office address, email address */
            std::string strSenderDetail;

            /* The genesis hash of the recipient */
            uint256_t hashRecipient = 0;

            /* Recipient details, which can include phone number, office address, email address */
            std::string strRecipientDetail;

            /* The account to invoice must be paid to */
            TAO::Register::Address hashAccount ;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check for description parameter. */
            if(params.find("description") != params.end())
                strDescription = params["description"].get<std::string>();
            else
                throw APIException(-224, "Missing description");

            /* Check for number parameter. */
            if(params.find("number") != params.end())
                strNumber = params["number"].get<std::string>();
            else
                throw APIException(-225, "Missing number");
            
            /* Check for reference parameter . */
            if(params.find("reference") != params.end())
                strReference = params["reference"].get<std::string>();
            else
                throw APIException(-226, "Missing reference");

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

            /* Check the account is a NXS account */
            if(account.get<uint256_t>("token") != 0)
                throw APIException(-59, "Account to credit is not a NXS account.");

            /* Check for sender detail parameter. */
            if(params.find("sender_detail") != params.end())
                strSenderDetail = params["sender_detail"].get<std::string>();
            else
                throw APIException(-228, "Missing sender detail");

            /* Check for recipient parameter. */
            if(params.find("recipient") != params.end())
                hashRecipient.SetHex(params["recipient"].get<std::string>());
            else if(params.find("recipient_username") != params.end())
                hashRecipient = TAO::Ledger::SignatureChain::Genesis(params["recipient_username"].get<std::string>().c_str());
            else
                throw APIException(-229, "Missing recipient");

            /* Check that the destination exists. */
            if(!LLD::Ledger->HasGenesis(hashRecipient))
                throw APIException(-230, "Recipient user does not exist");

            /* Check for recipient detail parameter. */
            if(params.find("recipient_detail") != params.end())
                strRecipientDetail = params["recipient_detail"].get<std::string>();
            else
                throw APIException(-231, "Missing recipient detail");


            /* Add the invoice details to the invoice JSON */
            invoice["description"] = strDescription;
            invoice["number"] = strNumber;
            invoice["reference"] = strReference;
            invoice["account"] = hashAccount.ToString();
            invoice["sender_detail"] = strSenderDetail;
            invoice["recipient"] = hashRecipient.ToString();
            invoice["recipient_detail"] = strRecipientDetail;
            invoice["items"] = json::json::array();

            /* Parse the invoice items details */
            if(params.find("items") == params.end() || !params["items"].is_array())
                throw APIException(-232, "Missing items");
            
            /* Check items is not empty */
            json::json items = params["items"];
            if(items.empty())
                throw APIException(-233, "Invoice must include at least one item");

            /* Iterate the items */
            for(auto it = items.begin(); it != items.end(); ++it)
            {
                /* Check that the required fields have been provided*/
                if(it->find("description") == it->end())
                    throw APIException(-234, "Missing item description.");

                if(it->find("amount") == it->end())
                    throw APIException(-235, "Missing item amount.");

                if(it->find("units") == it->end())
                    throw APIException(-238, "Missing item number of units.");

                /* Parse the values out of the definition json*/
                std::string strItemDescription =  (*it)["description"].get<std::string>();
                std::string strAmount =  (*it)["amount"].get<std::string>();
                std::string strUnits =  (*it)["units"].get<std::string>();

                /* The item amount */
                double dAmount = 0;

                /* The item number of units */
                uint64_t nUnits = 0;

                /* Attempt to convert the supplied value to a double, catching argument/range exceptions */
                try
                {
                    dAmount = std::stod(strAmount) ;
                }
                catch(const std::invalid_argument& e)
                {
                    throw APIException(-236, "Invalid item amount.");
                }
                catch(const std::out_of_range& e)
                {
                    throw APIException(-236, "Invalid item amount.");
                }

                if(dAmount == 0)
                    throw APIException(-237, "Item amount must be greater than 0.");

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

                /* Add this item to the invoice JSON */
                json::json item;
                item["description"] = strItemDescription;
                item["amount"] = dAmount;
                item["units"] = nUnits;

                invoice["items"].push_back(item);
            }
            

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

            /* Then the raw data */
            ssData << invoice.dump();

            /* Submit the payload object. */
            tx[0] << (uint8_t)TAO::Operation::OP::CREATE << hashRegister << (uint8_t)TAO::Register::REGISTER::READONLY << ssData.Bytes();

            /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
            if(params.find("name") != params.end())
                tx[1] = Names::CreateName(user->Genesis(), params["name"].get<std::string>(), "", hashRegister);

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
