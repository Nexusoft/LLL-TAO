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
#include <TAO/API/include/conditions.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/names.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <LLD/include/global.h>

#include <Util/include/convert.h>
#include <Util/include/math.h>

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
        /* Handle auto-tx feature. */
        if(config::GetBoolArg("-autotx", false)) //TODO: pipe in -autotx
        {
            return 0;
        }

        /* Let's check our contract size isn't out of bounds. */
        if(vContracts.size() >= 99)
            throw Exception(-120, "Maximum number of contracts exceeded (99), please try again or use -autotx mode.");

        /* The new key scheme */
        const uint8_t nScheme =
            ExtractScheme(jParams, "brainpool, falcon");

        /* The PIN to be used for this API call */
        SecureString strPIN;

        /* Unlock grabbing the pin, while holding a new authentication lock */
        RECURSIVE(Authentication::Unlock(jParams, strPIN, TAO::Ledger::PinUnlock::TRANSACTIONS));

        /* Get an instance of our credentials. */
        const auto& pCredentials =
            Authentication::Credentials(jParams);

        /* Create the transaction. */
        TAO::Ledger::Transaction tx;
        if(!TAO::Ledger::CreateTransaction(pCredentials, strPIN, tx, nScheme))
            throw Exception(-17, "Failed to create transaction");

        /* Add the contracts. */
        for(const auto& rContract : vContracts)
            tx << rContract;

        /* Add the contract fees. */
        AddFee(tx); //XXX: this returns true/false if fee was added, don't think we need this since it doesn't appear to be used

        /* Execute the operations layer. */
        if(!tx.Build())
            throw Exception(-44, "Transaction failed to build");

        /* Sign the transaction. */
        if(!tx.Sign(pCredentials->Generate(tx.nSequence, strPIN)))
            throw Exception(-31, "Ledger failed to sign transaction");

        /* Double check our next hash if -safemode enabled. */
        if(config::GetBoolArg("-safemode", false))
        {
            /* Re-calculate our next hash if safemode forcing not to use cache. */
            const uint256_t hashNext =
                TAO::Ledger::Transaction::NextHash(pCredentials->Generate(tx.nSequence + 1, strPIN, false), tx.nNextType);

            /* Check that this next hash is what we are expecting. */
            if(tx.hashNext != hashNext)
                throw Exception(-67, "-safemode next hash mismatch, broadcast terminated");
        }

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
            const uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT)
                throw TAO::API::Exception(-65, "Object is not an account");

            /* Check the account is a NXS account */
            if(object.get<uint256_t>("token") != TOKEN::NXS)
                throw TAO::API::Exception(-164, "Fee account is not a NXS account.");

            /* Get the account balance */
            const uint64_t nCurrentBalance = object.get<uint64_t>("balance");

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
    bool BuildContracts(const encoding::json& jParams, const uint512_t& hashTx, std::vector<TAO::Operation::Contract> &vContracts,
                        const build_function_t& xBuild)
    {
        /* Track contracts added to compare values for return */
        const uint32_t nContracts = vContracts.size();

        /* Check our transaction types. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Read the debit transaction that is being credited. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx))
                throw Exception(-40, "Previous transaction not found.");

            /* Loop through all transactions. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                /* Build the credit contract within the contract loop. */
                if(!xBuild(jParams, nContract, rContract, vContracts))
                    continue;
            }
        }
        else if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Read the debit transaction that is being credited. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx))
                throw Exception(-40, "Previous transaction not found.");

            /* Iterate through all TxOut's in the legacy transaction to see which are sends to a sig chain  */
            for(uint32_t nContract = 0; nContract < tx.vout.size(); ++nContract)
            {
                /* Build our contract we will operate on from the legacy output. */
                const TAO::Operation::Contract tContract = TAO::Operation::Contract(tx, nContract);

                /* Build the credit contract within the contract loop. */
                if(!xBuild(jParams, nContract, tContract, vContracts))
                    continue;
            }
        }
        else
            throw Exception(-41, "Invalid transaction-id");

        return (vContracts.size() > nContracts);
    }


    /* Builds a credit contract based on given contract and related parameters. */
    bool BuildCredit(const encoding::json& jParams, const uint32_t nContract, const TAO::Operation::Contract& rDebit,
        std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Extract some parameters from input data. */
        const TAO::Register::Address addrCredit =
            ExtractAddress(jParams, "", "default");

        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Copy our txid out of the contract. */
        const uint512_t hashTx = rDebit.Hash();

        /* Reset the contract to the position of the primitive. */
        const uint8_t nPrimitive = rDebit.Primitive();
        rDebit.SeekToPrimitive(false); //false to skip our primitive

        /* Check for coinbase credits first. */
        if(nPrimitive == TAO::Operation::OP::COINBASE)
        {
            /* Enforce address resolution for coinbase. */
            if(addrCredit == TAO::API::ADDRESS_NONE)
                throw Exception(-35, "Invalid parameter [name], expecting [exists]");

            /* Get the genesisHash of the user who mined the coinbase*/
            uint256_t hashMiner = 0;
            rDebit >> hashMiner;

            /* Check that the coinbase was mined by the caller */
            if(hashMiner != hashGenesis)
                return false;

            /* Get the amount from the coinbase transaction. */
            uint64_t nAmount = 0;
            rDebit >> nAmount;

            /* Now lets check our expected types match. */
            if(!CheckStandard(jParams, addrCredit))
                return false;

            /* if we passed all of these checks then insert the credit contract into the tx */
            TAO::Operation::Contract tContract;
            tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
            tContract << addrCredit << hashGenesis << nAmount;

            /* Push to our contract queue. */
            vContracts.push_back(tContract);

            return true;
        }

        /* Next check for debit credits. */
        else if(nPrimitive == TAO::Operation::OP::DEBIT)
        {
            /* Get the source account the debit transaction. */
            TAO::Register::Address addrSource;
            rDebit >> addrSource;

            /* Get the recipient from the debit transaction. */
            TAO::Register::Address addrRecipient;
            rDebit >> addrRecipient;

            /* Get the amount to respond to. */
            uint64_t nAmount = 0;
            rDebit >> nAmount;

            /* Check for a legacy output debit. */
            if(addrSource == TAO::Register::WILDCARD_ADDRESS)
            {
                /* Enforce address resolution for wildcard claim. */
                if(addrCredit == TAO::API::ADDRESS_NONE)
                    throw Exception(-35, "Invalid parameter [name], expecting [exists]");

                /* Read our crediting account. */
                TAO::Register::Object oCredit;
                if(!LLD::Register->ReadObject(addrCredit, oCredit, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Let's check our credit account is correct token. */
                if(oCredit.get<uint256_t>("token") != 0)
                    throw Exception(-59, "Account to credit is not a NXS account");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oCredit))
                    throw Exception(-49, "Unsupported type for name/address");

                /* If we passed these checks then insert the credit contract into the tx */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrCredit << addrSource << nAmount;

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }

            /* Check for wildcard for conditional contract (OP::VALIDATE). */
            if(addrRecipient == TAO::Register::WILDCARD_ADDRESS)
            {
                /* Enforce address resolution for wildcard claim. */
                if(addrCredit == TAO::API::ADDRESS_NONE)
                    throw Exception(-35, "Invalid parameter [name], expecting [exists]");

                /* Check for conditions. */
                if(!rDebit.Empty(TAO::Operation::Contract::CONDITIONS)) //XXX: unit tests for this scope in finance unit tests
                {
                    /* Check for condition. */
                    if(rDebit.Operations()[0] == TAO::Operation::OP::CONDITION)
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
                TAO::Register::Object oSource;
                if(!LLD::Register->ReadObject(addrSource, oSource, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Read our crediting account. */
                TAO::Register::Object oCredit;
                if(!LLD::Register->ReadObject(addrCredit, oCredit, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Let's check our credit account is correct token. */
                if(oSource.get<uint256_t>("token") != oCredit.get<uint256_t>("token"))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oCredit))
                    throw Exception(-49, "Unsupported type for name/address");

                /* If we passed these checks then insert the credit contract into the tx */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrCredit << addrSource << nAmount;

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }

            /* Get the token / account object that the debit was made to. */
            TAO::Register::Object oRecipient;
            if(!LLD::Register->ReadObject(addrRecipient, oRecipient, TAO::Ledger::FLAGS::MEMPOOL))
                return false;

            /* Check for standard credit to account. */
            const uint8_t nStandardBase = oRecipient.Base();
            if(nStandardBase == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* Check that the debit was made to an account that we own */
                if(oRecipient.hashOwner != hashGenesis)
                    return false;

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oRecipient))
                    throw Exception(-49, "Unsupported type for name/address");

                /* Create our new contract now. */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrRecipient << addrSource << nAmount; //we use addrRecipient from our debit contract instead of addrCredit

                /* Push to our contract queue. */
                vContracts.push_back(tContract);
            }

            /* If addressed to non-standard object, this could be a tokenized debit, so do some checks to find out. */
            else if(nStandardBase == TAO::Register::OBJECTS::NONSTANDARD || TAO::Register::REGISTER::STATE(nStandardBase))
            {
                /* Enforce address resolution for wildcard claim. */
                if(addrCredit == TAO::API::ADDRESS_NONE)
                    throw Exception(-35, "Invalid parameter [name], expecting [exists]");

                /* Attempt to get the proof from the parameters. */
                const TAO::Register::Address addrProof = ExtractAddress(jParams, "proof");

                /* Check that the owner is a token. */
                if(oRecipient.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                    return false;

                /* Read our token contract now since we now know it's correct. */
                TAO::Register::Object oToken;
                if(!LLD::Register->ReadObject(oRecipient.hashOwner, oToken, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* We shouldn't ever evaluate to true here, but if we do let's not make things worse for ourselves. */
                if(oToken.Standard() != TAO::Register::OBJECTS::TOKEN)
                    return false;

                /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                TAO::Register::Object oProof;
                if(!LLD::Register->ReadObject(addrProof, oProof, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Check that the proof is an account for the same token as the asset owner */
                if(oProof.get<uint256_t>("token") != oToken.get<uint256_t>("token"))
                    throw Exception(-61, "Proof account is for a different token than the asset token.");

                /* Retrieve the account to debit from. */
                TAO::Register::Object oSource;
                if(!LLD::Register->ReadObject(addrSource, oSource, TAO::Ledger::FLAGS::MEMPOOL))
                    return false;

                /* Read our crediting account. */
                TAO::Register::Object oCredit;
                if(!LLD::Register->ReadObject(addrCredit, oCredit, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Check that the account being debited from is the same token type as credit. */
                if(oSource.get<uint256_t>("token") != oCredit.get<uint256_t>("token"))
                    throw Exception(-33, "Source token account mismatch with recipient token");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oCredit))
                    throw Exception(-49, "Unsupported type for name/address");

                /* Calculate the partial amount we want to claim based on our share of the proof tokens */
                const uint64_t nPartial = (oProof.get<uint64_t>("balance") * nAmount) / oToken.get<uint64_t>("supply");

                /* Create our new contract now. */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrCredit << addrProof << nPartial;

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }
        }

        return false; //if we get this far, this contract is not creditable
    }


    /* Builds a claim contract based on given contract and related parameters. */
    bool BuildClaim(const encoding::json& jParams, const uint32_t nContract,
        const TAO::Operation::Contract& rTransfer, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Copy our txid out of the contract. */
        const uint512_t hashTx = rTransfer.Hash();

        /* Reset the contract to the position of the primitive. */
        const uint8_t nPrimitive = rTransfer.Primitive();
        rTransfer.SeekToPrimitive(false); //false to skip our primitive

        /* Check that primitive is correct. */
        if(nPrimitive != TAO::Operation::OP::TRANSFER)
            return false;

        /* Grab our address to pass back. */
        uint256_t hashAddress;
        rTransfer >> hashAddress;

        /* Deserialize some values. */
        uint256_t hashRecipient;
        rTransfer >> hashRecipient;

        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check that recipient is current session. */
        if(hashRecipient != hashGenesis)
            return false;

        /* Check out our object now. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashAddress, tObject))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Create our new contract now. */
        TAO::Operation::Contract tContract;
        tContract << uint8_t(TAO::Operation::OP::CLAIM) << hashTx << uint32_t(nContract) << hashAddress;

        /* Push to our contract queue. */
        vContracts.push_back(tContract);

        return true; //if we get this far, this claim was a success
    }


    /* Builds a void contract based on given contract and related parameters. */
    bool BuildVoid(const encoding::json& jParams, const uint32_t nContract,
        const TAO::Operation::Contract& rDependent, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check that we aren't voiding a transaction not owned by us. */
        if(rDependent.Caller() != hashGenesis)
            return false;

        /* Attempt to add our void contract now. */
        TAO::Operation::Contract tVoided;
        if(!AddVoid(rDependent, nContract, tVoided))
            return false;

        /* Push our void contract to outgoing queue. */
        vContracts.push_back(tVoided);

        return true;
    }


    /* Builds a name contract based on given input parameters. */
    void BuildName(const encoding::json& jParams, const uint256_t& hashRegister, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
        if(jParams.find("name") != jParams.end())
        {
            /* Check for empty name and alert caller of error. */
            const std::string& strName = jParams["name"].get<std::string>();
            if(strName.empty())
                throw Exception(-88, "Missing or empty name.");

            /* Check if creating token, which would mean this is a global name. */
            std::string strNamespace = ""; //default to our local namespace
            if(hashRegister.GetType() == TAO::Register::Address::TOKEN)
                strNamespace = TAO::Register::NAMESPACE::GLOBAL;

            /* Add an optional name if supplied. */
            vContracts.push_back
            (
                Names::CreateName(Authentication::Caller(jParams),
                strName, strNamespace, hashRegister)
            );
        }
    }


    /* Build a standard object based on hard-coded standards of object register. */
    TAO::Register::Object BuildStandard(const encoding::json& jParams, uint256_t &hashRegister)
    {
        /* Extract our type from input parameters. */
        const std::string strStandard = ExtractType(jParams);

        /* Handle for standard account type. */
        if(strStandard == "account")
        {
            /* Generate register address for account. */
            hashRegister =
                TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* If this is not a NXS token account, verify that the token identifier is for a valid token */
            const TAO::Register::Address hashToken = ExtractToken(jParams);
            if(hashToken != 0)
            {
                /* Check our address before hitting the database. */
                if(hashToken.GetType() != TAO::Register::Address::TOKEN)
                    throw Exception(-57, "Invalid parameter [token]");

                /* Get the register off the disk. */
                TAO::Register::Object tToken;
                if(!LLD::Register->ReadObject(hashToken, tToken, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-13, "Object not found");

                /* Check the standard */
                if(tToken.Standard() != TAO::Register::OBJECTS::TOKEN)
                    throw Exception(-57, "Invalid parameter [token]");
            }

            /* Create an account object register. */
            return TAO::Register::CreateAccount(hashToken);
        }

        /* Handle for standard token type. */
        if(strStandard == "token")
        {
            /* Generate register address for token. */
            hashRegister =
                TAO::Register::Address(TAO::Register::Address::TOKEN);

            /* Extract the supply parameter */
            const uint64_t nSupply  =
                ExtractInteger<uint64_t>(jParams, "supply");

            /* Check for nDecimals parameter. */
            const uint8_t nDecimals =
                ExtractInteger<uint64_t>(jParams, "decimals", 2, convert::MAX_DIGITS); //2, 8: default of 2 decimals, max of 8

            /* Sanitize the supply/decimals combination for uint64 overflow */
            const uint64_t nCoinFigures = math::pow(10, nDecimals);
            if(nDecimals > 0 && nSupply > (std::numeric_limits<uint64_t>::max() / nCoinFigures))
                throw Exception(-178, "Invalid supply/decimals. The maximum supply/decimals cannot exceed 18446744073709551615");

            /* Create a token object register. */
            return TAO::Register::CreateToken(hashRegister, nSupply * nCoinFigures, nDecimals);
        }

        /* Handle for standard name type. */
        if(strStandard == "name" || strStandard == "global")
        {
            /* Get our genesis-id for this call. */
            const uint256_t hashGenesis =
                Authentication::Caller(jParams);

            /* Check for required parameters. */
            if(!CheckParameter(jParams, "name", "string"))
                throw Exception(-28, "Missing parameter [name] for command");

            /* Check for required parameters. */
            if(!CheckParameter(jParams, "address", "string"))
                throw Exception(-28, "Missing parameter [address] for command");

            /* Grab our name parameter now. */
            const std::string strName =
                jParams["name"].get<std::string>();

            /* Grab our new register address to point towards. */
            const TAO::Register::Address hashExternal =
                TAO::Register::Address(jParams["address"].get<std::string>());

            /* Check for valid address now. */
            if(!hashExternal.IsValid())
                throw Exception(-57, "Invalid Parameter [address]");

            /* Check for global parameters. */
            const bool fGlobal =
                ExtractBoolean(jParams, "global", (strStandard == "global"));

            /* Check for reserved values. */
            if(fGlobal && TAO::Register::NAME::Reserved(strName))
                throw Exception(-22, "Field [", strName, "] is a reserved name");

            /* Check for namespace parameter. */
            if(CheckParameter(jParams, "namespace", "string"))
            {
                /* Grab our namespace parameter now. */
                const std::string strNamespace =
                    jParams["namespace"].get<std::string>();

                /* Grab our namespace's address. */
                const uint256_t hashNamespace =
                    TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

                /* Check for global names. */
                if(fGlobal)
                    throw Exception(-57, "Invalid parameter [global]");

                /* Make sure the namespace exists. */
                TAO::Register::Object rNamespace;
                if(!LLD::Register->ReadObject(hashNamespace, rNamespace))
                    throw Exception(-95, "Namespace does not exist [", strNamespace, "]");

                /* Check that the namespace is owned by us. */
                if(rNamespace.hashOwner != hashGenesis)
                    throw Exception(-96, "Cannot create name in [", strNamespace, "] you don't own");

                /* Calculate our address now. */
                hashRegister =
                    TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

                /* Build our name if in namespace. */
                return TAO::Register::CreateName(strNamespace, strName, hashExternal);
            }

            /* If none found assume in local namespace. */
            else if(!fGlobal)
            {
                /* Calculate our address now. */
                hashRegister =
                    TAO::Register::Address(strName, hashGenesis, TAO::Register::Address::NAME);

                /* Build our name if in local user-space which has an empty namespace. */
                return TAO::Register::CreateName("", strName, hashExternal);
            }

            /* Handle for global names from here. */
            if(!fGlobal)
                throw Exception(-28, "Missing parameter [global] for command");

            /* Grab our namespace's address. */
            const uint256_t hashNamespace =
                TAO::Register::Address(TAO::Register::NAMESPACE::GLOBAL, TAO::Register::Address::NAMESPACE);

            /* Calculate our address now. */
            hashRegister =
                TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

            /* Build our name if not in namespace. */
            return TAO::Register::CreateName(TAO::Register::NAMESPACE::GLOBAL, strName, hashExternal);
        }

        /* Handle for standard namespace type. */
        if(strStandard == "namespace")
        {
            /* Check for required parameters. */
            if(!CheckParameter(jParams, "namespace", "string"))
                throw Exception(-28, "Missing parameter [namespace] for command");

            /* Grab our namespace now. */
            const std::string strNamespace =
                jParams["namespace"].get<std::string>();

            /* Check namespace for case/allowed characters */
            if (!std::all_of(strNamespace.cbegin(), strNamespace.cend(),
                [](char c) { return std::islower(c) || std::isdigit(c) || c == '.'; })) //allow letters, numbers, and '.'
                throw Exception(-162, "Namespace can only contain lowercase letters, numbers, periods (.)");

            /* Check for reserved names. */
            if(TAO::Register::NAMESPACE::Reserved(strNamespace))
                throw Exception(-200, "Namespaces can't contain reserved names");

            /* Grab our namespace's address. */
            hashRegister =
                TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

            return TAO::Register::CreateNamespace(strNamespace);
        }

        throw Exception(-23, "Object standard [", strStandard, "] not available for command");
    }


    /* Build a blank object based on _commands string, generating register address as well. */
    TAO::Register::Object BuildObject(uint256_t &hashRegister, const uint16_t nUserType)
    {
        /* Check for invalid user-types. */
        if(!USER_TYPES::Valid(nUserType))
            throw Exception(-56, "Invalid user-type enum [", nUserType, "]");

        /* Start with our blank object. */
        TAO::Register::Object tObject =
            TAO::Register::Object();

        /* Serialize our commands name if applicable. */
        tObject << std::string("_usertype") << uint8_t(TAO::Register::TYPES::UINT16_T) << uint16_t(nUserType);

        /* Set the address return value now. */
        hashRegister =
            TAO::Register::Address(TAO::Register::Address::OBJECT);

        return tObject;
    }
} // End TAO namespace
