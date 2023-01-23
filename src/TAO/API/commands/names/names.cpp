/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/address.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Creates a new Name Object register for the given name and register
       address adds the register operation to the contract */
    TAO::Operation::Contract Names::CreateName(const uint256_t& hashGenesis, const std::string& strName,
                                               const std::string& strNamespace,
                                               const TAO::Register::Address& hashRegister)
    {
        /* Check name length */
        if(strName.length() == 0)
            throw Exception(-88, "Missing or empty name.");

        /* Name can't start with : */
        if(strName[0]== ':')
            throw Exception(-161, "Names cannot start with a colon");

        /* The hash representing the namespace that the Name will be created in. */
        uint256_t hashNamespace = hashGenesis; //default to local name

        /* Check to see whether the name is being created in a namesace */
        if(!strNamespace.empty())
        {
            /* If the name is not being created in the global namespace then check that the caller owns the namespace */
            if(strNamespace != TAO::Register::NAMESPACE::GLOBAL)
            {
                /* Namespace object to retrieve*/
                TAO::Register::Object oNamespace;

                /* Retrieve the Namespace object by name */
                if(!TAO::Register::GetNamespaceRegister(strNamespace, oNamespace))
                    throw Exception(-95, "Namespace does not exist: " + strNamespace);

                /* Check the owner is the hashGenesis */
                if(oNamespace.hashOwner != hashGenesis)
                    throw Exception(-96, "Cannot create a name in namespace " + strNamespace + " as you are not the owner.");
            }

            /* If it is a global name then check it doesn't contain any colons */
            else if(strNamespace.find(":") != strNamespace.npos)
                throw Exception(-171, "Global names cannot cannot contain a colon");

            /* Namespace hash is a SK256 hash of the namespace name */
            hashNamespace =
                TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
        }

        /* Obtain the name register address for the genesis/name combination */
        const TAO::Register::Address hashName =
            TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

        /* Check to see whether the name already exists  */
        TAO::Register::Object oLookup;
        if(LLD::Register->ReadState(hashName, oLookup, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-97, "An object with this name already exists.");

        /* Create the Name register object pointing to hashRegister */
        const TAO::Register::Object oName =
            TAO::Register::CreateName(strNamespace, strName, hashRegister);

        /* Add the Name object register operation to the transaction */
        TAO::Operation::Contract tContract;
        tContract << uint8_t(TAO::Operation::OP::CREATE) << hashName << uint8_t(TAO::Register::REGISTER::OBJECT) << oName.GetState();

        return tContract;
    }


    /* Retrieves a Name object by name. */
    TAO::Register::Object Names::GetName(const encoding::json& jParams, const std::string& strObjectName,
                                         TAO::Register::Address& hashName, const bool fThrow)
    {
        /* Declare the namespace hash to use for this object. */
        uint256_t hashGenesis;

        /* Track our name object. */
        TAO::Register::Object oNameRet;

        /* Get the hashNamespace for the global namespace */
        const uint256_t hashNamespace =
            TAO::Register::Address(TAO::Register::NAMESPACE::GLOBAL, TAO::Register::Address::NAMESPACE);

        /* Check if we have a local name override. */
        const auto nLocalNamePos = strObjectName.find("local:");
        if(nLocalNamePos != std::string::npos)
        {
            /* Extract our delimiter from string. */
            const std::string strName      = strObjectName.substr(nLocalNamePos + 6);

            /* First check the callers local namespace to see if it exists */
            if(!Authentication::Caller(jParams, hashGenesis))
                throw Exception(-11, "local: override requires session");

            /* Check if we can find the local name record. */
            if(TAO::Register::GetNameRegister(hashGenesis, strName, oNameRet))
            {
                /* Set the return name address. */
                hashName =
                    TAO::Register::Address(strName, hashGenesis, TAO::Register::Address::NAME);

                return oNameRet;
            }
        }

        /* Otherwise check our global name scope. */
        else if(TAO::Register::GetNameRegister(hashNamespace, strObjectName, oNameRet))
        {
            /* Set the return name address. */
            hashName =
                TAO::Register::Address(strObjectName, hashNamespace, TAO::Register::Address::NAME);

            return oNameRet;
        }

        /* Check if we have a namespace delimiter. */
        const auto nNamespacePos = strObjectName.find("::");
        if(nNamespacePos != std::string::npos)
        {
            /* Extract our namespace and name. */
            const std::string strNamespace = strObjectName.substr(0, nNamespacePos);
            const std::string strName      = strObjectName.substr(nNamespacePos + 2);

            /* Get our namespace address now. */
            const uint256_t hashNamespace =
                TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

            /* Check for the name record in namespace. */
            if(TAO::Register::GetNameRegister(hashNamespace, strName, oNameRet))
            {
                /* Set the return name address. */
                hashName =
                    TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

                return oNameRet;
            }
        }

        /* Check if we have a userspace delimiter. */
        const auto nUserspacePos = strObjectName.find(":");
        if(nUserspacePos != std::string::npos && nNamespacePos == std::string::npos)
        {
            /* Extract our namespace and name. */
            const std::string strNamespace = strObjectName.substr(0, nUserspacePos);
            const std::string strName      = strObjectName.substr(nUserspacePos + 1);

            /* Get our namespace address now. */
            const uint256_t hashNamespace =
                TAO::Ledger::SignatureChain::Genesis(SecureString(strNamespace.c_str()));

            /* Check for the name record in namespace. */
            TAO::Register::Object oNameRet;
            if(TAO::Register::GetNameRegister(hashNamespace, strName, oNameRet))
            {
                /* Set the return name address. */
                hashName =
                    TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

                return oNameRet;
            }
        }

        /* First check the callers local namespace to see if it exists */
        if(!Authentication::Caller(jParams, hashGenesis) && fThrow)
            throw Exception(-11, "Session not found");

        /* Check in our local namespace. */
        if(TAO::Register::GetNameRegister(hashGenesis, strObjectName, oNameRet))
        {
            /* Set the return name address. */
            hashName =
                TAO::Register::Address(strObjectName, hashGenesis, TAO::Register::Address::NAME);

            return oNameRet;
        }

        /* If it wasn't resolved then error */
        if(fThrow)
            throw Exception(-101, "Unknown name: ", strObjectName);

        return oNameRet;
    }
} /* End TAO namespace */
