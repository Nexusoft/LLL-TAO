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
        bool Register(const uint256_t& hashAddress, const uint8_t nType, const std::vector<uint8_t>& vchData,
                      const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot create register with reserved address");

            /* Check that the register doesn't exist yet. */
            if(LLD::regDB->HasState(hashAddress, nFlags))
                return debug::error(FUNCTION, "cannot allocate register of same memory address ", hashAddress.ToString());

            /* Set the owner of this register. */
            TAO::Register::State state;
            state.nVersion   = 1;
            state.nType      = nType;
            state.hashOwner  = tx.hashGenesis;
            state.nTimestamp = tx.nTimestamp;

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

                /* Grab the object standard to check values. */
                uint8_t nStandard = object.Standard();

                /* Switch based on standard types. */
                switch(nStandard)
                {

                    /* Check default values for creating a standard account. */
                    case TAO::Register::OBJECTS::ACCOUNT:
                    {
                        /* Check the account balance. */
                        uint64_t nBalance = object.get<uint64_t>("balance");
                        if(nBalance != 0)
                            return debug::error(FUNCTION, "account can't be created with non-zero balance ", nBalance);

                        /* Check that token identifier hasn't been claimed. */
                        if(nFlags & TAO::Register::FLAGS::WRITE
                        || nFlags & TAO::Register::FLAGS::MEMPOOL)
                        {
                            /* Get the token identifier. */
                            uint256_t nIdentifier = object.get<uint256_t>("token_address");

                            /* Check that token identifier hasn't been claimed. */
                            if(nIdentifier != 0  && !LLD::regDB->HasIdentifier(nIdentifier, nFlags))
                                return debug::error(FUNCTION, "account can't be created without token address", nIdentifier.GetHex());

                        }

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
                        if(object.get<uint256_t>("token_address") != 0)
                            return debug::error(FUNCTION, "trust account can't be created with non-default identifier ", object.get<uint256_t>("token_address").GetHex());

                        break;
                    }


                    /* Check default values for creating a standard token. */
                    case TAO::Register::OBJECTS::TOKEN:
                    {
                        /* Get the token identifier. */
                        uint256_t nIdentifier = object.get<uint256_t>("token_address");

                        /* Check for reserved native token. */
                        if(nIdentifier == 0)
                            return debug::error(FUNCTION, "token can't be created with reserved identifier ", nIdentifier.GetHex());

                        /* Check that token identifier hasn't been claimed. */
                        if((nFlags & TAO::Register::FLAGS::WRITE)
                        || (nFlags & TAO::Register::FLAGS::MEMPOOL))
                        {
                            /* Check the claimed register address to identifier. */
                            uint256_t hashClaimed = 0;
                            if(LLD::regDB->ReadIdentifier(nIdentifier, hashClaimed, nFlags))
                                return debug::error(FUNCTION, "token can't be created with reserved identifier ", nIdentifier.GetHex());

                            /* Write the new identifier to database. */
                            if(!LLD::regDB->WriteIdentifier(nIdentifier, hashAddress, nFlags))
                                return debug::error(FUNCTION, "failed to commit token register identifier to disk");
                        }

                        /* Check that the current supply and max supply are the same. */
                        if(object.get<uint64_t>("supply") != object.get<uint64_t>("balance"))
                            return debug::error(FUNCTION, "token current supply and balance can't mismatch");

                        break;
                    }

                    /* Enforce hash on Name objects to ensure that a Name cannot be created for someone elses genesis ID . */
                    case TAO::Register::OBJECTS::NAME:
                    {
                        /* Build vector to hold the genesis + name data for hashing */
                        std::vector<uint8_t> vData;
                        /* Insert the geneses hash of the transaction */
                        vData.insert(vData.end(), (uint8_t*)&tx.hashGenesis, (uint8_t*)&tx.hashGenesis + 32);
                        
                        /* Insert the name of from the Name object */
                        std::string strName = object.get<std::string>("name");
                        vData.insert(vData.end(), strName.begin(), strName.end());
                        
                        /* Hash this in the same was as the caller would have to generate hashAddress */
                        uint256_t hashName = LLC::SK256(vData);
                        
                        /* Compare the two.  If they are different then the caller did not use their own genesis ID to generate the
                           Name register hash */
                        if(hashName != hashAddress)
                            return debug::error(FUNCTION, "incorrect name or genesis");

                        break;
                    }
                }
            }

            /* Check the state change is correct. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << state.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE
            || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the post-state. */
                if(nState != TAO::Register::STATES::POSTSTATE)
                    return debug::error(FUNCTION, "register script not in post-state");

                /* Get the post state checksum. */
                uint64_t nChecksum;
                tx.ssRegister >> nChecksum;

                /* Check for matching post states. */
                if(nChecksum != state.GetHash())
                    return debug::error(FUNCTION, "register script ", std::hex, nChecksum, " has invalid post-state ", std::hex, state.GetHash());

                /* Write the register to the database. */
                if(!LLD::regDB->WriteState(hashAddress, state, nFlags))
                    return debug::error(FUNCTION, "failed to write new state");
            }

            return true;
        }
    }
}
