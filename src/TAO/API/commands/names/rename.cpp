/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Cancel an active order in market. */
    encoding::json Names::Rename(const encoding::json& jParams, const bool fHelp)
    {
        /* Cache the genesis-id of our caller. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Extract our address for name lookup. */
        if(!CheckParameter(jParams, "name", "string"))
            throw Exception(-11, "Missing Parameter [name]");

        /* Get our new name from paramters. */
        const std::string& strOldName =
            jParams["name"].get<std::string>();

        /* Extract our address for name lookup. */
        if(!CheckParameter(jParams, "new", "string"))
            throw Exception(-11, "Missing Parameter [new]");

        /* Get our new name from paramters. */
        const std::string& strNewName =
            jParams["new"].get<std::string>();

        /* We want to track the name's current address. */
        TAO::Register::Address hashOldRegister;

        /* Get our name object now. */
        const TAO::Register::Object oOldName =
            Names::GetName(jParams, strOldName, hashOldRegister, true);

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, oOldName))
            throw Exception(-49, "Unsupported type for name/address");

        /* Check that we own the name being changed. */
        if(oOldName.hashOwner != hashGenesis)
            throw Exception(-41, "Name provided is not owned by calling profile");

        /* Get a copy of our namespace. */
        const std::string& strNamespace =
            oOldName.get<std::string>("namespace");

        /* Throw error if we are operating on a global name. */
        if(strNamespace == TAO::Register::NAMESPACE::GLOBAL)
            throw Exception(-49, "Cannot rename a global name");

        /* Find our namespace hash with local as default. */
        uint256_t hashNamespace = hashGenesis;
        if(!strNamespace.empty())
            hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

        /* Get our new name's register address. */
        const uint256_t hashNewRegister =
            TAO::Register::Address(strNewName, hashNamespace, TAO::Register::Address::NAME);

        /* Track if we need to make a new name or if we are ping-ponging between existing names. */
        bool fNewRegister = true;

        /* Let's check if new register name already exists. */
        TAO::Register::Object oCheck;
        if(LLD::Register->ReadObject(hashNewRegister, oCheck))
        {
            /* Let's check that we own this register. */
            if(oCheck.hashOwner != oOldName.hashOwner)
                throw Exception(-41, "New name provided exists and is not owned by calling profile");

            /* Let's check that our namespaces are the same. */
            if(oCheck.get<std::string>("namespace") != strNamespace)
                throw Exception(-42, "New name provided exists and is not in the same namespace");

            fNewRegister = false;
        }

        /* Create an operations stream for the update payload. */
        TAO::Operation::Stream ssPayload;
        ssPayload << std::string("address") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << uint256_t(0);

        /* Build our contracts to deploy the name change. */
        std::vector<TAO::Operation::Contract> vContracts(2);
        vContracts[0] << uint8_t(TAO::Operation::OP::WRITE) << hashOldRegister << ssPayload.Bytes(); //this modifies current name

        /* Now build our new name record. */
        const uint256_t hashResolved = oOldName.get<uint256_t>("address");
        if(fNewRegister)
        {
            /* Build our new name object register. */
            const TAO::Register::Object oNewName =
                TAO::Register::CreateName(strNamespace, strNewName, hashResolved);

            /* Now build our contract go deploy to the chain. */
            vContracts[1] << uint8_t(TAO::Operation::OP::CREATE)      << hashNewRegister;
            vContracts[1] << uint8_t(TAO::Register::REGISTER::OBJECT) << oNewName.GetState();
        }
        else
        {
            /* Create an operations stream for the update payload. */
            TAO::Operation::Stream ssUpdate;
            ssUpdate << std::string("address") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << hashResolved;

            /* Update our existing name record now. */
            vContracts[1] << uint8_t(TAO::Operation::OP::WRITE) << hashNewRegister << ssUpdate.Bytes(); //this modifies current name
        }


        return BuildResponse(jParams, vContracts);
    }
}
