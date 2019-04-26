/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Man often becomes what he believes himself to be." - Mahatma Gandhi

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

#include <LLC/types/uint1024.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>
#include <new> //std::bad_alloc

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        //TODO: stress test and try to break operations transactions. Detect if parameters are sent in wrong order giving deliberate failures

        /** Execute
         *
         *  Executes a given operation byte sequence.
         *
         *  @param[in] regDB The register database to execute on.
         *  @param[in] hashOwner The owner executing the register batch.
         *
         *  @return True if operations executed successfully, false otherwise.
         *
         **/
        inline bool Execute(TAO::Ledger::Transaction& tx, uint8_t nFlags)
        {

            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Start the register stream at the beginning. */
            tx.ssRegister.seek(0, STREAM::BEGIN);


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

                        /* Record a new state to the register. */
                        case OP::WRITE:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Deserialize the register from tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Execute the operation method. */
                            if(!Write(hashAddress, vchData, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Append a new state to the register. */
                        case OP::APPEND:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Deserialize the register from tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Execute the operation method. */
                            if(!Append(hashAddress, vchData, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Create a new register. */
                        case OP::REGISTER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Extract the register type from tx.ssOperation. */
                            uint8_t nType;
                            tx.ssOperation >> nType;

                            /* Extract the register data from the tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Execute the operation method. */
                            if(!Register(hashAddress, nType, vchData, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Transfer ownership of a register to another signature chain. */
                        case OP::TRANSFER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer;
                            tx.ssOperation >> hashTransfer;

                            /* Execute the operation method. */
                            if(!Transfer(hashAddress, hashTransfer, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Transfer ownership of a register to another signature chain. */
                        case OP::CLAIM:
                        {
                            /* The transaction that this transfer is claiming. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* Extract the address from the tx.ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Execute the operation method. */
                            if(!Claim(hashTx, hashAddress, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Debit tokens from an account you own. */
                        case OP::DEBIT:
                        {
                            uint256_t hashAddress; //the register address debit is being sent from. Hard reject if this register isn't account id
                            tx.ssOperation >> hashAddress;

                            uint256_t hashTransfer;   //the register address debit is being sent to. Hard reject if this register isn't an account id
                            tx.ssOperation >> hashTransfer;

                            uint64_t  nAmount;  //the amount to be transfered
                            tx.ssOperation >> nAmount;

                            /* Execute the operation method. */
                            if(!Debit(hashAddress, hashTransfer, nAmount, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Credit tokens to an account you own. */
                        case OP::CREDIT:
                        {
                            /* The transaction that this credit is claiming. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* The proof this credit is using to make claims. */
                            uint256_t hashProof;
                            tx.ssOperation >> hashProof;

                            /* The account that is being credited. */
                            uint256_t hashAccount;
                            tx.ssOperation >> hashAccount;

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            tx.ssOperation >> nCredit;

                            /* Execute the operation method. */
                            if(!Credit(hashTx, hashProof, hashAccount, nCredit, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Coinbase operation. Creates an account if none exists. */
                        case OP::COINBASE:
                        {
                            /* Ensure that it as beginning of the tx.ssOperation. */
                            if(!tx.ssOperation.begin())
                                return debug::error(FUNCTION, "coinbase opeartion has to be first");

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            tx.ssOperation >> nCredit;

                            /* The extra nNonce available in script. */
                            uint64_t  nExtraNonce;
                            tx.ssOperation >> nExtraNonce;

                            /* Ensure that it as end of tx.ssOperation. TODO: coinbase should be followed by ambassador and developer scripts */
                            if(!tx.ssOperation.end())
                                return debug::error(FUNCTION, "coinbase can't have extra data");

                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case OP::TRUST:
                        {
                            /* Ensure that it as beginning of the tx.ssOperation. */
                            if(!tx.ssOperation.begin())
                                return debug::error(FUNCTION, "trust opeartion has to be first");

                            /* The account that is being staked. */
                            uint256_t hashAccount;
                            tx.ssOperation >> hashAccount;

                            /* The previous trust block. */
                            uint1024_t hashLastTrust;
                            tx.ssOperation >> hashLastTrust;

                            /* Previous trust sequence number. */
                            uint32_t nSequence;
                            tx.ssOperation >> nSequence;

                            /* The trust calculated. */
                            uint64_t nTrust;
                            tx.ssOperation >> nTrust;

                            /* The total to be staked. */
                            uint64_t  nStake;
                            tx.ssOperation >> nStake;

                            /* Execute the operation method. */
                            if(!Trust(hashAccount, hashLastTrust, nSequence, nTrust, nStake, nFlags, tx))
                                return false;

                            /* Ensure that it as end of tx.ssOperation. TODO: coinbase should be followed by ambassador and developer scripts */
                            if(!tx.ssOperation.end())
                                return debug::error(FUNCTION, "trust can't have extra data");

                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case OP::GENESIS:
                        {
                            /* Ensure that it as beginning of the tx.ssOperation. */
                            if(!tx.ssOperation.begin())
                                return debug::error(FUNCTION, "trust opeartion has to be first");

                            /* The account that is being staked. */
                            uint256_t hashAccount;
                            tx.ssOperation >> hashAccount;

                            /* The total to be staked. */
                            uint64_t  nStake;
                            tx.ssOperation >> nStake;

                            /* Execute the operation method. */
                            if(!Genesis(hashAccount, nStake, nFlags, tx))
                                return false;

                            /* Ensure that it as end of tx.ssOperation. TODO: coinbase should be followed by ambassador and developer scripts */
                            if(!tx.ssOperation.end())
                                return debug::error(FUNCTION, "trust can't have extra data");

                            break;
                        }


                        /* Authorize a transaction to happen with a temporal proof. */
                        case OP::AUTHORIZE:
                        {
                            /* The transaction that you are authorizing. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* The proof you are using that you have rights. */
                            uint256_t hashProof;
                            tx.ssOperation >> hashProof;

                            /* Execute the operation method. */
                            if(!Authorize(hashTx, hashProof, nFlags, tx))
                                return false;

                            break;
                        }


                        case OP::ACK:
                        {
                            /* The object register to tally the votes for. */
                            uint256_t hashRegister;

                            //TODO: OP::ACK -> tally trust to object register "trust" field.
                            //tally stake to object register "stake" field

                            break;
                        }


                        case OP::NACK:
                        {
                            /* The object register to tally the votes for. */
                            uint256_t hashRegister;

                            //TODO: OP::NACK -> remove trust from LLD
                            //remove stake from object register "stake" field

                            break;
                        }


                        //case OP::SIGNATURE:
                        //{
                            //a transaction proof that designates the hashOwner or genesisID has signed published data
                            //>> vchData. to prove this data was signed by being published. This can be a hash if needed.

                        //    break;
                        //}

                        default:
                            return debug::error(FUNCTION, "operations reached invalid stream state");
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

#endif
