/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

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
namespace TAO::Register
{
    /* Unpack a state register from operation scripts. */
    bool Unpack(const TAO::Operation::Contract& rContract, State &state, uint256_t &hashAddress)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.SeekToPrimitive();

        /* Make sure no exceptions are thrown. */
        try
        {
            /* De-Serialize the operation. */
            uint8_t OPERATION = 0;
            rContract >> OPERATION;

            /* Check the current opcode. */
            switch(OPERATION)
            {
                /* Create a new register. */
                case TAO::Operation::OP::CREATE:
                {
                    /* Extract the address from the rContract. */
                    rContract >> hashAddress;

                    /* Extract the register type from rContract. */
                    uint8_t nType = 0;
                    rContract >> nType;

                    /* Extract the register data. */
                    std::vector<uint8_t> vchData;
                    rContract >> vchData;

                    /* Create the register object. */
                    state.nVersion   = 1;
                    state.nType      = nType;
                    state.hashOwner  = rContract.Caller();

                    /* Calculate the new operation. */
                    if(!TAO::Operation::Create::Execute(state, vchData, rContract.Timestamp()))
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
    bool Unpack(const TAO::Operation::Contract& rContract, uint256_t &hashAddress)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.SeekToPrimitive();

        /* Make sure no exceptions are thrown. */
        try
        {
            /* Deserialize the operation. */
            uint8_t nOP = 0;
            rContract >> nOP;

            /* Check the current opcode. */
            switch(nOP)
            {
                /* Operations that lead with address. */
                case TAO::Operation::OP::WRITE:
                case TAO::Operation::OP::APPEND:
                case TAO::Operation::OP::CREATE:
                case TAO::Operation::OP::TRANSFER:
                case TAO::Operation::OP::DEBIT:
                case TAO::Operation::OP::FEE:
                case TAO::Operation::OP::LEGACY:
                {
                    /* Extract the address from the rContract. */
                    rContract >> hashAddress;

                    return true;
                }

                /* Operations that only contain txid. */
                case TAO::Operation::OP::MIGRATE:
                {
                    /* Extract the rContract calling address. */
                    rContract.Seek(64); //seek over our txid
                    rContract >> hashAddress;

                    return true;
                }

                /* Operations that have a txid and rContract-id. */
                case TAO::Operation::OP::CLAIM:
                case TAO::Operation::OP::CREDIT:
                {
                    /* Extract the rContract calling address. */
                    rContract.Seek(68); //seek over our txid and rContract-id
                    rContract >> hashAddress;

                    return true;
                }

                /* Address is generated for trust. */
                case TAO::Operation::OP::GENESIS:
                case TAO::Operation::OP::TRUST:
                {
                    /* Get trust account address for rContract caller */
                    hashAddress =  //XXX: we may want to remove this section, as this has a high overhead.
                        TAO::Register::Address(std::string("trust"), rContract.Caller(), TAO::Register::Address::TRUST);

                    return true;
                }
            }
        }
        catch(const std::exception& e)
        {
        }

        return false;
    }


    /* Unpack a previous transaction from operation scripts. */
    bool Unpack(const TAO::Operation::Contract& rContract, uint512_t& hashPrevTx)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.SeekToPrimitive();

        /* Make sure no exceptions are thrown. */
        try
        {
            /* Deserialize the operation. */
            uint8_t OPERATION = 0;
            rContract >> OPERATION;

            /* Check the current opcode. */
            switch(OPERATION)
            {
                /* Create a new register. */
                case TAO::Operation::OP::CREDIT:
                case TAO::Operation::OP::CLAIM:
                {
                    /* Extract the previous tx hash from the rContract. */
                    rContract >> hashPrevTx;

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


    /* Unpacks a source txid and contract from an OP::VALIDATE transaction. */
    bool Unpack(const TAO::Operation::Contract& rContract, uint512_t& hashPrevTx, uint32_t& nContract)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.Reset(TAO::Operation::Contract::OPERATIONS);

        /* Make sure no exceptions are thrown. */
        try
        {
            /* Deserialize the operation. */
            uint8_t OPERATION = 0;
            rContract >> OPERATION;

            /* Check the current opcode. */
            if(OPERATION != TAO::Operation::OP::VALIDATE)
                return false;

            /* Extract the previous tx hash from the rContract. */
            rContract >> hashPrevTx;

            /* Extract the previous contact ID from the rContract */
            rContract >> nContract;

            return true;
        }
        catch(const std::exception& e)
        {
        }

        return false;
    }


    /* Unpacks the values used to read and write proofs for any given transaction type. */
    bool Unpack(const TAO::Operation::Contract& rContract, uint256_t& hashProof, uint512_t& hashPrevTx, uint32_t& nContract)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.SeekToPrimitive();

        /* Make sure no exceptions are thrown. */
        try
        {
            /* Deserialize the operation. */
            uint8_t OPERATION = 0;
            rContract >> OPERATION;

            /* Check the current opcode. */
            switch(OPERATION)
            {
                /* Check for the CREDIT opcode. */
                case TAO::Operation::OP::CREDIT:
                {
                    /* Extract the binary data from contract streams. */
                    rContract >> hashPrevTx;
                    rContract >> nContract;

                    /* Jump past our address so we get to the proof section. */
                    rContract.Seek(32);

                    /* Now extract our proof data. */
                    rContract >> hashProof;

                    return true;
                }

                /* Check for the CLAIM opcode. */
                case TAO::Operation::OP::CLAIM:
                {
                    /* Extract the binary data from contract streams. */
                    rContract >> hashPrevTx;
                    rContract >> nContract;
                    rContract >> hashProof;

                    return true;
                }
            }
        }
        catch(const std::exception& e)
        {
        }

        return false;
    }


    /* Unpack the amount of NXS in rContract. */
    bool Unpack(const TAO::Operation::Contract& rContract, uint64_t& nAmount)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.SeekToPrimitive();
        nAmount = 0;

        /* Make sure no exceptions are thrown. */
        try
        {
            /* Deserialize the operation. */
            uint8_t OPERATION = 0;
            rContract >> OPERATION;

            /* Check the current opcode. */
            switch(OPERATION)
            {
                case TAO::Operation::OP::COINBASE:
                {
                    /* Seek to coinbase/coinstake. */
                    rContract.Seek(32);
                    rContract >> nAmount;

                    return true;
                }

                case TAO::Operation::OP::TRUST:
                {
                    /* Seek to reward. */
                    rContract.Seek(80);
                    rContract >> nAmount;

                    return true;
                }

                case TAO::Operation::OP::GENESIS:
                {
                    rContract >> nAmount;

                    return true;
                }

                case TAO::Operation::OP::DEBIT:
                {
                    /* Seek to debit amount. */
                    rContract.Seek(64);
                    rContract >> nAmount;

                    return true;
                }

                case TAO::Operation::OP::CREDIT:
                {
                    /* Seek to credit amount. */
                    rContract.Seek(132);
                    rContract >> nAmount;

                    return true;
                }

                case TAO::Operation::OP::MIGRATE:
                {
                    /* Seek to migrate amount. */
                    rContract.Seek(168);
                    rContract >> nAmount;

                    return true;
                }

                case TAO::Operation::OP::LEGACY:
                {
                    /* Seek to debit amount. */
                    rContract.Seek(32);
                    rContract >> nAmount;

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


    /* Unpack an op legacy rContract to find it's output script. */
    bool Unpack(const TAO::Operation::Contract& rContract, Legacy::Script& script)
    {
        /* Reset the contract to the position of the primitive. */
        rContract.SeekToPrimitive();

        /* Make sure no exceptions are thrown. */
        try
        {
            /* Deserialize the operation. */
            uint8_t OPERATION = 0;
            rContract >> OPERATION;

            /* Check the current opcode. */
            switch(OPERATION)
            {
                /* Check the op code. */
                case TAO::Operation::OP::LEGACY:
                {
                    /* Seek to output script. */
                    rContract.Seek(40);

                    rContract >> script;

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
