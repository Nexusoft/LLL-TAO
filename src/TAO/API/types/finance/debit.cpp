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
#include <TAO/API/include/conditions.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Ledger/include/chainstate.h>
#include <Legacy/wallet/wallet.h>

#include <Util/templates/datastream.h>
#include <Util/include/string.h>

using namespace TAO::Operation;

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Debit a NXS account. */
        json::json Finance::Debit(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for credit parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-46, "Missing amount");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-16, "Account has not been unlocked for transactions");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction.");

            /* The register address of the recipient acccount. */
            TAO::Register::Address hashTo;

            /* legacy recipient address if this is a sig chain to UTXO transaction */
            Legacy::NexusAddress legacyAddress;

            /* Flag to indicate this is a sig chain to UTXO transaction */
            bool fLegacy = false;

            /* If name_to is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name_to") != params.end())
                hashTo = Names::ResolveAddress(params, params["name_to"].get<std::string>());
            else if(params.find("address_to") != params.end())
            {
                std::string strAddressTo = params["address_to"].get<std::string>();
                
                /* Decode the base58 register address */
                if(IsRegisterAddress(strAddressTo))
                    hashTo.SetBase58(strAddressTo);

                /* Check that it is valid */
                if(!hashTo.IsValid())
                    throw APIException(-165, "Invalid address_to");

                /* Check to see if it is a legacy address */
                if(hashTo.IsLegacy())
                {
                    legacyAddress.SetString(strAddressTo);
                    fLegacy = true;
                }

                if( hashTo == 0 && !legacyAddress.IsValid())
                    throw APIException(-165, "Invalid address_to");
            }
            else
                throw APIException(-64, "Missing recipient account name_to / address_to");

            /* The account to debit from. */
            TAO::Register::Address hashFrom;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end())
                hashFrom = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashFrom.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashFrom, object))
                throw APIException(-13, "Account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                throw APIException(-65, "Object is not an account");

            /* Check the account is a NXS account */
            if(object.get<uint256_t>("token") != 0)
                throw APIException(-66, "Account is not a NXS account.  Please use the tokens API for debiting non-NXS token accounts.");


            uint8_t nDecimals = TAO::Ledger::NXS_DIGITS;
            uint64_t nCurrentBalance = object.get<uint64_t>("balance");

            /* Get the amount to debit. */
            uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * pow(10, nDecimals);

            /* Check the amount is not too small once converted by the token Decimals */
            if(nAmount == 0)
                throw APIException(-68, "Amount too small");

            /* Check they have the required funds */
            if(nAmount > nCurrentBalance)
                throw APIException(-69, "Insufficient funds");

            /* The optional payment reference */
            uint64_t nReference = 0;
            if(params.find("reference") != params.end())
            {
                /* The reference as a string */
                std::string strReference = params["reference"].get<std::string>();

                /* Check that the reference contains only numeric characters before attempting to convert it */
                if(!IsAllDigit(strReference) || !IsUINT64(strReference))
                    throw APIException(-167, "Invalid reference");

                /* Convert the reference to uint64 */
                nReference = stoull(strReference);
            }
            
            /* Build the transaction payload object. */
            if(fLegacy)
            {
                /* legacy payload */
                Legacy::Script script;
                script.SetNexusAddress(legacyAddress);

                tx[0] << (uint8_t)OP::LEGACY << hashFrom << nAmount << script;
            }
            else
            {
                /* Build the OP:DEBIT */
                tx[0] << (uint8_t)OP::DEBIT << hashFrom << hashTo << nAmount << nReference;
            
                /* Add expiration condition if caller has passed an expires value */
                if(params.find("expires") != params.end())
                    AddExpires( params, user->Genesis(), tx[0]);
            }

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* If this is a legacy transaction then we need to make sure it shows in the legacy wallet */
            if(fLegacy)
            {
                TAO::Ledger::BlockState notUsed;
                Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, notUsed, true);
            }
            /* Build a JSON response object. */
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
