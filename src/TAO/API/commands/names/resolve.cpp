/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/session-manager.h>

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
        strName = "~" + tName.get<std::string>("name"); //local name

        return true;
    }




    /* Scans the Name records associated with the hashGenesis sig chain to find an entry with a matching hashRegister address */
    std::string Names::ResolveName(const uint256_t& hashGenesis, const TAO::Register::Address& hashRegister)
    {
        /* Declare the return val */
        std::string strName = "";

        /* Register address of nameObject.  Not used by this method */
        TAO::Register::Address hashNameObject;

        /* The resolved name record  */
        TAO::Register::Object name;

        /* Look up the Name object for the register address in the specified sig chain, if one has been provided */
        if(hashGenesis != 0)
        {
            /* If we are in client mode then if the hashGenesis is not for the logged in user we need to make sure we
               have downloaded their sig chain so that we have access to it */
            if(config::fClient.load() && hashGenesis != GetSessionManager().Get(0).GetAccount()->Genesis() )
            {
                /* Download the users signature chain transactions, but we do not need events */
                //TAO::API::DownloadSigChain(hashGenesis, false);
            }

            /* Now lookup the name in this sig chain */
            name = Names::GetName(hashGenesis, hashRegister, hashNameObject);
        }

        /* Check to see if we resolved the name using the specified sig chain */
        if(!name.IsNull())
        {
            /* Get the name from the Name register */
            strName = name.get<std::string>("name");
        }
        else if(!config::fClient.load())
        {
            /* If we couldn't resolve the register name from the callers local names, we next check to see if it is a global name */

            /* Batch read all names. */
            std::vector<TAO::Register::Object> vNames;
            if(LLD::Register->BatchRead("name", vNames, -1))
            {
                /* Check through all names. */
                for(auto& object : vNames)
                {
                    /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                    if(!object.Parse())
                        continue;

                    /* Check that it is a global */
                    if(object.get<std::string>("namespace") == TAO::Register::NAMESPACE::GLOBAL
                        && object.get<uint256_t>("address") == hashRegister)
                    {
                        /* Get the name from the Name register */
                        strName = object.get<std::string>("name");
                        break;
                    }
                }
            }
        }

        return strName;
    }
} /* End TAO namespace */
