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
        bool Rollback(const TAO::Ledger::Transaction& tx)
        {
            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Start the register stream at the beginning. */
            tx.ssRegister.seek(0, STREAM::BEGIN);

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
                            uint8_t nType;
                            tx.ssOperation >> nType;

                            /* Skip over the post-state data. */
                            uint64_t nSize = ReadCompactSize(tx.ssOperation);

                            /* Seek the tx.ssOperation to next operation. */
                            tx.ssOperation.seek(nSize);

                            /* Seek register past the post state */
                            tx.ssRegister.seek(9);

                            /* Check for object register. */
                            if(nType == REGISTER::OBJECT)
                            {
                                /* Read the object from register database. */
                                Object object;
                                if(!LLD::regDB->ReadState(hashAddress, object))
                                    return debug::error(FUNCTION, "failed to read register object");

                                /* Parse the object. */
                                if(!object.Parse())
                                    return debug::error(FUNCTION, "failed to parse object");

                                /* Check for token to remove reserved identifier. */
                                if(object.Standard() == OBJECTS::TOKEN)
                                {
                                    /* Erase the identifier. */ //TODO: possibly do not check for false
                                    if(!LLD::regDB->EraseIdentifier(object.get<uint32_t>("identifier")))
                                        return debug::error(FUNCTION, "could not erase identifier");
                                }
                            }

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

                            /* Extract the transfer address from the tx. */
                            uint256_t hashTransfer;
                            tx.ssOperation >> hashTransfer;

                            /* Read the register from database. */
                            State state;
                            if(!LLD::regDB->ReadState(hashAddress, state))
                                return debug::error(FUNCTION, "register pre-state doesn't exist");

                            /* Rollback the event. */
                            if(!LLD::legDB->EraseEvent(hashTransfer))
                                return debug::error(FUNCTION, "failed to rollback event");

                            /* Set the previous owner to this sigchain. */
                            state.hashOwner = tx.hashGenesis;

                            /* Write the register to database. */
                            if(!LLD::regDB->WriteState(hashAddress, state))
                                return debug::error(FUNCTION, "failed to rollback to pre-state");

                            /* Seek register past the post state */
                            tx.ssRegister.seek(9);

                            break;
                        }


                        /* Transfer ownership of a register to another signature chain. */
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* The transaction that this transfer is claiming. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* Read the previous transaction. */
                            TAO::Ledger::Transaction txClaim;
                            if(!LLD::legDB->ReadTx(hashTx, txClaim))
                                return debug::error(FUNCTION, "could not read previous transaction");

                            /* Seek past the op. */
                            txClaim.ssOperation.seek(1);

                            /* Read the address. */
                            uint256_t hashAddress;
                            txClaim.ssOperation >> hashAddress;

                            /* Read the transfer address. */
                            uint256_t hashTransfer;
                            txClaim.ssOperation >> hashTransfer;

                            /* Check for claim back to self. */
                            if(txClaim.hashGenesis == tx.hashGenesis)
                            {
                                /* Write the event. */
                                //if(!LLD::legDB->WriteEvent(hashTransfer, hashTx))
                                //    return debug::error(FUNCTION, "failed to write event");
                            }

                            /* Erase the proof. */
                            if(!LLD::legDB->EraseProof(hashAddress, hashTx))
                                return debug::error(FUNCTION, "failed to erase transfer proof");

                            /* Read the register from database. */
                            State state;
                            if(!LLD::regDB->ReadState(hashAddress, state))
                                return debug::error(FUNCTION, "register pre-state doesn't exist");

                            /* Set the previous owner to this sigchain. */
                            state.hashOwner = 0;

                            /* Write the register to database. */
                            if(!LLD::regDB->WriteState(hashAddress, state))
                                return debug::error(FUNCTION, "failed to rollback to pre-state");

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
                            /* Skip ahead in operation stream. */
                            tx.ssOperation.seek(72);

                            /* Scope the register pre-state verification. */
                            {
                                /* Verify the first register code. */
                                uint8_t nState;
                                tx.ssRegister  >> nState;

                                /* Check the state is prestate. */
                                if(nState != STATES::PRESTATE)
                                    return debug::error(FUNCTION, "register state not in pre-state");

                                /* Verify the register's prestate. */
                                State state;
                                tx.ssRegister  >> state;

                                /* Write the register from database. */
                                if(!LLD::regDB->WriteTrust(tx.hashGenesis, state))
                                    return debug::error(FUNCTION, "failed to rollback to pre-state");
                            }

                            /* Scope the system register pre-state verification. */
                            {
                                /* Get the system pre-state. */
                                uint8_t nState;
                                tx.ssSystem  >> nState;

                                /* Check the state is prestate. */
                                if(nState != STATES::PRESTATE)
                                    return debug::error(FUNCTION, "register state not in pre-state");

                                /* Verify the register's prestate. */
                                State state;
                                tx.ssSystem  >> state;

                                /* Write the register to database. */
                                if(!LLD::regDB->WriteState(uint256_t(SYSTEM::TRUST), state))
                                    return debug::error(FUNCTION, "failed to rollback to pre-state");
                            }


                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case TAO::Operation::OP::GENESIS:
                        {
                            /* Scope the register pre-state verification. */
                            {
                                /* Verify the first register code. */
                                uint8_t nState;
                                tx.ssRegister  >> nState;

                                /* Check the state is prestate. */
                                if(nState != STATES::PRESTATE)
                                    return debug::error(FUNCTION, "register state not in pre-state");

                                /* The account that is being staked. */
                                uint256_t hashAccount;
                                tx.ssOperation >> hashAccount;

                                /* Verify the register's prestate. */
                                State state;
                                tx.ssRegister  >> state;

                                /* Write the register from database. */
                                if(!LLD::regDB->WriteState(hashAccount, state))
                                    return debug::error(FUNCTION, "failed to rollback to pre-state");
                            }

                            /* Scope the system register pre-state verification. */
                            {
                                /* Get the system pre-state. */
                                uint8_t nState;
                                tx.ssSystem  >> nState;

                                /* Check the state is prestate. */
                                if(nState != STATES::PRESTATE)
                                    return debug::error(FUNCTION, "register state not in pre-state");

                                /* Verify the register's prestate. */
                                State state;
                                tx.ssSystem  >> state;

                                /* Write the register to database. */
                                if(!LLD::regDB->WriteState(uint256_t(SYSTEM::TRUST), state))
                                    return debug::error(FUNCTION, "failed to rollback to pre-state");
                            }

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
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* Get the hash proof. */
                            uint256_t hashProof;
                            tx.ssOperation >> hashProof;

                            /* The account that is being credited. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Read the previous transaction. */
                            TAO::Ledger::Transaction txClaim;
                            if(!LLD::legDB->ReadTx(hashTx, txClaim))
                                return debug::error(FUNCTION, "could not read previous transaction");

                            /* Check for claim back to self. */
                            if(txClaim.hashGenesis == tx.hashGenesis)
                            {
                                /* Seek to the address to. */
                                txClaim.ssOperation.seek(33);

                                /* Get the hash to. */
                                uint256_t hashTo;
                                txClaim.ssOperation >> hashTo;

                                /* Read the register from the database. */
                                TAO::Register::State stateTo;
                                if(!LLD::regDB->ReadState(hashTo, stateTo))
                                    return debug::error(FUNCTION, "register address doesn't exist ", hashTo.ToString());

                                /* Write the event. */
                                //if(!LLD::legDB->WriteEvent(stateTo.hashOwner, hashTx))
                                //    return debug::error(FUNCTION, "failed to write event");
                            }

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

                            /* Erase the proof event from database. */
                            if(!LLD::legDB->EraseProof(hashProof, hashTx))
                                return debug::error(FUNCTION, "failed to erase the proof");

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
