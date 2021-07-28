/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/wallet/wallet.h>

#include <LLD/include/global.h>

#include <TAO/API/types/commands/system.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Validates a register / legacy address */
    encoding::json System::Validate(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract the address, which will either be a legacy address or a sig chain account address */
        const std::string strAddress =
            ExtractString(jParams, "address");

        /* The decoded register address */
        const TAO::Register::Address hashAddress =
            TAO::Register::Address(strAddress);

        /* Populate address into response */
        encoding::json jRet;
        jRet["address"] = strAddress;
        jRet["valid"]   = false;

        /* handle recipient being a register address */
        if(!hashAddress.IsValid())
            return jRet;

        /* Check to see if this is a legacy address */
        if(hashAddress.IsLegacy())
        {
            /* Set our response values. */
            jRet["valid"] = true;
            jRet["type"]  = "LEGACY";

            #ifndef NO_WALLET

            /* Only populate mine field when wallet enabled. */
            jRet["mine"]  = false;

            /* Legacy address */
            const Legacy::NexusAddress hashAddress = Legacy::NexusAddress(strAddress);

            /* Check that we have the key in this wallet. */
            if(Legacy::Wallet::Instance().HaveKey(hashAddress)
            || Legacy::Wallet::Instance().HaveScript(hashAddress.GetHash256()))
                jRet["mine"] = true;

            #endif

            return jRet;
        }

        /* Get the tObject. We only consider an address valid if the tObject exists in the register database*/
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashAddress, tObject, TAO::Ledger::FLAGS::MEMPOOL))
            return jRet;

        /* Set our response values. */
        jRet["valid"] = true;
        jRet["type"]  = GetRegisterName(tObject.nType);

        /* Check for caller's genesis. */
        const uint256_t hashCaller =
            Commands::Get<Users>()->GetCallersGenesis(jParams);

        /* Make sure we were able to get the caller. */
        if(hashCaller != 0)
        {
            /* Set false for fall through. */
            jRet["mine"]  = false;

            /* Check if address is owned by current user */
            if(tObject.hashOwner == hashCaller)
                jRet["mine"] = true;
        }

        /* Add our standard for objects. */
        if(tObject.nType == TAO::Register::REGISTER::OBJECT)
            jRet["standard"] = GetStandardName(tObject.Standard());

        return jRet;
    }
}
