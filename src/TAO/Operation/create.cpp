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

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/constants.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Create::Commit(const TAO::Register::State& state, const TAO::Register::Address& address, uint64_t& nCost, const uint8_t nFlags)
        {
            /* Set cost to zero. */
            nCost = 0;

            /* Check register types specific rules. */
            switch(state.nType)
            {
                /* Check for object registers. */
                case TAO::Register::REGISTER::OBJECT:
                {
                    /* Create the object register. */
                    TAO::Register::Object object = TAO::Register::Object(state);

                    /* Parse the object register. */
                    if(!object.Parse())
                        return debug::error(FUNCTION, "object register failed to parse");

                    /* Get the cost. */
                    nCost = object.Cost();

                    /* Switch based on standard types. */
                    uint8_t nStandard = object.Standard();
                    switch(nStandard)
                    {

                        /* Check default values for creating a standard account. */
                        case TAO::Register::OBJECTS::ACCOUNT:
                        {
                            /* Check the address type to expected type. */
                            if(!address.IsAccount())
                                return debug::error(FUNCTION, "address type mismatch with object type");

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
                            /* Check the address type to expected type. */
                            if(!address.IsToken())
                                return debug::error(FUNCTION, "address type mismatch with object type");

                            /* Get the identifier. */
                            uint256_t hashIdentifier = object.get<uint256_t>("token");

                            /* Check identifier to address. */
                            if(hashIdentifier != address)
                                return debug::error(FUNCTION, "token identifier must be token address");

                            /* Check for reserved native token. */
                            if(hashIdentifier == 0 || LLD::Register->HasState(hashIdentifier, nFlags))
                                return debug::error(FUNCTION, "token can't use reserved identifier ", hashIdentifier.SubString());

                            /* Check that the current supply and max supply are the same. */
                            if(object.get<uint64_t>("supply") != object.get<uint64_t>("balance"))
                                return debug::error(FUNCTION, "token current supply and balance can't mismatch");

                            break;
                        }


                        /* Check default values for creating a standard account. */
                        case TAO::Register::OBJECTS::TRUST:
                        {
                            /* Check the address type to expected type. */
                            if(!address.IsTrust())
                                return debug::error(FUNCTION, "address type mismatch with object type");

                            /* Enforce the hash of trust accounts to be deterministically generated from genesis hash */
                            TAO::Register::Address trust = TAO::Register::Address(std::string("trust"), state.hashOwner, TAO::Register::Address::TRUST);

                            /* Fail if trust address was not generated deterministically based on callers genesis. */
                            if(trust != address)
                                return debug::error(FUNCTION, "trust address mismatch");

                            break;
                        }


                        /* Enforce hash on Name objects to ensure that a Name cannot be created for someone elses genesis ID . */
                        case TAO::Register::OBJECTS::NAME:
                        {
                            /* Check the address type to expected type. */
                            if(!address.IsName())
                                return debug::error(FUNCTION, "address type mismatch with object type");

                            /* Declare the namespace hash */
                            uint256_t hashNamespace = 0;

                            /* The name */
                            std::string strName = object.get<std::string>("name");

                            /* If the Name contains a namespace then use a hash of this to verify the register address hash */
                            std::string strNamespace = object.get<std::string>("namespace");
                            if(!strNamespace.empty())
                            {
                                /* Namespace hash is a SK256 hash of the namespace name */
                                hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
                                
                                /* If the namespace is NOT the global namespace then retrieve the namespace object 
                                   and check that the hashGenesis is the owner */
                                if(strNamespace != TAO::Register::NAMESPACE::GLOBAL)
                                {
                                    TAO::Register::Object objectNamespace;
                                    if(!TAO::Register::GetNamespaceRegister(strNamespace, objectNamespace))
                                        return debug::error(FUNCTION, "Namespace does not exist: ", strNamespace);

                                    /* Check the owner is the hashGenesis */
                                    if(objectNamespace.hashOwner != state.hashOwner)
                                        return debug::error(FUNCTION, "Namespace not owned by caller: ", strNamespace );
                                }
                            }
                            else
                                /* Otherwise we use the owner genesis Hash */
                                hashNamespace = state.hashOwner;

                            /* Create an address in the same was as the caller would have to generate hashAddress */
                            TAO::Register::Address name = TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

                            /* Fail if caller didn't user their own genesis to create name. */
                            if(name != address)
                                return debug::error(FUNCTION, "incorrect name or genesis");

                            break;
                        }


                        /* Enforce hash on Name objects to ensure that a Name cannot be created for someone elses genesis ID . */
                        case TAO::Register::OBJECTS::NAMESPACE:
                        {
                            /* Check the address type to expected type. */
                            if(!address.IsNamespace())
                                return debug::error(FUNCTION, "address type mismatch with object type");

                            /* Insert the name of from the Name object */
                            std::string strNamespace = object.get<std::string>("namespace");

                            /* Hash this in the same was as the caller would have to generate hashAddress */
                            TAO::Register::Address name = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

                            /* Fail if caller didn't user their own genesis to create name. */
                            if(name != address)
                                return debug::error(FUNCTION, "namespace address mismatch");

                            break;
                        }

                        /* Check for non-standard types. */
                        default:
                        {
                            /* Check the address type to expected type. */
                            if(!address.IsObject())
                                return debug::error(FUNCTION, "address type mismatch with object type");
                        }
                    }

                    break;
                }


                /* Check for readonly types. */
                case TAO::Register::REGISTER::READONLY:
                {
                    /* Check the address type to expected type. */
                    if(!address.IsReadonly())
                        return debug::error(FUNCTION, "address type mismatch with object type");

                    break;
                }


                /* Check for append types. */
                case TAO::Register::REGISTER::APPEND:
                {
                    /* Check the address type to expected type. */
                    if(!address.IsAppend())
                        return debug::error(FUNCTION, "address type mismatch with object type");

                    break;
                }


                /* Check for raw types. */
                case TAO::Register::REGISTER::RAW:
                {
                    /* Check the address type to expected type. */
                    if(!address.IsRaw())
                        return debug::error(FUNCTION, "address type mismatch with object type");

                    break;
                }
            }

            /* Check that the register doesn't exist yet. */
            if(LLD::Register->HasState(address, nFlags))
                return debug::error(FUNCTION, "cannot allocate register of same memory address ", address.SubString());

            /* Attempt to write new state to disk. */
            if(!LLD::Register->WriteState(address, state, nFlags))
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
                            return debug::error(FUNCTION, "account balance must be zero ", nBalance);

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

                    /* Check name for invalid characters. */
                    case TAO::Register::OBJECTS::NAME:
                    {
                        /* Get the namespace. */
                        std::string strNamespace = object.get<std::string>("namespace");

                        /* Get the name. */
                        std::string strName = object.get<std::string>("name");
                        
                        /* Global names must not contain a : or :: */
                        if(strNamespace == TAO::Register::NAMESPACE::GLOBAL)
                        {
                            if(strName.find(":") != strName.npos)
                                return debug::error(FUNCTION, "Global names cannot contain colons: ", strName);
                        }
                        else
                        {
                            /* Local and namespaced names must not start with a : or :: */
                            if(strName[0] == ':')
                                return debug::error(FUNCTION, "Names cannot start with a colon: ", strName);
                        }

                        break;
                    }

                    /* Check namespace name is not reserved. */
                    case TAO::Register::OBJECTS::NAMESPACE:
                    {
                        /* Get the token identifier. */
                        std::string strNamespace = object.get<std::string>("namespace");

                        /* Check for reserved names. */
                        if(strNamespace == TAO::Register::NAMESPACE::GLOBAL)
                            return debug::error(FUNCTION, "namespace can't be created with reserved name ", strNamespace);

                        /* Check that name doesn't contain colons */
                        if(strNamespace.find(":") != strNamespace.npos)
                            return debug::error(FUNCTION, "Namespace names cannot contain colons: ", strNamespace);

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
            TAO::Register::Address address;
            contract >> address;

            /* Check for invalid address. */
            if(!address.IsValid())
                return debug::error(FUNCTION, "cannot create register with invalid address");

            /* Check for reserved values. */
            if(TAO::Register::Reserved(address))
                return debug::error(FUNCTION, "cannot create register with reserved address");

            /* Check for wildcard. */
            if(address == TAO::Register::WILDCARD_ADDRESS)
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
