/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/execute.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/append.h>
#include <TAO/Operation/include/claim.h>
#include <TAO/Operation/include/coinbase.h>
#include <TAO/Operation/include/constants.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/credit.h>
#include <TAO/Operation/include/debit.h>
#include <TAO/Operation/include/fee.h>
#include <TAO/Operation/include/genesis.h>
#include <TAO/Operation/include/legacy.h>
#include <TAO/Operation/include/migrate.h>
#include <TAO/Operation/include/transfer.h>
#include <TAO/Operation/include/trust.h>
#include <TAO/Operation/include/validate.h>
#include <TAO/Operation/include/write.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/enum.h>

namespace TAO::API
{
    /* Checks a given string against value to find wildcard pattern matches. */
    TAO::Register::Object ExecuteContract(const TAO::Operation::Contract& rContract)
    {
        /* Reset the contract streams. */
        rContract.SeekToPrimitive();

        /* Get the contract OP. */
        uint8_t nOP = 0;
        rContract >> nOP;

        /* Special case for CREATE that doesn't have a pre-state. */
        if(nOP == TAO::Operation::OP::CREATE)
        {
            /* Seek over address. */
            rContract.Seek(32);

            /* Get the Register Type. */
            uint8_t nType = 0;
            rContract >> nType;

            /* Get the register data. */
            std::vector<uint8_t> vchData;
            rContract >> vchData;

            /* Create the register object. */
            TAO::Register::Object tObject;
            tObject.nVersion   = 1;
            tObject.nType      = nType;
            tObject.hashOwner  = rContract.Caller();

            /* Calculate the new operation. */
            if(!TAO::Operation::Create::Execute(tObject, vchData, rContract.Timestamp()))
                throw Exception(-30, FUNCTION, "Operations failed to execute");

            /* Parse object after created. */
            if(!tObject.Parse())
                throw Exception(-30, FUNCTION, "Object failed to parse");

            return tObject;
        }

        /* Loop through all operations to calculate post-state. */
        TAO::Register::Object tObject = rContract.PreState();
        switch(nOP)
        {
            /* Generate pre-state to database. */
            case TAO::Operation::OP::WRITE:
            case TAO::Operation::OP::APPEND:
            {
                /* Seek over address. */
                rContract.Seek(32);

                /* Get the state data. */
                std::vector<uint8_t> vchData;
                rContract >> vchData;

                /* Case for OP::WRITE. */
                if(nOP == TAO::Operation::OP::WRITE && !TAO::Operation::Write::Execute(tObject, vchData, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                /* Case for OP::APPEND. */
                if(nOP == TAO::Operation::OP::APPEND && !TAO::Operation::Append::Execute(tObject, vchData, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Transfer ownership of a register to another signature chain. */
            case TAO::Operation::OP::TRANSFER:
            {
                /* Seek over address. */
                rContract.Seek(32);

                /* Read the register transfer recipient. */
                uint256_t hashTransfer = 0;
                rContract >> hashTransfer;

                /* Read the force transfer flag */
                uint8_t nType = 0;
                rContract >> nType;

                /* Register custody in SYSTEM ownership until claimed, unless the ForceTransfer flag has been set */
                uint256_t hashNewOwner = 0; //default to SYSTEM
                if(nType == TAO::Operation::TRANSFER::FORCE)
                    hashNewOwner = hashTransfer;

                /* Handle version switch for contract to transfer to 0x00 leading genesis-id. */
                else if(rContract.Version() > 1) //this allows us to iterate history while getting properties of transfer to system
                {
                    /* Set the new owner to transfer recipient. */
                    hashNewOwner = rContract.Caller();
                    hashNewOwner.SetType(TAO::Ledger::GENESIS::SYSTEM); //this byte (0x00) means there is no valid owner during transfer
                }

                /* Calculate the new operation. */
                if(!TAO::Operation::Transfer::Execute(tObject, hashNewOwner, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Transfer ownership of a register to another signature chain. */
            case TAO::Operation::OP::CLAIM:
            {
                /* Calculate the new operation. */
                if(!TAO::Operation::Claim::Execute(tObject, rContract.Caller(), rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Coinstake operation. Requires an account. */
            case TAO::Operation::OP::TRUST:
            {
                /* Seek to scores. */
                rContract.Seek(64);

                /* Get the trust score. */
                uint64_t nScore = 0;
                rContract >> nScore;

                /* Get the stake change. */
                int64_t nStakeChange = 0;
                rContract >> nStakeChange;

                /* Get the stake reward. */
                uint64_t nReward = 0;
                rContract >> nReward;

                /* Calculate the new operation. */
                if(!TAO::Operation::Trust::Execute(tObject, nReward, nScore, nStakeChange, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Coinstake operation. Requires an account. */
            case TAO::Operation::OP::GENESIS:
            {
                /* Get the stake reward. */
                uint64_t nReward = 0;
                rContract >> nReward;

                /* Calculate the new operation. */
                if(!TAO::Operation::Genesis::Execute(tObject, nReward, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Debit tokens from an account you own. */
            case TAO::Operation::OP::DEBIT:
            {
                /* Seek to scores. */
                rContract.Seek(64);

                /* Get the transfer amount. */
                uint64_t  nAmount = 0;
                rContract >> nAmount;

                /* Calculate the new operation. */
                if(!TAO::Operation::Debit::Execute(tObject, nAmount, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Credit tokens to an account you own. */
            case TAO::Operation::OP::CREDIT:
            {
                /* Seek to scores. */
                rContract.Seek(132);

                /* Get the transfer amount. */
                uint64_t  nAmount = 0;
                rContract >> nAmount;

                /* Calculate the new operation. */
                if(!TAO::Operation::Credit::Execute(tObject, nAmount, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Migrate a trust key to a trust account register. */
            case TAO::Operation::OP::MIGRATE:
            {
                /* Seek to scores. */
                rContract.Seek(168);

                /* Get the amount to migrate. */
                uint64_t nAmount;
                rContract >> nAmount;

                /* Get the trust score to migrate. */
                uint32_t nScore;
                rContract >> nScore;

                /* Calculate the new operation. */
                if(!TAO::Operation::Migrate::Execute(tObject, nAmount, nScore, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }


            /* Debit tokens from an account you own. */
            case TAO::Operation::OP::FEE:
            case TAO::Operation::OP::LEGACY:
            {
                /* Seek to scores. */
                rContract.Seek(32);

                /* Get the fee amount. */
                uint64_t nValue = 0;
                rContract >> nValue;

                /* Calculate the new operation. */
                if(nOP == TAO::Operation::OP::FEE && !TAO::Operation::Fee::Execute(tObject, nValue, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                /* Calculate the new operation. */
                if(nOP == TAO::Operation::OP::LEGACY && !TAO::Operation::Legacy::Execute(tObject, nValue, rContract.Timestamp()))
                    throw Exception(-30, FUNCTION, "Operations failed to execute");

                return tObject;
            }

            default:
                throw Exception(-29, FUNCTION, "Unsupported format [", uint32_t(nOP), "] specified");
        }
    }
}
