/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_VERIFY_H
#define NEXUS_TAO_REGISTER_INCLUDE_VERIFY_H

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/stream.h>

namespace TAO::Register
{

    /* Verify the pre-states of a register to current network state. */
    bool Verify(TAO::Ledger::Transaction tx, uint8_t nFlags)
    {
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
                            return debug::error(FUNCTION "register state not in pre-state", __PRETTY_FUNCTION__);

                        /* Verify the register's prestate. */
                        State prestate;
                        tx.ssRegister  >> prestate;

                        /* Read the register from database. */
                        State dbstate;
                        if(!LLD::regDB->ReadState(hashAddress, dbstate))
                            return debug::error(FUNCTION "register pre-state doesn't exist", __PRETTY_FUNCTION__);

                        /* Check the ownership. */
                        if(dbstate.hashOwner != tx.hashGenesis)
                            return debug::error(FUNCTION "cannot generate pre-state if not owner", __PRETTY_FUNCTION__);

                        /* Check the prestate to the dbstate. */
                        if(prestate != dbstate)
                            return debug::error(FUNCTION "prestate and dbstate mismatch", __PRETTY_FUNCTION__);

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
                            return debug::error(FUNCTION "register pre-state doesn't exist", __PRETTY_FUNCTION__);

                        /* Check the ownership. */
                        if(dbstate.hashOwner != tx.hashGenesis)
                            return debug::error(FUNCTION "cannot generate pre-state if not owner", __PRETTY_FUNCTION__);

                        /* Seek to next operation. */
                        tx.ssOperation.seek(32);

                        /* Seek register past the post state */
                        tx.ssRegister.seek(9);

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
                            return debug::error(FUNCTION "register state not in pre-state", __PRETTY_FUNCTION__);

                        /* Verify the register's prestate. */
                        State prestate;
                        tx.ssRegister  >> prestate;

                        /* Read the register from database. */
                        State dbstate;
                        if(!LLD::regDB->ReadState(hashAddress, dbstate))
                            return debug::error(FUNCTION "register pre-state doesn't exist", __PRETTY_FUNCTION__);

                        /* Check the ownership. */
                        if(dbstate.hashOwner != tx.hashGenesis)
                            return debug::error(FUNCTION "cannot generate pre-state if not owner", __PRETTY_FUNCTION__);

                        /* Check the prestate to the dbstate. */
                        if(prestate != dbstate)
                            return debug::error(FUNCTION "prestate and dbstate mismatch", __PRETTY_FUNCTION__);

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
                            return debug::error(FUNCTION "register state not in pre-state", __PRETTY_FUNCTION__);

                        /* Verify the register's prestate. */
                        State prestate;
                        tx.ssRegister  >> prestate;

                        /* Read the register from database. */
                        State dbstate;
                        if(!LLD::regDB->ReadState(hashAddress, dbstate))
                            return debug::error(FUNCTION "register pre-state doesn't exist", __PRETTY_FUNCTION__);

                        /* Check the ownership. */
                        if(dbstate.hashOwner != tx.hashGenesis)
                            return debug::error(FUNCTION "cannot generate pre-state if not owner", __PRETTY_FUNCTION__);

                        /* Check the prestate to the dbstate. */
                        if(prestate != dbstate)
                            return debug::error(FUNCTION "prestate and dbstate mismatch", __PRETTY_FUNCTION__);

                        /* Seek to the next operation. */
                        tx.ssOperation.seek(8);

                        break;
                    }
                }
            }
        }
        catch(std::runtime_error& e)
        {
            return debug::error(FUNCTION "exception encountered %s", __PRETTY_FUNCTION__, e.what());
        }

        /* If nothing failed, return true for evaluation. */
        return true;
    }
}

#endif
