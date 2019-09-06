/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Claim a transferred asset. */
        json::json Objects::Claim(const json::json& params, uint8_t nType, const std::string& strType)
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

            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction.");

            /* Read the Transfer transaction being claimed. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(hashTx, txPrev))
                throw APIException(-99, "Transfer transaction not found.");

            /* Check to see whether they have provided a new name */
            std::string strName;
            if(params.find("name") != params.end())
                strName = params["name"].get<std::string>();

            /* Declare json object to store the objects that were claimed */
            json::json jsonClaimed = json::json::array();

            /* Loop through all transactions. */
            int32_t nCurrent = -1;
            for(uint32_t nContract = 0; nContract < txPrev.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& contract = txPrev[nContract];

                /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                contract.Reset();

                /* Get the operation byte. */
                uint8_t nOP = 0;
                contract >> nOP;

                /* Check type. */
                if(nOP != TAO::Operation::OP::TRANSFER)
                    continue;

                /* Get the address of the asset being transferred from the transaction. */
                TAO::Register::Address hashAddress;
                contract >> hashAddress;

                /* Get the genesis hash (recipient) of the transfer*/
                uint256_t hashGenesis = 0;
                contract >> hashGenesis;

                /* Read the force transfer flag */
                uint8_t nForceFlag = 0;
                contract >> nForceFlag;

                /* Ensure that this transfer was meant for this user or that we are claiming back our own transfer */
                if(hashGenesis != tx.hashGenesis && tx.hashGenesis != txPrev.hashGenesis)
                    continue;

                /* Ensure this wasn't a forced transfer (which requires no Claim) */
                if(nForceFlag == TAO::Operation::TRANSFER::FORCE)
                    continue;

                /* Ensure that the object being transferred is an object */
                /* Get the object from the register DB.   */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-104, "Object not found");

                /* Only include raw and non-standard object types */
                if(object.nType != TAO::Register::REGISTER::OBJECT
                && object.nType != TAO::Register::REGISTER::APPEND
                && object.nType != TAO::Register::REGISTER::RAW)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(object.nType == TAO::Register::REGISTER::OBJECT
                && (!object.Parse() || object.Standard() != nType))
                    continue;

                /* Submit the payload object. */
                tx[++nCurrent] << (uint8_t)TAO::Operation::OP::CLAIM << hashTx << uint32_t(nContract) << hashAddress;

                /* Add the address to the return JSON */
                jsonClaimed.push_back(hashAddress.ToString() );

                /* Create a name object for the claimed object unless this is a Name or Namespace already */
                if(nType != TAO::Register::OBJECTS::NAME && nType != TAO::Register::OBJECTS::NAMESPACE)
                {
                    /* Declare to contract to create new name */
                    TAO::Operation::Contract nameContract;

                    /* If the caller has passed in a name then create a name record using the new name */
                    if(!strName.empty())
                        nameContract = Names::CreateName(user->Genesis(), strName, "", hashAddress);

                    /* Otherwise create a new name from the previous owners name */
                    else
                        nameContract = Names::CreateName(user->Genesis(), params, hashTx);

                    /* If the Name contract operation was created then add it to the transaction */
                    if(!nameContract.Empty())
                        tx[++nCurrent] = nameContract;
                }
            }

            /* Error if no valid contracts found */
            if(nCurrent == -1)
                throw APIException(-152, "Transaction contains no valid transfers.");

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
            ret["claimed"]  = jsonClaimed;
            ret["txid"]  = tx.GetHash().ToString();

            return ret;
        }
    }
}
