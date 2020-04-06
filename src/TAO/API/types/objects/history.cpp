/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/objects.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/write.h>
#include <TAO/Operation/include/append.h>

#include <TAO/Ledger/types/transaction.h>

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
            TAO::Register::Address hashRegister ;

            /* The Object to get history for */
            TAO::Register::State state;

            /* Check whether the caller has provided the name parameter. */
            if(params.find("name") != params.end())
            {
                /* Edge case for name objects as these do not need to be resolved to an address */
                if(nType == TAO::Register::OBJECTS::NAME)
                    state = Names::GetName(params, params["name"].get<std::string>(), hashRegister);
                /* Edge case for namespace objects as the address is a hash of the name */
                else if(nType == TAO::Register::OBJECTS::NAMESPACE)
                    hashRegister = LLC::SK256(params["name"].get<std::string>());
                else
                    /* If name is provided then use this to deduce the register address */
                    hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");

            /* Get the register if we haven't already loaded it. */
            if(state.IsNull() && !LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-106, "Invalid name / address");

            /* Make adjustment to history check and detect if the register is owned by system. */
            uint256_t hashOwner = state.hashOwner;
            if(hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
                hashOwner.SetType(TAO::Ledger::GenesisType());

            if(config::fClient.load() && hashOwner != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Read the last hash of owner. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashOwner, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-107, "No history found");

            /* Iterate through sigchain for register updates. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

                /* Check through all the contracts. */
                for(int32_t nContract = tx.Size() - 1; nContract >= 0; --nContract)
                {
                    /* Get the contract. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                    contract.Reset();

                    /* Get the operation byte. */
                    uint8_t OPERATION = 0;
                    contract >> OPERATION;

                    /* Check for conditional OP */
                    switch(OPERATION)
                    {
                        case TAO::Operation::OP::VALIDATE:
                        {
                            /* Seek through validate. */
                            contract.Seek(68);
                            contract >> OPERATION;

                            break;
                        }

                        case TAO::Operation::OP::CONDITION:
                        {
                            /* Get new operation. */
                            contract >> OPERATION;
                        }
                    }

                    /* Check for key operations. */
                    switch(OPERATION)
                    {
                        /* Break when at the register declaration. */
                        case TAO::Operation::OP::CREATE:
                        {
                            /* Get the Address of the Register. */
                            TAO::Register::Address hashAddress;
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

                            /* Check the register type */
                            if(nRegisterType != TAO::Register::REGISTER::APPEND
                            && nRegisterType != TAO::Register::REGISTER::RAW
                            && nRegisterType != TAO::Register::REGISTER::READONLY
                            && nRegisterType != TAO::Register::REGISTER::OBJECT)
                            {
                                throw APIException(-109, "Specified name/address is not of type " + strType);
                            }

                            /* Create the register object. */
                            TAO::Register::Object state;
                            state.nVersion   = 1;
                            state.nType      = nRegisterType;
                            state.hashOwner  = contract.Caller();

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                                throw APIException(-110, "Contract execution failed");

                            /* If it is an object register then parse so that we can check the type */
                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-36, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-109, "Specified name/address is not of type " + strType);
                            }

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]     = "CREATE";
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a name of namesace object, as those are the edge case that do not have a Name object themselves */
                            bool fLookupName = nType != TAO::Register::OBJECTS::NAME && nType != TAO::Register::OBJECTS::NAMESPACE;
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, fLookupName);

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
                            TAO::Register::Address hashAddress;
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
                                throw APIException(-110, "Contract execution failed");

                            if(state.nType == TAO::Register::REGISTER::OBJECT)
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!state.Parse())
                                    throw APIException(-36, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-109, "Specified name/address is not of type " + strType);
                            }

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a name of namesace object, as those are the edge case that do not have a Name object themselves */
                            bool fLookupName = nType != TAO::Register::OBJECTS::NAME && nType != TAO::Register::OBJECTS::NAMESPACE;
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, fLookupName);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

                            /* Push to return array. */
                            ret.push_back(obj);

                            break;
                        }


                        case TAO::Operation::OP::APPEND:
                        {
                            /* Get the address. */
                            TAO::Register::Address hashAddress;
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
                            TAO::Register::State state;
                            contract >>= state;

                            /* Get the post state, as this is what we need to output for the history */
                            if(!TAO::Operation::Append::Execute(state, vchData, contract.Timestamp()))
                                throw APIException(-110, "Contract execution failed");

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a name of namesace object, as those are the edge case that do not have a Name object themselves */
                            bool fLookupName = (nType != TAO::Register::OBJECTS::NAME && nType != TAO::Register::OBJECTS::NAMESPACE);
                            json::json data  = TAO::API::ObjectToJSON(params, state, hashRegister, fLookupName);

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
                            TAO::Register::Address hashAddress;
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
                                    throw APIException(-36, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-109, "Specified name/address is not of type " + strType);
                            }

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a name of namesace object, as those are the edge case that do not have a Name object themselves */
                            bool fLookupName = nType != TAO::Register::OBJECTS::NAME && nType != TAO::Register::OBJECTS::NAMESPACE;
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, fLookupName);

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
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Read the register transfer recipient. */
                            TAO::Register::Address hashTransfer;
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
                                    throw APIException(-36, "Failed to parse object register");

                                /* Only include object registers of the specified type */
                                if(state.Standard() != nType)
                                    throw APIException(-109, "Specified name/address is not of type " + strType);
                            }

                            /* Complete object parameters. */
                            obj["owner"]    = hashTransfer.ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            /* Get the JSON data for this object.  NOTE that we pass false for fLookupName if the requested type
                               is a name of namesace object, as those are the edge case that do not have a Name object themselves */
                            bool fLookupName = nType != TAO::Register::OBJECTS::NAME && nType != TAO::Register::OBJECTS::NAMESPACE;
                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, fLookupName);

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
