/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/check.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/build.h>
#include <TAO/Register/types/object.h>

#include <Util/encoding/include/utf-8.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /*  Determines whether a string value is a valid base58 encoded register address. */
    bool CheckAddress(const std::string& strValue)
    {
        /* Decode the incoming string into a register address */
        TAO::Register::Address tAddress;
        tAddress.SetBase58(strValue);

        /* Check to see whether it is valid */
        return tAddress.IsValid();
    }


    /* Determines if given parameter is what is correct expected type. Will throw exception if invalid. */
    bool CheckParameter(const encoding::json& jParams, const std::string& strKey, const std::string& strType)
    {
        /* Check that given parameter exists first. */
        if(jParams.find(strKey) == jParams.end())
            return false;

        /* Now let's check for empty parameters. */
        if(jParams[strKey].empty() || jParams[strKey].is_null())
            return false;

        /* Check for an empty string. */
        if(jParams[strKey].is_string())
        {
            /* Get a reference of our string. */
            const std::string& strParam =
                jParams[strKey].get<std::string>();

            /* Check for an empty string. */
            if(strParam == "")
                return false;

            /* Check we are using proper utf-8 encoding. */
            if(!encoding::utf8::is_valid(strParam.begin(), strParam.end()))
                throw Exception(-130, "Invalid character encoding for [", strKey, "], expecting utf8");
        }

        /* If no type specified, return now. */
        if(strType.empty())
            return true;

        /* Check for expected type. */
        if(strType.find(jParams[strKey].type_name()) == strType.npos)
            throw Exception(-35, "Invalid parameter [", strKey, "=", jParams[strKey].type_name(), "], expecting [", strKey, "=", strType, "]");

        return true;
    }


    /* Determines if given request is what is correct expected type. Will throw exception if invalid. */
    bool CheckRequest(const encoding::json& jParams, const std::string& strKey, const std::string& strType)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(jParams.find("request") == jParams.end())
            return false;

        return CheckParameter(jParams["request"], strKey, strType);
    }


    /*  Utilty method that checks that the signature chain is mature and can therefore create new transactions.
     *  Throws an appropriate Exception if it is not mature. */
    bool CheckMature(const uint256_t& hashGenesis)
    {
        /* Get the user configurable required maturity */
        const uint32_t nMaturityRequired =
            config::GetArg("-maturityrequired", config::fTestNet ? 2 : 33);

        /* If set to 0 then there is no point checking the maturity so return early */
        if(nMaturityRequired > 0)
        {
            /* The hash of the last transaction for this sig chain from disk */
            uint512_t hashLast = 0;
            if(LLD::Logical->ReadLast(hashGenesis, hashLast))
            {
                /* Get the last transaction from disk for this sig chain */
                TAO::API::Transaction tx;
                if(!LLD::Logical->ReadTx(hashLast, tx))
                    return false;

                /* If the previous transaction is a coinbase or coinstake then check the maturity */
                if(tx.IsCoinBase() || tx.IsCoinStake())
                {
                    /* Get number of confirmations of previous TX */
                    uint32_t nConfirms = 0;
                    LLD::Ledger->ReadConfirmations(hashLast, nConfirms);

                    /* Check to see if it is mature */
                    if(nConfirms < nMaturityRequired)
                        return false;
                }
            }
        }

        return true;
    }


    /* Utilty method that checks that the last transaction was within a given amount of seconds. */
    bool CheckTimespan(const uint256_t& hashGenesis, const uint32_t nSeconds)
    {
        /* The hash of the last transaction for this sig chain from disk */
        uint512_t hashLast = 0;
        if(!LLD::Logical->ReadLast(hashGenesis, hashLast))
            return true;

        /* Get the last transaction from disk for this sig chain */
        TAO::API::Transaction tx;
        if(!LLD::Logical->ReadTx(hashLast, tx))
            return true;

        return (tx.nTimestamp + nSeconds < runtime::unifiedtimestamp());
    }


    /* Checks if a contract will execute correctly once built including conditions. */
    bool CheckContract(const TAO::Operation::Contract& rContract)
    {
        /* Dummy value used for build. */
        std::map<uint256_t, TAO::Register::State> mapStates;

        /* Start a ACID transaction. */
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER); //we use miner flag here so we don't conflict with mempool

        /* Return flag */
        bool fSanitized = false;
        try
        {
            /* Temporarily disable error logging so that we don't log errors for contracts that fail to execute. */
            debug::fLogError = false;

            /* Make a copy of our contract, to ensure we don't create bugs if this state isn't properly managed */
            TAO::Operation::Contract tContract = rContract; //XXX: assess how much this copy costs us in cycles, compare with ref

            /* Track if contract succeeded execution and bulding. */
            fSanitized = TAO::Register::Build(tContract, mapStates, TAO::Ledger::FLAGS::MINER)
                         && TAO::Operation::Execute(tContract, TAO::Ledger::FLAGS::MINER);

            /* Reenable error logging. */
            debug::fLogError = true;
        }

        /* Log the error and attempt to continue processing */
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, e.what());
        }

        /* Abort the mempool ACID transaction */
        LLD::TxnAbort(TAO::Ledger::FLAGS::MINER);

        return fSanitized;
    }


    /* Checks if the designated object matches the explicet type specified in parameters. */
    bool CheckStandard(const encoding::json& jParams, const uint256_t& hashCheck)
    {
        /* Let's grab our object to check against and throw if it's missing. */
        TAO::Register::Object rObject;
        if(!LLD::Register->ReadObject(hashCheck, rObject))
            throw Exception(-13, "Object not found");

        /* Execute now that we have the object. */
        return CheckStandard(jParams, rObject);
    }


    /*  Checks if the designated object matches the explicet type specified in parameters. */
    bool CheckStandard(const encoding::json& jParams, const TAO::Register::Object& rObject)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(!CheckRequest(jParams, "type", "string, array"))
            return true;

        /* Check that we have the commands set. */
        const Base* pBase = Commands::Instance(jParams["request"]["commands"].get<std::string>());
        if(!pBase)
            return true;

        /* Check for array to iterate. */
        const encoding::json jTypes = jParams["request"]["type"];
        if(jTypes.is_array())
        {
            /* Loop through standards. */
            for(const auto& jCheck : jTypes)
            {
                /* Check if at least one standard is correct. */
                if(pBase->CheckObject(jCheck.get<std::string>(), rObject))
                    return true;
            }

            return false;
        }

        /* Catch-all here will treat input as a string. */
        return pBase->CheckObject(jTypes.get<std::string>(), rObject);
    }
} // End TAO namespace
