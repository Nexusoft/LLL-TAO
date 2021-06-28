/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/evaluate.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/names.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Build a response object for a transaction that was built. */
    encoding::json BuildResponse(const encoding::json& jParams, const TAO::Register::Address& hashRegister,
                             const std::vector<TAO::Operation::Contract>& vContracts)
    {
        /* Build a JSON response object. */
        encoding::json jRet;
        jRet["success"] = true; //just a little response for if using -autotx
        jRet["address"] = hashRegister.ToString();

        /* Handle passing txid if not in -autotx mode. */
        const uint512_t hashTx = BuildAndAccept(jParams, vContracts);
        if(hashTx != 0)
            jRet["txid"] = hashTx.ToString();

        return jRet;
    }


    /* Build a response object for a transaction that was built. */
    encoding::json BuildResponse(const encoding::json& jParams, const std::vector<TAO::Operation::Contract>& vContracts)
    {
        /* Build a JSON response object. */
        encoding::json jRet;
        jRet["success"] = true; //just a little response for if using -autotx

        /* Handle passing txid if not in -autotx mode. */
        const uint512_t hashTx = BuildAndAccept(jParams, vContracts);
        if(hashTx != 0)
            jRet["txid"] = hashTx.ToString();

        return jRet;
    }


    /* Builds a transaction based on a list of contracts, to be deployed as a single tx or batched. */
    uint512_t BuildAndAccept(const encoding::json& jParams, const std::vector<TAO::Operation::Contract>& vContracts)
    {
        /* Authenticate the users credentials */
        if(!Commands::Get<Users>()->Authenticate(jParams))
            throw Exception(-139, "Invalid credentials");

        /* Get the PIN to be used for this API call */
        const SecureString strPIN =
            Commands::Get<Users>()->GetPin(jParams, TAO::Ledger::PinUnlock::TRANSACTIONS);

        /* Get the session to be used for this API call */
        Session& session = Commands::Get<Users>()->GetSession(jParams);

        /* Handle auto-tx feature. */
        if(config::GetBoolArg("-autotx", false))
        {
            /* Add our contracts to the notifications queue. */
            for(const auto& tContract : vContracts)
                session.vProcessQueue->push(tContract);

            return 0;
        }

        /* Otherwise let's lock the session to generate the tx. */
        LOCK(session.CREATE_MUTEX);

        /* Create the transaction. */
        TAO::Ledger::Transaction tx;
        if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
            throw Exception(-17, "Failed to create transaction");

        /* Add the contracts. */
        for(const auto& contract : vContracts)
            tx << contract;

        /* Add the contract fees. */
        AddFee(tx); //XXX: this returns true/false if fee was added, don't think we need this since it doesn't appear to be used

        /* Execute the operations layer. */
        if(!tx.Build())
            throw Exception(-44, "Transaction failed to build");

        /* Sign the transaction. */
        if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
            throw Exception(-31, "Ledger failed to sign transaction");

        /* Execute the operations layer. */
        if(!TAO::Ledger::mempool.Accept(tx))
            throw Exception(-32, "Failed to accept");

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
                    throw TAO::API::Exception(-163, "Could not retrieve default NXS account to debit fees.");

                /* Get the address of the default account */
                hashRegister = objectDefaultName.get<uint256_t>("address");
            }

            /* Retrieve the account */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw TAO::API::Exception(-13, "Object not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw TAO::API::Exception(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT)
                throw TAO::API::Exception(-65, "Object is not an account");

            /* Check the account is a NXS account */
            if(object.get<uint256_t>("token") != 0)
                throw TAO::API::Exception(-164, "Fee account is not a NXS account.");

            /* Get the account balance */
            uint64_t nCurrentBalance = object.get<uint64_t>("balance");

            /* Check that there is enough balance to pay the fee */
            if(nCurrentBalance < nCost)
                throw TAO::API::Exception(-214, "Insufficient funds to pay fee");

            /* Add the fee contract */
            uint32_t nContractPos = tx.Size();
            tx[nContractPos] << uint8_t(TAO::Operation::OP::FEE) << hashRegister << nCost;

            return true;
        }

        return false;
    }


    /* Builds a credit contract based on given txid. */
    bool BuildCredits(const encoding::json& jParams, const uint512_t& hashTx, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Track contracts added to compare values for return */
        const uint32_t nContracts = vContracts.size();

        /* Check our transaction types. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Read the debit transaction that is being credited. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-40, "Previous transaction not found.");

            /* Loop through all transactions. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                /* Build the credit contract within the contract loop. */
                if(!BuildCredit(jParams, nContract, rContract, vContracts))
                    continue;
            }
        }
        else if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Read the debit transaction that is being credited. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-40, "Previous transaction not found.");

            /* Iterate through all TxOut's in the legacy transaction to see which are sends to a sig chain  */
            for(uint32_t nContract = 0; nContract < tx.vout.size(); ++nContract)
            {
                /* Build our contract we will operate on from the legacy output. */
                const TAO::Operation::Contract tContract = TAO::Operation::Contract(tx, nContract);

                /* Build the credit contract within the contract loop. */
                if(!BuildCredit(jParams, nContract, tContract, vContracts))
                    continue;
            }
        }
        else
            throw Exception(-41, "Invalid transaction-id");

        return (vContracts.size() > nContracts);
    }


    /* Builds a credit contract based on given contract and related parameters. */
    bool BuildCredit(const encoding::json& jParams, const uint32_t nContract, const TAO::Operation::Contract& rContract,
        std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Extract some parameters from input data. */
        const TAO::Register::Address hashCredit = ExtractAddress(jParams, "", "default");

        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Commands::Get<Users>()->GetSession(jParams).GetAccount()->Genesis();

        /* Copy our txid out of the contract. */
        const uint512_t hashTx = rContract.Hash();

        /* Reset the contract to the position of the primitive. */
        const uint8_t nPrimitive = rContract.Primitive();
        rContract.SeekToPrimitive(false); //false to skip our primitive

        /* Check for coinbase credits first. */
        if(nPrimitive == TAO::Operation::OP::COINBASE)
        {
            /* Get the genesisHash of the user who mined the coinbase*/
            uint256_t hashMiner = 0;
            rContract >> hashMiner;

            /* Check that the coinbase was mined by the caller */
            if(hashMiner != hashGenesis)
                return false;

            /* Get the amount from the coinbase transaction. */
            uint64_t nAmount = 0;
            rContract >> nAmount;

            /* Now lets check our expected types match. */
            if(!CheckStandard(jParams, hashCredit))
                return false;

            /* if we passed all of these checks then insert the credit contract into the tx */
            TAO::Operation::Contract contract;
            contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
            contract << hashCredit << hashGenesis << nAmount;

            /* Push to our contract queue. */
            vContracts.push_back(contract);

            return true;
        }

        /* Next check for debit credits. */
        else if(nPrimitive == TAO::Operation::OP::DEBIT)
        {
            /* Get the hashFrom from the debit transaction. */
            TAO::Register::Address hashFrom;
            rContract >> hashFrom;

            /* Get the hashCredit from the debit transaction. */
            TAO::Register::Address hashTo;
            rContract >> hashTo;

            /* Get the amount to respond to. */
            uint64_t nAmount = 0;
            rContract >> nAmount;

            /* Check for a legacy output debit. */
            if(hashFrom == TAO::Register::WILDCARD_ADDRESS)
            {
                /* Read our crediting account. */
                TAO::Register::Object objCredit;
                if(!LLD::Register->ReadObject(hashCredit, objCredit, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Let's check our credit account is correct token. */
                if(objCredit.get<uint256_t>("token") != 0)
                    throw Exception(-59, "Account to credit is not a NXS account");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, objCredit))
                    throw Exception(-49, "Unexpected type for name / address");

                /* If we passed these checks then insert the credit contract into the tx */
                TAO::Operation::Contract contract;
                contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                contract << hashCredit << hashFrom << nAmount;

                /* Push to our contract queue. */
                vContracts.push_back(contract);

                return true;
            }

            /* Check for wildcard for conditional contract (OP::VALIDATE). */
            if(hashTo == TAO::Register::WILDCARD_ADDRESS)
            {
                /* Check for conditions. */
                if(!rContract.Empty(TAO::Operation::Contract::CONDITIONS)) //XXX: unit tests for this scope in finance unit tests
                {
                    /* Check for condition. */
                    if(rContract.Operations()[0] == TAO::Operation::OP::CONDITION)
                    {
                        /* Read the contract database. */
                        uint256_t hashValidator = 0;
                        if(LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator))
                        {
                            /* Check that the caller is the claimant. */
                            if(hashValidator != hashGenesis)
                                return false;
                        }
                    }
                }

                /* Retrieve the account we are debiting from */
                TAO::Register::Object objFrom;
                if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Read our crediting account. */
                TAO::Register::Object objCredit;
                if(!LLD::Register->ReadObject(hashCredit, objCredit, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Let's check our credit account is correct token. */
                if(objFrom.get<uint256_t>("token") != objCredit.get<uint256_t>("token"))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, objCredit))
                    throw Exception(-49, "Unexpected type for name / address");

                /* If we passed these checks then insert the credit contract into the tx */
                TAO::Operation::Contract contract;
                contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                contract << hashCredit << hashFrom << nAmount;

                /* Push to our contract queue. */
                vContracts.push_back(contract);

                return true;
            }

            /* Get the token / account object that the debit was made to. */
            TAO::Register::Object objTo;
            if(!LLD::Register->ReadObject(hashTo, objTo, TAO::Ledger::FLAGS::MEMPOOL))
                return false;

            /* Check for standard credit to account. */
            const uint8_t nStandardBase = objTo.Base();
            if(nStandardBase == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* Check that the debit was made to an account that we own */
                if(objTo.hashOwner != hashGenesis)
                    return false;

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, objTo))
                    throw Exception(-49, "Unexpected type for name / address");

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
                const TAO::Register::Address hashProof = ExtractAddress(jParams, "proof");

                /* Check that the owner is a token. */
                if(objTo.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                    return false;

                /* Read our token contract now since we now know it's correct. */
                TAO::Register::Object objOwner;
                if(!LLD::Register->ReadObject(objTo.hashOwner, objOwner, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* We shouldn't ever evaluate to true here, but if we do let's not make things worse for ourselves. */
                if(objOwner.Standard() != TAO::Register::OBJECTS::TOKEN)
                    return false;

                /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                TAO::Register::Object objProof;
                if(!LLD::Register->ReadObject(hashProof, objProof, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Check that the proof is an account for the same token as the asset owner */
                if(objProof.get<uint256_t>("token") != objOwner.get<uint256_t>("token"))
                    throw Exception(-61, "Proof account is for a different token than the asset token.");

                /* Retrieve the account to debit from. */
                TAO::Register::Object objFrom;
                if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Read our crediting account. */
                TAO::Register::Object objCredit;
                if(!LLD::Register->ReadObject(hashCredit, objCredit, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Check that the account being debited from is the same token type as credit. */
                if(objFrom.get<uint256_t>("token") != objCredit.get<uint256_t>("token"))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, objCredit))
                    throw Exception(-49, "Unexpected type for name / address");

                /* Calculate the partial amount we want to claim based on our share of the proof tokens */
                const uint64_t nPartial = (objProof.get<uint64_t>("balance") * nAmount) / objOwner.get<uint64_t>("supply");

                /* Create our new contract now. */
                TAO::Operation::Contract contract;
                contract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                contract << hashCredit << hashProof << nPartial;

                /* Push to our contract queue. */
                vContracts.push_back(contract);

                return true;
            }
        }

        return false; //if we get this far, this contract is not creditable
    }
} // End TAO namespace
