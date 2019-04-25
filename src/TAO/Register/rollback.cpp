/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/stream.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <new> //std::bad_alloc

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Rollback(TAO::Ledger::Transaction tx)
        {
            /* Make sure no exceptions are thrown. */
            try
            {
                /* Loop through the operations. */
                while(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {

                        /* Check pre-state to database. */
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Verify the first register code. */
                            uint8_t nState;
                            tx.ssRegister  >> nState;

                            /* Check the state is prestate. */
                            if(nState != STATES::PRESTATE)
                                return debug::error(FUNCTION, "register state not in pre-state");

                            /* Verify the register's prestate. */
                            State prestate;
                            tx.ssRegister  >> prestate;

                            /* Write the register from database. */
                            if(!LLD::regDB->WriteState(hashAddress, prestate))
                                return debug::error(FUNCTION, "failed to rollback to pre-state");

                            /* Skip over the post-state data. */
                            uint64_t nSize = ReadCompactSize(tx.ssOperation);

                            /* Seek the tx.ssOperation to next operation. */
                            tx.ssOperation.seek(nSize);

                            /* Seek registers past the post state */
                            tx.ssRegister.seek(9);

                            break;
                        }


                        /*
                         * Erase the register
                         */
                        case TAO::Operation::OP::REGISTER:
                        {
                            /* Get the address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Skip over type. */
                            tx.ssOperation.seek(1);

                            /* Skip over the post-state data. */
                            uint64_t nSize = ReadCompactSize(tx.ssOperation);

                            /* Seek the tx.ssOperation to next operation. */
                            tx.ssOperation.seek(nSize);

                            /* Seek register past the post state */
                            tx.ssRegister.seek(9);

                            //TODO: erase identifier

                            /* Erase the register from database. */
                            if(!LLD::regDB->EraseState(hashAddress))
                                return debug::error(FUNCTION, "failed to erase post-state");

                            break;
                        }


                        /* Transfer ownership of a register to another signature chain. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Read the register from database. */
                            State dbstate;
                            if(!LLD::regDB->ReadState(hashAddress, dbstate))
                                return debug::error(FUNCTION, "register pre-state doesn't exist");

                            /* Rollback the event. */
                            if(!LLD::legDB->EraseEvent(dbstate.hashOwner))
                                return debug::error(FUNCTION, "failed to rollback event");

                            /* Set the previous owner to this sigchain. */
                            dbstate.hashOwner = tx.hashGenesis;

                            /* Write the register to database. */
                            if(!LLD::regDB->WriteState(hashAddress, dbstate))
                                return debug::error(FUNCTION, "failed to rollback to pre-state");

                            /* Seek to next operation. */
                            tx.ssOperation.seek(32);

                            /* Seek register past the post state */
                            tx.ssRegister.seek(9);

                            break;
                        }

                        /* Coinbase operation. Creates an account if none exists. */
                        case TAO::Operation::OP::COINBASE:
                        {
                            tx.ssOperation.seek(16);

                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case TAO::Operation::OP::TRUST:
                        {
                            /* The account that is being staked. */
                            uint256_t hashAccount;
                            tx.ssOperation >> hashAccount;

                            /* The previous trust block. */
                            uint1024_t hashLastTrust;
                            tx.ssOperation >> hashLastTrust;

                            /* Previous trust sequence number. */
                            uint32_t nSequence;
                            tx.ssOperation >> nSequence;

                            /* The previous trust calculated. */
                            uint64_t nLastTrust;
                            tx.ssOperation >> nLastTrust;

                            /* The total to be staked. */
                            uint64_t  nStake;
                            tx.ssOperation >> nStake;

                            break;
                        }


                        /* Debit tokens from an account you own. */
                        case TAO::Operation::OP::DEBIT:
                        {
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Verify the first register code. */
                            uint8_t nState;
                            tx.ssRegister  >> nState;

                            /* Check the state is prestate. */
                            if(nState != STATES::PRESTATE)
                                return debug::error(FUNCTION, "register state not in pre-state");

                            /* Verify the register's prestate. */
                            State prestate;
                            tx.ssRegister  >> prestate;

                            /* Read the register from database. */
                            if(!LLD::regDB->WriteState(hashAddress, prestate))
                                return debug::error(FUNCTION, "failed to rollback to pre-state");

                            /* Get the address to. */
                            uint256_t hashTo;
                            tx.ssOperation >> hashTo;

                            /* Read the register from the database. */
                            TAO::Register::State stateTo;
                            if(!LLD::regDB->ReadState(hashTo, stateTo))
                                return debug::error(FUNCTION, "register address doesn't exist ", hashTo.ToString());

                            /* Write the event to the ledger database. */
                            if(!LLD::legDB->EraseEvent(stateTo.hashOwner))
                                return debug::error(FUNCTION, "failed to rollback event");

                            /* Seek to the next operation. */
                            tx.ssOperation.seek(8);

                            /* Seek register past the post state */
                            tx.ssRegister.seek(9);

                            break;
                        }


                        /* Credit tokens to an account you own. */
                        case TAO::Operation::OP::CREDIT:
                        {
                            /* The transaction that this credit is claiming. */
                            tx.ssOperation.seek(96);

                            /* The account that is being credited. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Verify the first register code. */
                            uint8_t nState;
                            tx.ssRegister  >> nState;

                            /* Check the state is prestate. */
                            if(nState != STATES::PRESTATE)
                                return debug::error(FUNCTION, "register state not in pre-state");

                            /* Verify the register's prestate. */
                            State prestate;
                            tx.ssRegister  >> prestate;

                            /* Read the register from database. */
                            if(!LLD::regDB->ReadState(hashAddress, prestate))
                                return debug::error(FUNCTION, "failed to rollback to pre-state");

                            /* Seek to the next operation. */
                            tx.ssOperation.seek(8);

                            break;
                        }
                    }
                }
            }
            catch(const std::bad_alloc &e)
            {
                return debug::error(FUNCTION, "Memory allocation failed ", e.what());
            }
            catch(const std::runtime_error& e)
            {
                return debug::error(FUNCTION, "exception encountered ", e.what());
            }

            /* If nothing failed, return true for evaluation. */
            return true;
        }
    }
}
