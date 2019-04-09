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

#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/objects/token.h>
#include <TAO/Register/objects/account.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Creates a new register if it doesn't exist. */
        bool Register(const uint256_t &hashAddress, const uint8_t nType, const std::vector<uint8_t> &vchData, const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for wildcard reserved values. */
            if(hashAddress == OP::WILDCARD)
                return debug::error(FUNCTION, "cannot create register with WILDCARD address");

            /* Check that the register doesn't exist yet. */
            if(LLD::regDB->HasState(hashAddress))
                return debug::error(FUNCTION, "cannot allocate register of same memory address ", hashAddress.ToString());

            /* Set the owner of this register. */
            TAO::Register::State state;
            state.nVersion  = 1;
            state.nType     = nType;
            state.hashOwner = hashCaller;

            /* Check register types specific rules. */
            switch(nType)
            {

                case TAO::Register::STATE::ACCOUNT:
                {
                    /* Create an account object. */
                    TAO::Register::Account acct;

                    /* Setup a stream to deserialize the data. */
                    DataStream ssData(vchData, SER_REGISTER, state.nVersion);
                    ssData >> acct;

                    /* Check that the size is correct. */
                    if(vchData.size() != acct.GetSerializeSize(SER_REGISTER, state.nVersion))
                        return debug::error(FUNCTION, "unexpected account register size ", vchData.size());

                    /* Check the account version. */
                    if(acct.nVersion != 1)
                        return debug::error(FUNCTION, "unexpected account version ", acct.nVersion);

                    /* Check the account balance. */
                    if(acct.nBalance != 0)
                        return debug::error(FUNCTION, "account can't be created with non-zero balance ", acct.nBalance);

                    /* Check that token identifier hasn't been claimed. */
                    if(acct.nIdentifier != 0 && !LLD::regDB->HasIdentifier(acct.nIdentifier, nFlags))
                        return debug::error(FUNCTION, "account can't be created with no identifier ", acct.nIdentifier);

                    break;
                }

                case TAO::Register::STATE::TOKEN:
                {
                    /* Create an account object. */
                    TAO::Register::Token token;

                    /* Setup a stream to deserialize the data. */
                    DataStream ssData(vchData, SER_REGISTER, state.nVersion);
                    ssData >> token;

                    /* Check that the size is correct. */
                    if(vchData.size() != token.GetSerializeSize(SER_REGISTER, state.nVersion))
                        return debug::error(FUNCTION, "unexpected token register size ", vchData.size());

                    /* Check the account version. */
                    if(token.nVersion != 1)
                        return debug::error(FUNCTION, "unexpected token version ", token.nVersion);

                    /* Check that token identifier hasn't been claimed. */
                    if((nFlags & TAO::Register::FLAGS::WRITE) || (nFlags & TAO::Register::FLAGS::MEMPOOL))
                    {
                        /* Check the claimed register address to identifier. */
                        uint256_t hashClaimed = 0;
                        if(token.nIdentifier == 0 || (LLD::regDB->ReadIdentifier(token.nIdentifier, hashClaimed, nFlags) && hashClaimed != hashAddress))
                            return debug::error(FUNCTION, "token can't be created with reserved identifier ", token.nIdentifier);

                        /* Write the new identifier to database. */
                        if(!LLD::regDB->WriteIdentifier(token.nIdentifier, hashAddress, nFlags))
                            return debug::error(FUNCTION, "failed to commit token register identifier to disk");
                    }

                    /* Check that the current supply and max supply are the same. */
                    if(token.nMaxSupply != token.nBalance)
                        return debug::error(FUNCTION, "token current supply and max supply can't mismatch");

                    break;
                }
            }

            /* Set the state from binary data. */
            state.nTimestamp = tx.nTimestamp;
            state.SetState(vchData);

            /* Check the state change is correct. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << state.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
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
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->WriteState(hashAddress, state))
                    return debug::error(FUNCTION, "failed to write new state");
            }

            return true;
        }
    }
}
