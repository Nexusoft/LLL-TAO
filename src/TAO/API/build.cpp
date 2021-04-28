/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/build.h>
#include <TAO/API/include/global.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/names.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Extract an address from incoming parameters to derive from name or address field. */
    uint256_t ExtractAddress(const json::json& params, const std::string strSuffix)
    {
        /* Cache a couple keys we will be using. */
        const std::string strName = "name"    + (strSuffix.empty() ? ("") : ("_" + strSuffix));
        const std::string strAddr = "address" + (strSuffix.empty() ? ("") : ("_" + strSuffix));;

        /* If name is provided then use this to deduce the register address, */
        if(params.find(strName) != params.end())
        {
            /* Check for the ALL name, that debits from all relevant accounts. */
            if(params[strName] == "ALL")
            {
                /* Check for send to all */
                if(strSuffix == "to")
                    throw APIException(-310, "Cannot sent to ALL accounts");

                return TAO::Register::WILDCARD_ADDRESS; //placeholder for ALL functionality 0xffffff.....ffffff
            }

            return Names::ResolveAddress(params, params[strName].get<std::string>());
        }

        /* Otherwise let's check for the raw address format. */
        else if(params.find(strAddr) != params.end())
        {
            /* Declare our return value. */
            const TAO::Register::Address hashRet =
                TAO::Register::Address(params[strAddr].get<std::string>());

            /* Check that it is valid */
            if(!hashRet.IsValid())
                throw APIException(-165, "Invalid " + strAddr);

            return hashRet;
        }

        /* This exception is for name_to/address_to */
        else if(strSuffix == "to")
            throw APIException(-64, "Missing recipient account name_to / address_to");

        /* This exception is for name_proof/address_proof */
        else if(strSuffix == "proof")
            throw APIException(-54, "Missing name_proof / address_proof to credit");

        /* This exception is for name/address */
        throw APIException(-33, "Missing name / address");
    }


    /* Builds a transaction based on a list of contracts, to be deployed as a single tx or batched. */
    uint512_t BuildAndAccept(const json::json& params, const std::vector<TAO::Operation::Contract>& vContracts)
    {
        /* Authenticate the users credentials */
        if(!users->Authenticate(params))
            throw APIException(-139, "Invalid credentials");

        /* Get the PIN to be used for this API call */
        const SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

        /* Get the session to be used for this API call */
        const Session& session = users->GetSession(params);
        LOCK(session.CREATE_MUTEX);

        /* Create the transaction. */
        TAO::Ledger::Transaction tx;
        if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
            throw APIException(-17, "Failed to create transaction");

        /* Add the contracts. */
        for(const auto& contract : vContracts)
            tx << contract;

        /* Add the contract fees. */
        AddFee(tx); //XXX: this returns true/false if fee was added, don't think we need this since it doesn't appear to be used

        /* Execute the operations layer. */
        if(!tx.Build())
            throw APIException(-44, "Transaction failed to build");

        /* Sign the transaction. */
        if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
            throw APIException(-31, "Ledger failed to sign transaction");

        /* Execute the operations layer. */
        if(!TAO::Ledger::mempool.Accept(tx))
            throw APIException(-32, "Failed to accept");

        //TODO: we want to add a localdb index here, so it can be re-broadcast on restart
        return tx.GetHash();
    }


    /* Calculates the required fee for the transaction and adds the OP::FEE contract to the transaction if necessary.
     *  If a specified fee account is not specified, the method will lookup the "default" NXS account and use this account
     *  to pay the fees.  An exception will be thrownIf there are insufficient funds to pay the fee. */
    bool AddFee(TAO::Ledger::Transaction& tx, const TAO::Register::Address& hashFeeAccount)
    {
        /* First we need to ensure that the transaction is built so that the contracts have their pre states */
        tx.Build();

        /* Obtain the transaction cost */
        uint64_t nCost = tx.Cost();

        /* If a fee needs to be applied then add it */
        if(nCost > 0)
        {
            /* The register adddress of the account to deduct fees from */
            TAO::Register::Address hashRegister;

            /* If the caller has specified a fee account to use then use this */
            if(hashFeeAccount.IsValid() && hashFeeAccount.IsAccount())
            {
                hashRegister = hashFeeAccount;
            }
            else
            {
                /* Otherwise we need to look up the default fee account */
                TAO::Register::Object objectDefaultName;

                if(!TAO::Register::GetNameRegister(tx.hashGenesis, std::string("default"), objectDefaultName))
                    throw TAO::API::APIException(-163, "Could not retrieve default NXS account to debit fees.");

                /* Get the address of the default account */
                hashRegister = objectDefaultName.get<uint256_t>("address");
            }

            /* Retrieve the account */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw TAO::API::APIException(-13, "Account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw TAO::API::APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT)
                throw TAO::API::APIException(-65, "Object is not an account");

            /* Check the account is a NXS account */
            if(object.get<uint256_t>("token") != 0)
                throw TAO::API::APIException(-164, "Fee account is not a NXS account.");

            /* Get the account balance */
            uint64_t nCurrentBalance = object.get<uint64_t>("balance");

            /* Check that there is enough balance to pay the fee */
            if(nCurrentBalance < nCost)
                throw TAO::API::APIException(-214, "Insufficient funds to pay fee");

            /* Add the fee contract */
            uint32_t nContractPos = tx.Size();
            tx[nContractPos] << uint8_t(TAO::Operation::OP::FEE) << hashRegister << nCost;

            return true;
        }

        return false;
    }
} // End TAO namespace
