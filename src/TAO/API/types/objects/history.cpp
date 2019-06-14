/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/objects.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/write.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* History of an object register and its ownership */
        json::json Objects::History(const json::json& params, uint8_t nType, const std::string& strType)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* The Object to get history for */
            TAO::Register::State state;

            /* Check whether the caller has provided the name parameter. */
            if(params.find("name") != params.end())
            {
                /* Edge case for NAME objects as these do not need to be resolved to an address */
                if( nType == TAO::Register::OBJECTS::NAME)
                    state = Names::GetName(params, params["name"].get<std::string>(), hashRegister);
                else
                    /* If name is provided then use this to deduce the register address */
                    hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");

            /* Get the register if we haven't already loaded it. */
            if(state.IsNull() && !LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-24, "Invalid name / address");

            /* Read the last hash of owner. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(state.hashOwner, hashLast))
                throw APIException(-24, "No history found");

            /* Iterate through sigchain for register updates. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
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
                            uint8_t nRegisterType = 0;
                            contract >> nRegisterType;

                            /* Get the register data. */
                            std::vector<uint8_t> vchData;
                            contract >> vchData;

                            if(nRegisterType != TAO::Register::REGISTER::APPEND
                            && nRegisterType != TAO::Register::REGISTER::RAW
                            && nRegisterType != TAO::Register::REGISTER::OBJECT)
                            {
                                throw APIException(-24, "Specified name/address is not of type " + strType);
                            }

                            /* Create the register object. */
                            TAO::Register::Object state;
                            state.nVersion   = 1;
                            state.nType      = nRegisterType;
                            state.hashOwner  = contract.Caller();

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                                throw APIException(-24, "Contract execution failed");

                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-24, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-24, "Specified name/address is not of type " + strType);
                            }

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]     = "CREATE";
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a NAME object, as those are the edge case that do not have a Name object themselves */
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, nType != TAO::Register::OBJECTS::NAME);

                            /* Copy the asset data in to the response after the type/checksum */
                            obj.insert(data.begin(), data.end());


                            /* Push to return array. */
                            ret.push_back(obj);

                            /* Set hash last to zero to break. */
                            hashLast = 0;

                            break;
                        }


                        case TAO::Operation::OP::WRITE:
                        {
                            /* Get the address. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Get the register data. */
                            std::vector<uint8_t> vchData;
                            contract >> vchData;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]       = "MODIFY";

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::Object state;
                            contract >>= state;

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Write::Execute(state, vchData, contract.Timestamp()))
                                throw APIException(-24, "Contract execution failed");

                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-24, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-24, "Specified name/address is not of type " + strType);
                            }

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a NAME object, as those are the edge case that do not have a Name object themselves */
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, nType != TAO::Register::OBJECTS::NAME);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

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

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::Object state;
                            contract >>= state;

                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-24, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-24, "Specified name/address is not of type " + strType);
                            }

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a NAME object, as those are the edge case that do not have a Name object themselves */
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, nType != TAO::Register::OBJECTS::NAME);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

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
                            obj["type"] = "TRANSFER";
                            

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::Object state;
                            contract >>= state;

                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-24, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-24, "Specified name/address is not of type " + strType);
                            }

                            /* Complete object parameters. */
                            obj["owner"]    = hashTransfer.ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a NAME object, as those are the edge case that do not have a Name object themselves */
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, nType != TAO::Register::OBJECTS::NAME);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

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
