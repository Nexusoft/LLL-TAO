/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/users.h>
#include <TAO/API/include/supply.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLC/include/random.h>
#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Gets the history of an item. */
        json::json Supply::History(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Check for username parameter. */
            if(params.find("address") == params.end())
                throw APIException(-23, "Missing memory address");

            /* Get the Register ID. */
            uint256_t hashRegister;
            hashRegister.SetHex(params["address"].get<std::string>());

            /* Get the register. */
            TAO::Register::State state;
            if(!LLD::regDB->ReadState(hashRegister, state))
                throw APIException(-24, "No state found");

            /* Generate return object. */
            json::json first;
            first["owner"]      = state.hashOwner.ToString();
            first["timestamp"]  = state.nTimestamp;

            /* Reset read position. */
            state.nReadPos = 0;

            /* Grab the last state. */
            while(!state.end())
            {
                /* If the data type is string. */
                std::string data;
                state >> data;

                first["checksum"] = state.hashChecksum;
                first["state"]    = data;
            }

            /* Push to return array. */
            ret.push_back(first);

            /* Read the last hash of owner. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(state.hashOwner, hashLast))
                throw APIException(-24, "No last hash found");

            /* Iterate through sigchain for register updates. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Get the operation byte. */
                uint8_t nType = 0;
                tx.ssOperation >> nType;

                /* Check for key operations. */
                TAO::Register::State state;
                switch(nType)
                {
                    /* Break when at the register declaration. */
                    case TAO::Operation::OP::REGISTER:
                    {
                        /* Set hash last to zero to break. */
                        hashLast = 0;

                        break;
                    }

                    case TAO::Operation::OP::APPEND:
                    {
                        /* Seek past pre-state. */
                        tx.ssRegister.seek(1);

                        /* De-Serialize state register. */
                        tx.ssRegister >> state;

                        /* Generate return object. */
                        json::json obj;
                        obj["OP"]         = "APPEND";
                        obj["owner"]      = state.hashOwner.ToString();
                        obj["timestamp"]  = state.nTimestamp;

                        /* Reset read position. */
                        state.nReadPos = 0;

                        /* Grab the last state. */
                        while(!state.end())
                        {
                            /* If the data type is string. */
                            std::string data;
                            state >> data;

                            obj["checksum"] = state.hashChecksum;
                            obj["state"]    = data;
                        }

                        /* Push to return array. */
                        ret.push_back(obj);

                        break;
                    }

                    case TAO::Operation::OP::WRITE:
                    {
                        /* Seek past pre-state. */
                        tx.ssRegister.seek(1);

                        /* De-Serialize state register. */
                        tx.ssRegister >> state;

                        /* Generate return object. */
                        json::json obj;
                        obj["OP"]         = "WRITE";
                        obj["owner"]      = state.hashOwner.ToString();
                        obj["timestamp"]  = state.nTimestamp;

                        /* Reset read position. */
                        state.nReadPos = 0;

                        /* Grab the last state. */
                        while(!state.end())
                        {
                            /* If the data type is string. */
                            std::string data;
                            state >> data;

                            obj["checksum"] = state.hashChecksum;
                            obj["state"]    = data;
                        }

                        /* Push to return array. */
                        ret.push_back(obj);

                        break;
                    }

                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Jump to proper sigchain. */
                        tx.ssOperation >> hashLast;

                        break;
                    }

                    /* Get old owner from transfer. */
                    case TAO::Operation::OP::TRANSFER:
                    {

                        break;
                    }

                    default:
                        continue;
                }
            }

            return ret;
        }
    }
}
