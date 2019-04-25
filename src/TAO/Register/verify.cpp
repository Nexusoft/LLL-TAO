/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Register/types/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Register/include/verify.h>

#include <new> //std::bad_alloc

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Verify(TAO::Ledger::Transaction tx)
        {
            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Start the register stream at the beginning. */
            tx.ssRegister.seek(0, STREAM::BEGIN);

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Loop through the operations */
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

                            /* Read the register from database. */
                            State dbstate;
                            if(!LLD::regDB->ReadState(hashAddress, dbstate))
                                return debug::error(FUNCTION, "register pre-state doesn't exist");

                            /* Check the ownership. */
                            if(dbstate.hashOwner != tx.hashGenesis)
                                return debug::error(FUNCTION, "cannot generate pre-state if not owner");

                            /* Check the prestate to the dbstate. */
                            if(prestate != dbstate)
                                return debug::error(FUNCTION, "prestate and dbstate mismatch");

                            /* Skip over the post-state data. */
                            uint64_t nSize = ReadCompactSize(tx.ssOperation);

                            /* Seek the tx.ssOperation to next operation. */
                            tx.ssOperation.seek(nSize);

                            /* Seek registers past the post state */
                            tx.ssRegister.seek(9);

                            break;
                        }


                        /*
                         * This does not contain any prestates.
                         */
                        case TAO::Operation::OP::REGISTER:
                        {
                            /* Skip over address and type. */
                            tx.ssOperation.seek(33);

                            /* Skip over the post-state data. */
                            uint64_t nSize = ReadCompactSize(tx.ssOperation);

                            /* Seek the tx.ssOperation to next operation. */
                            tx.ssOperation.seek(nSize);

                            /* Seek register past the post state */
                            tx.ssRegister.seek(9);

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

                            /* Check the ownership. */
                            if(dbstate.hashOwner != tx.hashGenesis)
                                return debug::error(FUNCTION, "cannot generate pre-state if not owner");

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
                            tx.ssOperation.seek(180);

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
                            State dbstate;
                            if(!LLD::regDB->ReadState(hashAddress, dbstate))
                                return debug::error(FUNCTION, "register pre-state doesn't exist");

                            /* Check the ownership. */
                            if(dbstate.hashOwner != tx.hashGenesis)
                                return debug::error(FUNCTION, "cannot generate pre-state if not owner");

                            /* Check the prestate to the dbstate. */
                            if(prestate != dbstate)
                                return debug::error(FUNCTION, "prestate and dbstate mismatch");

                            /* Seek to the next operation. */
                            tx.ssOperation.seek(40);

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
                            State dbstate;
                            if(!LLD::regDB->ReadState(hashAddress, dbstate))
                                return debug::error(FUNCTION, "register pre-state doesn't exist");

                            /* Check the ownership. */
                            if(dbstate.hashOwner != tx.hashGenesis)
                                return debug::error(FUNCTION, "cannot generate pre-state if not owner");

                            /* Check the prestate to the dbstate. */
                            if(prestate != dbstate)
                                return debug::error(FUNCTION, "prestate and dbstate mismatch");

                            /* Seek to the next operation. */
                            tx.ssOperation.seek(8);

                            break;
                        }

                        default:
                            return debug::error(FUNCTION, "invalid code for register verification");
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
