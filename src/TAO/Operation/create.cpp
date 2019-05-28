/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Creates a new register if it doesn't exist. */
        bool Create(TAO::Register::State &state, const std::vector<uint8_t>& vchData)
        {
            /* Check the register is a valid type. */
            if(!TAO::Register::Range(state.nType))
                return debug::error(FUNCTION, "register using invalid type range");

            /* Set the data in the state register. */
            state.SetState(vchData);

            /* Check register types specific rules. */
            if(nType == TAO::Register::REGISTER::OBJECT)
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
                            return debug::error(FUNCTION, "trust account can't be created with non-zero balance ", object.get<uint64_t>("balance"));

                        /* Check the account balance. */
                        if(object.get<uint64_t>("stake") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-zero stake ", object.get<uint64_t>("stake"));

                        /* Check the account balance. */
                        if(object.get<uint64_t>("trust") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-zero trust ", object.get<uint64_t>("trust"));

                        /* Check that token identifier hasn't been claimed. */
                        if(object.get<uint256_t>("identifier") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-default identifier ", object.get<uint256_t>("identifier").GetHex());

                        break;
                    }


                    /* Check default values for creating a standard token. */
                    case TAO::Register::OBJECTS::TOKEN:
                    {
                        /* Get the token identifier. */
                        uint256_t nIdentifier = object.get<uint256_t>("identifier");

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
            state.SetChecksum();

            /* Check the state change is correct. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.ToString(), " is in invalid state");

            return true;
        }
    }
}
