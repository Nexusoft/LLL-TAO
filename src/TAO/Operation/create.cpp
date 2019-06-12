/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/system.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Create::Commit(const TAO::Register::State& state, const uint256_t& hashAddress, const uint8_t nFlags)
        {
            /* Check register types specific rules. */
            if(state.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Create the object register. */
                TAO::Register::Object object = TAO::Register::Object(state);

                /* Parse the object register. */
                if(!object.Parse())
                    return debug::error(FUNCTION, "object register failed to parse");

                /* Switch based on standard types. */
                uint8_t nStandard = object.Standard();
                switch(nStandard)
                {

                    /* Check default values for creating a standard account. */
                    case TAO::Register::OBJECTS::ACCOUNT:
                    {
                        /* Get the identifier. */
                        uint256_t hashIdentifier = object.get<uint256_t>("token");

                        /* Check that the register doesn't exist yet. */
                        if(hashIdentifier != 0 && !LLD::Register->HasState(hashIdentifier, nFlags))
                            return debug::error(FUNCTION, "cannot create account without identifier");

                        break;
                    }


                    /* Check default values for creating a standard token. */
                    case TAO::Register::OBJECTS::TOKEN:
                    {
                        /* Get the identifier. */
                        uint256_t hashIdentifier = object.get<uint256_t>("token");

                        /* Check identifier to address. */
                        if(hashIdentifier != hashAddress)
                            return debug::error(FUNCTION, "token identifier must be token address");

                        /* Check for reserved native token. */
                        if(hashIdentifier == 0 || LLD::Register->HasState(hashIdentifier, nFlags))
                            return debug::error(FUNCTION, "token can't use reserved identifier ", hashIdentifier.SubString());

                        /* Check that the current supply and max supply are the same. */
                        if(object.get<uint64_t>("supply") != object.get<uint64_t>("balance"))
                            return debug::error(FUNCTION, "token current supply and balance can't mismatch");

                        break;
                    }


                    /* Enforce hash on Name objects to ensure that a Name cannot be created for someone elses genesis ID . */
                    case TAO::Register::OBJECTS::NAME:
                    {
                        /* Declare the namespace hash */
                        uint256_t hashNamespace = 0;

                        /* If the Name contains a namespace then use a hash of this to verify the register address hash */
                        std::string strNamespace = object.get<std::string>("namespace");
                        if(!strNamespace.empty())
                        {
                            /* Generate the namespace hash from the namespace name */
                            hashNamespace = TAO::Register::NamespaceHash(strNamespace);

                            /* Retrieve the namespace object and check that the hashGenesis is the owner */
                            TAO::Register::Object namespaceObject;
                            if(!TAO::Register::GetNamespaceRegister(strNamespace, namespaceObject))
                                return debug::error(FUNCTION, "Namespace does not exist: ", strNamespace);

                            /* Check the owner is the hashGenesis */
                            if(namespaceObject.hashOwner != state.hashOwner)
                                return debug::error(FUNCTION, "Namespace not owned by caller: ", strNamespace );

                        }
                        else
                            /* Otherwise we use the owner genesis Hash */
                            hashNamespace = state.hashOwner;
                        

                        /* Build vector to hold the genesis + name data for hashing */
                        std::vector<uint8_t> vData((uint8_t*)&hashNamespace, (uint8_t*)&hashNamespace + 32);

                        /* Insert the name of from the Name object */
                        std::string strName = object.get<std::string>("name");
                        vData.insert(vData.end(), strName.begin(), strName.end());

                        /* Hash this in the same was as the caller would have to generate hashAddress */
                        uint256_t hashName = LLC::SK256(vData);

                        /* Fail if caller didn't user their own genesis to create name. */
                        if(hashName != hashAddress)
                            return debug::error(FUNCTION, "incorrect name or genesis");

                        break;
                    }


                    /* Enforce hash on Namespace objects to ensure that the register address is a hash of the namespace name. */
                    case TAO::Register::OBJECTS::NAMESPACE:
                    {
                        /* Insert the name of from the Name object */
                        std::string strNamespace = object.get<std::string>("namespace");

                        /* Hash this in the same was as the caller would have to generate hashAddress */
                        uint256_t hashNamespace = TAO::Register::NamespaceHash(strNamespace);

                        /* Fail if caller didn't user their own genesis to create name. */
                        if(hashNamespace != hashAddress)
                            return debug::error(FUNCTION, "incorrect name or genesis");

                        break;
                    }
                }
            }

            /* Check that the register doesn't exist yet. */
            if(LLD::Register->HasState(hashAddress, nFlags))
                return debug::error(FUNCTION, "cannot allocate register of same memory address ", hashAddress.SubString());

            /* Attempt to write new state to disk. */
            if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Creates a new register if it doesn't exist. */
        bool Create::Execute(TAO::Register::State &state, const std::vector<uint8_t>& vchData, const uint64_t nTimestamp)
        {
            /* Check the register is a valid type. */
            if(!TAO::Register::Range(state.nType))
                return debug::error(FUNCTION, "register using invalid type range");

            /* Set the data in the state register. */
            state.SetState(vchData);

            /* Check register types specific rules. */
            if(state.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Create the object register. */
                TAO::Register::Object object = TAO::Register::Object(state);

                /* Parse the object register. */
                if(!object.Parse())
                    return debug::error(FUNCTION, "object register failed to parse");

                /* Switch based on standard types. */
                uint8_t nStandard = object.Standard();
                switch(nStandard)
                {

                    /* Check default values for creating a standard account. */
                    case TAO::Register::OBJECTS::ACCOUNT:
                    {
                        /* Check the account balance. */
                        uint64_t nBalance = object.get<uint64_t>("balance");
                        if(nBalance != 0)
                            return debug::error(FUNCTION, "account balance msut be zero ", nBalance);

                        break;
                    }


                    /* Check default values for creating a standard account. */
                    case TAO::Register::OBJECTS::TRUST:
                    {
                        /* Check the account balance. */
                        if(object.get<uint64_t>("balance") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-zero balance ",
                            object.get<uint64_t>("balance"));

                        /* Check the account balance. */
                        if(object.get<uint64_t>("stake") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-zero stake ",
                            object.get<uint64_t>("stake"));

                        /* Check the account balance. */
                        if(object.get<uint64_t>("trust") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-zero trust ",
                            object.get<uint64_t>("trust"));

                        /* Check that token identifier hasn't been claimed. */
                        if(object.get<uint256_t>("token") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-default identifier ",
                            object.get<uint256_t>("token").SubString());

                        break;
                    }


                    /* Check default values for creating a standard token. */
                    case TAO::Register::OBJECTS::TOKEN:
                    {
                        /* Get the token identifier. */
                        uint256_t nIdentifier = object.get<uint256_t>("token");

                        /* Check for reserved native token. */
                        if(nIdentifier == 0)
                            return debug::error(FUNCTION, "token can't be created with reserved identifier ", nIdentifier.GetHex());

                        /* Check that the current supply and max supply are the same. */
                        if(object.get<uint64_t>("supply") != object.get<uint64_t>("balance"))
                            return debug::error(FUNCTION, "token current supply and balance can't mismatch");

                        break;
                    }
                }
            }

            /* Update the state register checksum. */
            state.nCreated  = nTimestamp;
            state.nModified = nTimestamp;
            state.SetChecksum();

            /* Check the state change is correct. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "post-state is in invalid state");

            return true;
        }


        /* Verify Append and caller register. */
        bool Create::Verify(const Contract& contract)
        {
            /* Rewind back on byte. */
            contract.Rewind(1, Contract::OPERATIONS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::CREATE)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            uint256_t hashAddress = 0;
            contract >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot create register with reserved address");

            /* Check for wildcard. */
            if(hashAddress == ~uint256_t(0))
                return debug::error(FUNCTION, "cannot create register with wildcard address");

            /* Get the object data size. */
            uint32_t nSize = contract.ReadCompactSize(Contract::OPERATIONS);

            /* Check register size limits. */
            if(nSize > 1024)
                return debug::error(FUNCTION, "register is beyond size limits");

            /* Seek read position to first position. */
            contract.Rewind(32 + GetSizeOfCompactSize(nSize), Contract::OPERATIONS);

            return true;
        }
    }
}
