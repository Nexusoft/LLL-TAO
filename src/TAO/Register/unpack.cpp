/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/unpack.h>

#include <LLD/include/global.h>

#include <TAO/Operation/types/stream.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/create.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <cstring>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Unpack a state register from operation scripts. */
        bool Unpack(const TAO::Operation::Contract& contract, State &state, uint256_t &hashAddress)
        {
            /* Reset the contract to the position of the primitive. */
            contract.SeekToPrimitive();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* De-Serialize the operation. */
                uint8_t OPERATION = 0;
                contract >> OPERATION;

                /* Check the current opcode. */
                switch(OPERATION)
                {
                    /* Create a new register. */
                    case TAO::Operation::OP::CREATE:
                    {
                        /* Extract the address from the contractn. */
                        contract >> hashAddress;

                        /* Extract the register type from contractn. */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Extract the register data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Create the register object. */
                        state.nVersion   = 1;
                        state.nType      = nType;
                        state.hashOwner  = contract.Caller();

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        return true;
                    }

                    default:
                    {
                        return false;
                    }
                }
            }
            catch(const std::exception& e)
            {
            }

            return false;
        }


        /* Unpack a source register address from operation scripts. */
        bool Unpack(const TAO::Operation::Contract& contract, uint256_t &hashAddress)
        {
            /* Reset the contract to the position of the primitive. */
            contract.SeekToPrimitive();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Deserialize the operation. */
                uint8_t OPERATION = 0;
                contract >> OPERATION;

                /* Check the current opcode. */
                switch(OPERATION)
                {
                    /* Create a new register. */
                    case TAO::Operation::OP::DEBIT:
                    case TAO::Operation::OP::LEGACY:
                    case TAO::Operation::OP::TRANSFER:
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Extract the address from the contract. */
                        contract >> hashAddress;

                        return true;
                    }

                    default:
                    {
                        return false;
                    }
                }
            }
            catch(const std::exception& e)
            {
            }

            return false;
        }


        /* Unpack a previous transaction from operation scripts. */
        bool Unpack(const TAO::Operation::Contract& contract, uint512_t& hashPrevTx, uint32_t& nContract)
        {
            /* Reset the contract to the position of the primitive. */
            contract.SeekToPrimitive();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Deserialize the operation. */
                uint8_t OPERATION = 0;
                contract >> OPERATION;

                /* Check the current opcode. */
                switch(OPERATION)
                {
                    /* Create a new register. */
                    case TAO::Operation::OP::CREDIT:
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Extract the previous tx hash from the contract. */
                        contract >> hashPrevTx;

                        /* Extract the previous contact ID from the contract */
                        contract >> nContract;
                        
                        return true;
                    }

                    default:
                    {
                        return false;
                    }
                }
            }
            catch(const std::exception& e)
            {
            }

            return false;
        }


        /* Unpack the amount of NXS in contract. */
        bool Unpack(const TAO::Operation::Contract& contract, uint64_t& nAmount)
        {
            /* Reset the contract to the position of the primitive. */
            contract.SeekToPrimitive();
            nAmount = 0;

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Deserialize the operation. */
                uint8_t OPERATION = 0;
                contract >> OPERATION;

                /* Check the current opcode. */
                switch(OPERATION)
                {
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Seek to coinbase/coinstake. */
                        contract.Seek(32);

                        contract >> nAmount;

                        return true;
                    }

                    case TAO::Operation::OP::TRUST:
                    {
                        /* Seek to coinstake. */
                        contract.Seek(80);

                        contract >> nAmount;

                        return true;
                    }

                    case TAO::Operation::OP::GENESIS:
                    {
                        contract >> nAmount;

                        return true;
                    }

                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Seek to debit amount. */
                        contract.Seek(64);

                        contract >> nAmount;

                        return true;
                    }

                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Seek to credit amount. */
                        contract.Seek(132);

                        contract >> nAmount;

                        return true;
                    }

                    case TAO::Operation::OP::MIGRATE:
                    {
                        /* Seek to migrate amount. */
                        contract.Seek(168);

                        contract >> nAmount;

                        return true;
                    }

                    case TAO::Operation::OP::LEGACY:
                    {
                        /* Seek to debit amount. */
                        contract.Seek(32);

                        contract >> nAmount;

                        return true;
                    }

                    default:
                    {
                        nAmount = 0;
                        return false;
                    }
                }
            }
            catch(const std::exception& e)
            {
            }

            return false;

        }


        /* Unpack a transaction and test for the operation it contains. */
        bool Unpack(const TAO::Operation::Contract& contract, const uint8_t nCode)
        {
            return contract.Primitive() == nCode;
        }

        /* Unpack an op legacy contract to find it's output script. */
        bool Unpack(const TAO::Operation::Contract& contract, Legacy::Script& script)
        {
            /* Reset the contract to the position of the primitive. */
            contract.SeekToPrimitive();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Deserialize the operation. */
                uint8_t OPERATION = 0;
                contract >> OPERATION;

                /* Check the current opcode. */
                switch(OPERATION)
                {
                    /* Check the op code. */
                    case TAO::Operation::OP::LEGACY:

                    {
                        /* Seek to output script. */
                        contract.Seek(40);

                        contract >> script;

                        return true;
                    }

                    default:
                    {
                        return false;
                    }
                }
            }
            catch(const std::exception& e)
            {
            }

            return false;
        }
        
    }
}
