/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/check.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/build.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /*  Determines whether a string value is a valid base58 encoded register address.
     *  This only checks to see if the value an be decoded into a valid Register::Address with a valid Type.
     *  It does not check to see whether the register address exists in the database
     */
    bool CheckAddress(const std::string& strValue)
    {
        /* Decode the incoming string into a register address */
        TAO::Register::Address address;
        address.SetBase58(strValue);

        /* Check to see whether it is valid */
        return address.IsValid();
    }


    /*  Utilty method that checks that the signature chain is mature and can therefore create new transactions.
     *  Throws an appropriate Exception if it is not mature. */
    void CheckMature(const uint256_t& hashGenesis)
    {
        /* No need to check this in private mode as there is no PoS/Pow */
        if(!config::fHybrid.load())
        {
            /* Get the number of blocks to maturity for this sig chain */
            const uint32_t nBlocksToMaturity = Commands::Get<Users>()->BlocksToMaturity(hashGenesis);
            if(nBlocksToMaturity > 0)
                throw Exception(-202, "Signature chain not mature. ", nBlocksToMaturity, " more confirmation(s) required.");
        }
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
            fSanitized = TAO::Register::Build(tContract, mapStates, TAO::Ledger::FLAGS::MEMPOOL)
                         && TAO::Operation::Execute(tContract, TAO::Ledger::FLAGS::MEMPOOL);

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
    bool CheckStandard(const encoding::json& jParams, const uint256_t& hashCheck, const bool fParse)
    {
        /* Let's grab our object to check against and throw if it's missing. */
        TAO::Register::Object objCheck;
        if(!LLD::Register->ReadObject(hashCheck, objCheck))
            throw Exception(-13, "Object not found");

        /* Execute now that we have the object. */
        return CheckStandard(jParams, objCheck);
    }


    /*  Checks if the designated object matches the explicet type specified in parameters. */
    bool CheckStandard(const encoding::json& jParams, const TAO::Register::Object& objCheck)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(jParams.find("request") == jParams.end())
            return true;

        /* Check for our type we are checking against. */
        if(jParams["request"].find("type") == jParams["request"].end())
            return true;

        /* Check that we have the commands set. */
        const Base* pBase = Commands::Get(jParams["request"]["commands"].get<std::string>());
        if(!pBase)
            return true;

        /* We only fail here, as we want to isolate returns based on the standards, not parameters. */
        return pBase->CheckObject(jParams["request"]["type"].get<std::string>(), objCheck);
    }
} // End TAO namespace
