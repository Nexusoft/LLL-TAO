/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Resolves a register address from a name by looking up the Name object. */
    TAO::Register::Address Names::ResolveAddress(const encoding::json& jParams, const std::string& strName, const bool fThrow)
    {
        /* Check for the NXS ticker. */
        if(strName == "NXS")
            return TOKEN::NXS;

        /* Declare the return register address hash */
        TAO::Register::Address hashRegister;

        /* Get the Name object by name */
        const TAO::Register::Object tObject =
            Names::GetName(jParams, strName, hashRegister, fThrow);

        /* Get the address that this name register is pointing to */
        if(tObject.Check("address", TAO::Register::TYPES::UINT256_T, true))
            return tObject.get<uint256_t>("address");

        return TAO::API::ADDRESS_NONE; //otherwise return none (1)
    }


    /* Resolves a register address from a namespace by looking up the namespace using incoming parameters */
    TAO::Register::Address Names::ResolveNamespace(const encoding::json& jParams)
    {
        /* Check that we have namespace parameter. */
        if(!CheckParameter(jParams, "namespace", "string"))
            throw Exception(-56, "Missing Parameter [namespace]");

        /* Grab our namespace from incoming parameters. */
        const std::string& strLookup =
            jParams["namespace"].get<std::string>();

        /* Check that it is valid */
        if(TAO::Register::Address(strLookup).IsValid())
            throw Exception(-56, "Parameter [namespace] cannot be register address. Must use [address] for raw");

        return ResolveNamespace(strLookup);
    }


    /* Resolves a register address from a namespace by looking up the namespace by hashing namespace name */
    TAO::Register::Address Names::ResolveNamespace(const std::string& strNamespace)
    {
        return TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
    }


    /* Does a reverse name look-up by PTR records from names API logical indexes. */
    bool Names::ReverseLookup(const uint256_t& hashAddress, std::string &strName)
    {
        /* Check for regular NXS accounts. */
        if(hashAddress == TOKEN::NXS)
        {
            /* Set to our NXS ticker. */
            strName = "NXS";
            return true;
        }

        /* Handle for NXS hardcoded token name. */
        uint256_t hashName;
        if(!LLD::Logical->ReadPTR(hashAddress, hashName))
            return false;

        /* Get the name object now. */
        TAO::Register::Object tName;
        if(!LLD::Register->ReadObject(hashName, tName))
            return false;

        /* Get our namespace. */
        const std::string strNamespace =
            tName.get<std::string>("namespace");

        /* Check for namespace. */
        if(!strNamespace.empty())
        {
            /* Check for global. */
            if(strNamespace == TAO::Register::NAMESPACE::GLOBAL)
            {
                strName = tName.get<std::string>("name");
                return true;
            }

            /* Grab our name from object. */
            strName = (strNamespace + "::" + tName.get<std::string>("name"));
            return true;
        }

        /* Grab our name from object. */
        strName = tName.get<std::string>("name"); //local name

        return true;
    }

    /* Does a reverse name look-up by PTR records from names API logical indexes. */
    bool Names::ReverseLookup(const uint256_t& hashAddress, encoding::json &jRet)
    {
        /* Check for regular NXS accounts. */
        if(hashAddress == TOKEN::NXS)
        {
            /* Set to our NXS ticker. */
            jRet["name"]   = "NXS";
            jRet["global"] = true;
            jRet["mine"]   = false;

            return true;
        }

        /* Handle for NXS hardcoded token name. */
        uint256_t hashName;
        if(!LLD::Logical->ReadPTR(hashAddress, hashName))
            return false;

        /* Get the name object now. */
        TAO::Register::Object tName;
        if(!LLD::Register->ReadObject(hashName, tName))
            return false;

        /* Output our name data now. */
        NameToJSON(tName, jRet);

        return true;
    }


    /* Output all the name related json data to a json object. */
    void Names::NameToJSON(const TAO::Register::Object& rName, encoding::json &jRet)
    {
        /* Delete these keys so we can define order and since we are adjusting values anyhow. */
        jRet.erase("namespace");
        jRet.erase("name");

        /* Get our namespace. */
        const std::string strNamespace =
            rName.get<std::string>("namespace");

        /* Check if this is our register. */
        uint256_t hashSession;
        const bool fMine =
            Authentication::Active(rName.hashOwner, hashSession);

        /* Check for namespace. */
        if(!strNamespace.empty())
        {
            /* Check for global. */
            if(strNamespace == TAO::Register::NAMESPACE::GLOBAL)
            {
                /* Set to our NXS ticker. */
                jRet["name"]   = rName.get<std::string>("name");
                jRet["global"] = true;
                jRet["mine"]   = fMine;

                return;
            }

            /* Set to our NXS ticker. */
            jRet["name"]      = rName.get<std::string>("name");
            jRet["namespace"] = strNamespace;
            jRet["global"]    = false;
            jRet["mine"]      = fMine;

            return;
        }

        /* Set our value for local name. */
        jRet["name"]      = rName.get<std::string>("name");

        /* Add our username namespace if we are logged in. */
        if(fMine)
            jRet["user"] = Authentication::Credentials(hashSession)->UserName();

        /* Set our remaining values. */
        jRet["local"]     = true;
        jRet["mine"]      = fMine;
    }
} /* End TAO namespace */
