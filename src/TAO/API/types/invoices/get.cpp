/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/invoices.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/include/user_types.h>

#include <TAO/Operation/include/enum.h>

#include <LLD/include/global.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Get's the description of an item. */
        json::json Invoices::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

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

            /* Build the response JSON. */
            /* Look up the object name based on the Name records in the caller's sig chain */
            std::string strName = Names::ResolveName(users->GetCallersGenesis(params), hashRegister);

            /* Add the name to the response if one is found. */
            if(!strName.empty())
                ret["name"] = strName;

            /* Add standard register details */
            ret["address"]    = hashRegister.ToString();
            ret["created"]    = state.nCreated;
            ret["modified"]   = state.nModified;
            ret["owner"]      = TAO::Register::Address(state.hashOwner).ToString();

            /* Deserialize the invoice data */
            std::string strJSON;
            state >> strJSON;

            /* parse the serialized invoice JSON so that we can easily add the fields to the response */
            json::json invoice = json::json::parse(strJSON);

            /* Add each of the invoice fields to the response */
            for(auto it = invoice.begin(); it != invoice.end(); ++it)
                ret[it.key()] = it.value();

            /* Get the recipient genesis hash from the invoice data so that we can use it to calculate the status*/
            uint256_t hashRecipient;
            hashRecipient.SetHex(invoice["recipient"].get<std::string>());

            /* Add status */
            ret["status"] = get_status(state, hashRegister, hashRecipient);

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }


        /* Returns a status for the invoice (draft/outstanding/paid/cancelled) */ 
        std::string Invoices::get_status(const TAO::Register::State& state, const TAO::Register::Address& hashInvoice, 
                               const uint256_t& hashRecipient)
        {
            /* The return value string */
            std::string strStatus;

            /* flag to indicate a transfer transaction has occurred */
            bool fTransfer = false;

            /* flag to indicate a claim transaction has occurred */
            bool fClaim = false;

            /* Genesis hash of the invoice creator, as determined from the OP::CREATE transaction */
            uint256_t hashCreator = 0;


            /* Read the last hash of owner. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(state.hashOwner, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-107, "No history found");

            /* Iterate through sigchain for register updates. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

                /* Check through all the contracts. */
                for(int32_t nContract = tx.Size() - 1; nContract >= 0; --nContract)
                {
                    /* Get the contract. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                    contract.Reset();

                    /* Get the operation byte. */
                    uint8_t OPERATION = 0;
                    contract >> OPERATION;

                    /* Check for key operations. */
                    switch(OPERATION)
                    {
                        case TAO::Operation::OP::CREATE:
                        {
                            /* Get the Address of the Register. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashInvoice)
                                break;
                            
                            /* Set the address of the hashCreator */
                            hashCreator = contract.Caller();

                            break;

                        }
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nContract = 0;
                            contract >> nContract;

                            /* Extract the address from the contract. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashInvoice)
                                break;

                            /* Set claim flag */
                            fClaim = true;

                            break;
                        }

                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the contract. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashInvoice)
                                break;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer;
                            contract >> hashTransfer;

                            /* Check to see if the transfer recipient is the invoice recipient */
                            if(hashTransfer == hashRecipient)
                                fTransfer = true;

                            break;
                        }
                    }
                }

                /* If we have found the CREATE transaction then  we can break out of processing as we have enough info */
                if(hashCreator != 0)
                    break;

            }

            /* Calculate the status */
            if(!fTransfer)
                strStatus = "DRAFT"; // never transferred so must be a draft
            else if(fClaim && state.hashOwner == hashRecipient)
                strStatus = "PAID"; // owned by invoice recipient so must have been paid
            else if(fClaim && state.hashOwner != 0)
                strStatus = "CANCELLED"; // owned by someone other than the recipient so must be cancelled
            else
                strStatus = "OUTSTANDING"; // no current owner so has been transferred but not claimed (outstanding)

            return strStatus;
        }
    }
}
