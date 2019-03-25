/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_OUTPUT_H
#define NEXUS_TAO_OPERATION_INCLUDE_OUTPUT_H

#include <LLC/types/uint1024.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <cstring>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        //TODO: stress test and try to break operations transactions. Detect if parameters are sent in wrong order giving deliberate failures

        /** Output
         *
         *  Output an operation script into human readable text.
         *
         **/
        inline std::string Output(TAO::Ledger::Transaction& tx)
        {
            /* Create the output object. */
            std::string strOutput;

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

                        /* Record a new state to the register. */
                        case OP::WRITE:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Deserialize the register from tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Output the string information. */
                            strOutput = debug::safe_printstr
                            (
                                "Write(address=", hashAddress.ToString(), ", ",
                                "data=", HexStr(vchData.begin(), vchData.end()), ")"
                            );

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Append(address=", hashAddress.ToString(), ", ",
                                "data=", HexStr(vchData.begin(), vchData.end()), ")"
                            );

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Register(address=", hashAddress.ToString(), ", ",
                                "type=", nType, ", ",
                                "data=", HexStr(vchData.begin(), vchData.end()), ")"
                            );

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Transfer(address=", hashAddress.ToString(), ", ",
                                "transfer=", hashTransfer.ToString(), ")"
                            );

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Debit(address=", hashAddress.ToString(), ", ",
                                "transfer=", hashTransfer.ToString(), ", ",
                                "amount=", nAmount, ")"
                            );

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Credit(txid=", hashTx.ToString(), ", ",
                                "proof=", hashProof.ToString(), ", ",
                                "account=", hashAccount.ToString(), ", ",
                                "amount=", nCredit, ")"
                            );

                            break;
                        }


                        /* Coinbase operation. Creates an account if none exists. */
                        case OP::COINBASE:
                        {

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            tx.ssOperation >> nCredit;

                            /* The extra nNonce available in script. */
                            uint64_t  nExtraNonce;
                            tx.ssOperation >> nExtraNonce;

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Coinbase(nonce=", nExtraNonce, ", ",
                                "amount=", nCredit, ")"
                            );

                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case OP::TRUST:
                        {

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Trust(account=", hashAccount.ToString(), ", ",
                                "last=", hashLastTrust.ToString(), ", ",
                                "sequence=", nSequence, ", ",
                                "trust", nTrust, ", ",
                                "amount=", nStake, ")"
                            );

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

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Authorize(txid=", hashTx.ToString(), ", ",
                                "proof=", hashProof.ToString(), ")"
                            );

                            break;
                        }


                        case OP::VOTE:
                        {
                            /* The object register to tally the votes for. */
                            uint256_t hashRegister;

                            /* The account that holds the token balance. */
                            uint256_t hashProof;

                            /* Output the string information. */
                            strOutput += debug::safe_printstr
                            (
                                "Vote(register=", hashRegister.ToString(), ", ",
                                "proof=", hashProof.ToString(), ")"
                            );

                            break;
                        }

                        //case OP::SIGNATURE:
                        //{
                            //a transaction proof that designates the hashOwner or genesisID has signed published data
                            //>> vchData. to prove this data was signed by being published. This can be a hash if needed.

                        //    break;
                        //}
                    }
                }
            }
            catch(const std::runtime_error& e)
            {
            }

            /* If nothing failed, return true for evaluation. */
            return strOutput;
        }
    }
}

#endif
