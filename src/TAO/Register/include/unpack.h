/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_UNPACK_H
#define NEXUS_TAO_REGISTER_INCLUDE_UNPACK_H

#include <LLC/types/uint1024.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/state.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <cstring>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Unpack
         *
         *  Unpack a state register declaration from operation scripts.
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, State &state, uint256_t &hashAddress)
        {

            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Make sure no exceptions are thrown. */
            try
            {

                /* Loop through the operations tx.ssOperation. */
                while(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {

                        /* Create a new register. */
                        case TAO::Operation::OP::REGISTER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            tx.ssOperation >> hashAddress;

                            /* Extract the register type from tx.ssOperation. */
                            uint8_t nType;
                            tx.ssOperation >> nType;

                            /* Extract the register data from the tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Set the owner of this register. */
                            if(!LLD::regDB->ReadState(hashAddress, state))
                                return false;

                            return true;
                        }

                        default:
                        {
                            return false;
                        }
                    }
                }
            }
            catch(const std::runtime_error& e)
            {
            }

            return false;
        }


        /** Unpack
         *
         *  Unpack a state register declaration from operation scripts.
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, uint256_t &hashAddress)
        {

            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Make sure no exceptions are thrown. */
            try
            {

                /* Loop through the operations tx.ssOperation. */
                while(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {

                        /* Create a new register. */
                        case TAO::Operation::OP::DEBIT:
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            tx.ssOperation >> hashAddress;

                            return true;
                        }

                        default:
                        {
                            return false;
                        }
                    }
                }
            }
            catch(const std::runtime_error& e)
            {
            }

            return false;
        }


        /** Unpack
         *
         *  Unpack a previous transaction from operation scripts.
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, uint512_t& hashPrevTx)
        {

            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Make sure no exceptions are thrown. */
            try
            {

                /* Loop through the operations tx.ssOperation. */
                while(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {

                        /* Create a new register. */
                        case TAO::Operation::OP::CREDIT:
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            tx.ssOperation >> hashPrevTx;

                            return true;
                        }

                        default:
                        {
                            return false;
                        }
                    }
                }
            }
            catch(const std::runtime_error& e)
            {
            }

            return false;
        }
    }
}

#endif
