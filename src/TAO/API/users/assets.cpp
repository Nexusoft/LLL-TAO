/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unordered_set>

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a list of assets owned by a signature chain. */
        json::json Users::Assets(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;// = json::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Watch for destination genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else if(!config::fAPISessions.load() && mapSessions.count(0))
                hashGenesis = mapSessions[0]->Genesis();
            else
                throw APIException(-25, "Missing Genesis or Username");

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* In order to work out which registers are currently owned by a particular sig chain
               we must iterate through all of the transactions for the sig chain and track the history 
               of each register.  By  iterating through the transactions from most recent backwards we 
               can make some assumptions about the current owned state.  For example if we find a debit 
               transaction for a register before finding a transfer then we must know we currently own it. 
               Similarly if we find a transfer transaction for a register before any other transaction 
               then we must know we currently to NOT own it. */

            /* Keep a running list of owned and transferred registers. We use a set to store these registers because 
               we are going to be checking them frequently to see if a hash is already in the container, 
               and a set offers us near linear search time*/
            std::unordered_set<uint256_t> vTransferredRegisters;
            std::unordered_set<uint256_t> vOwnedRegisters;
            
            /* For also put the owned register hashes in a vector as we want to maintain the insertion order, with the 
               most recently updated register first*/
            std::vector<uint256_t> vRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Iterate through the operations in each transaction */
                /* Start the stream at the beginning. */
                tx.ssOperation.seek(0, STREAM::BEGIN);

                if(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {
                        /* These are the register-based operations that prove ownership if encountered before a transfer*/
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        case TAO::Operation::OP::REGISTER:
                        case TAO::Operation::OP::DEBIT:
                        case TAO::Operation::OP::TRUST:
                        {
                            /* Extract the address from the ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* for these operations, if the address is NOT in the transferred list 
                               then we know that we must currently own this register */
                            if( vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end() 
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                vOwnedRegisters.insert(hashAddress);
                                vRegisters.push_back(hashAddress);
                            }

                            break;
                        }
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* If we find a TRANSFER before any other transaction for this register then we can know 
                               for certain that we no longer own it */
                            if( vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end() 
                            && vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end())
                            {
                                vTransferredRegisters.insert(hashAddress);
                            }
                            
                            if( std::find(vOwnedRegisters.begin(), vOwnedRegisters.end(), hashAddress) == vOwnedRegisters.end() )
                                vTransferredRegisters.insert(hashAddress);
                            
                            break;
                        }

                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the tx id of the corresponding transfer from the ssOperation. */
                            uint512_t hashTransferTx;
                            tx.ssOperation >> hashTransferTx;

                            /* Read the claimed transaction. */
                            TAO::Ledger::Transaction txClaim;
                            
                            /* Check disk of writing new block. */
                            if((!LLD::legDB->ReadTx(hashTransferTx, txClaim) || !LLD::legDB->HasIndex(hashTransferTx)))
                                return debug::error(FUNCTION, hashTransferTx.ToString(), " tx doesn't exist or not indexed");

                            /* Check mempool or disk if not writing. */
                            else if(!TAO::Ledger::mempool.Get(hashTransferTx, txClaim)
                            && !LLD::legDB->ReadTx(hashTransferTx, txClaim))
                                return debug::error(FUNCTION, hashTransferTx.ToString(), " tx doesn't exist");

                            /* Extract the state from tx. */
                            uint8_t TX_OP;
                            txClaim.ssOperation >> TX_OP;

                            /* Extract the address  */
                            uint256_t hashAddress;
                            txClaim.ssOperation >> hashAddress;
                            

                            /* If we find a CLAIM transaction before a TRANSFER, then we know that we must currently own this register */
                            if( vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end() 
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                vOwnedRegisters.insert(hashAddress);
                                vRegisters.push_back(hashAddress);
                            }
                            
                            break;
                        }

                    }
                }

            }

            uint32_t nTotal = 0;
            /* Add the register data to the response */
            for(const auto& hashRegister : vRegisters)
            {
                /* Get the asset from the register DB.  We can read it as an Object.
                If this fails then we try to read it as a base State type and assume it was
                created as a raw format asset */
                TAO::Register::Object object;
                if(!LLD::regDB->ReadState(hashRegister, object))
                    throw APIException(-24, "Asset not found");

                /* Only include raw and non-standard object types (assets)*/
                if( object.nType != TAO::Register::REGISTER::APPEND 
                    && object.nType != TAO::Register::REGISTER::RAW 
                    && object.nType != TAO::Register::REGISTER::OBJECT)
                {
                    continue;
                }

                if(object.nType == TAO::Register::REGISTER::OBJECT)
                {
                    /* parse object so that the data fields can be accessed */
                    object.Parse();

                    if( object.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                        continue;
                }

                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;
                    
                /* Convert the object to JSON */
                ret.push_back(TAO::API::ObjectRegisterToJSON(object, hashRegister));


            }

            return ret;
        }
    }
}
