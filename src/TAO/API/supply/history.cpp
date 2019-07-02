/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/users.h>
#include <TAO/API/include/supply.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/jsonutils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/append.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

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
            json::json ret = json::json::array();

            /* Get the register address. */
            uint256_t hashRegister = 0;

            /* If name is provided then use this to deduce the register address */
            if(params.find("name") != params.end())
                hashRegister = AddressFromName(params, params["name"].get<std::string>());

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");

            /* Get the register. */
            TAO::Register::State state;
            if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-24, "Item not found");

            /* Read the last hash of owner. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(state.hashOwner, hashLast))
                throw APIException(-24, "No last hash found");

            /* Iterate through sigchain for register updates. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Check through all the contracts. */
                for(int32_t nContract = tx.Size() - 1; nContract >= 0; --nContract)
                {
                    /* Get the contract. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Get the operation byte. */
                    uint8_t OPERATION = 0;
                    contract >> OPERATION;

                    /* Check for key operations. */
                    switch(OPERATION)
                    {
                        /* Break when at the register declaration. */
                        case TAO::Operation::OP::CREATE:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Get the Register Type. */
                            uint8_t nType = 0;
                            contract >> nType;

                            /* Get the register data. */
                            std::vector<uint8_t> vchData;
                            contract >> vchData;

                            if(nType != TAO::Register::REGISTER::APPEND
                            && nType != TAO::Register::REGISTER::RAW
                            && nType != TAO::Register::REGISTER::OBJECT)
                            {
                                throw APIException(-24, "Specified name/address is not an asset.");
                            }

                            /* Create the register object. */
                            TAO::Register::Object state;
                            state.nVersion   = 1;
                            state.nType      = nType;
                            state.hashOwner  = contract.Caller();

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                                return false;

                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-24, "Failed to parse object register");

                                /* Only include non standard object registers (assets) */
                                if(state.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                                    throw APIException(-24, "Specified name/address is not an asset.");
                            }

                            /* Generate return object. */
                            json::json obj;
                            obj["type"] = "CREATE";
                            obj["checksum"] = state.hashChecksum;

                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister);

                            /* Copy the asset data in to the response after the type/checksum */
                            obj.insert(data.begin(), data.end());


                            /* Push to return array. */
                            ret.push_back(obj);

                            /* Set hash last to zero to break. */
                            hashLast = 0;

                            break;
                        }

                        case TAO::Operation::OP::APPEND:
                        {
                            /* Get the address. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Get the data to write. */
                            std::vector<uint8_t> vchData;
                            contract >> vchData;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]       = "MODIFY";
                            obj["owner"]      = contract.Caller().ToString();

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::State state;
                            contract >>= state;

                            /* Handle for append registers. */
                            if(!TAO::Operation::Append::Execute(state, vchData, contract.Timestamp()))
                                return false;

                            /* Complete object parameters. */
                            obj["modified"]   = state.nModified;
                            obj["created"]    = state.nCreated;

                            /* Reset read position. */
                            state.nReadPos = 0;

                            /* Grab the last state. */
                            while(!state.end())
                            {
                                /* If the data type is string. */
                                std::string data;
                                state >> data;

                                obj["checksum"] = state.hashChecksum;
                                obj["data"]     = data;
                            }

                            /* Push to return array. */
                            ret.push_back(obj);

                            break;
                        }


                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nContract = 0;
                            contract >> nContract;

                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]       = "CLAIM";
                            obj["owner"]      = contract.Caller().ToString();

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::State state;
                            contract >>= state;

                            /* Complete object parameters. */
                            obj["modified"]   = state.nModified;
                            obj["created"]    = state.nCreated;

                            /* Push to return array. */
                            ret.push_back(obj);

                            /* Get the previous txid. */
                            hashLast = hashTx;

                            break;
                        }

                        /* Get old owner from transfer. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer = 0;
                            contract >> hashTransfer;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]       = "TRANSFER";
                            obj["owner"]      = hashTransfer.ToString();

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::State state;
                            contract >>= state;

                            /* Complete object parameters. */
                            obj["modified"]   = state.nModified;
                            obj["created"]    = state.nCreated;

                            /* Push to return array. */
                            ret.push_back(obj);

                            break;
                        }

                        default:
                            continue;
                    }
                }
            }

            return ret;
        }
    }
}
