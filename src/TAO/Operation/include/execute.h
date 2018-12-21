/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

#include <LLC/types/uint1024.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

namespace TAO::Operation
{
    //TODO: stress test and try to break operations transactions. Detect if parameters are sent in wrong order giving deliberate failures

    /** Execute
     *
     *  Executes a given operation byte sequence.
     *
     *  @param[in] regDB The register database to execute on
     *  @param[in] hashOwner The owner executing the register batch.
     *
     *  @return True if operations executed successfully.
     *
     **/
    inline bool Execute(std::vector<uint8_t> vchData, uint256_t hashOwner)
    {
        /* Create the operations stream to execute. */
        Stream stream = Stream(vchData);

        while(!stream.End())
        {
            uint8_t OPERATION;
            stream >> OPERATION;

            /* Check the current opcode. */
            switch(OPERATION)
            {

                /* Record a new state to the register. */
                case OP::WRITE:
                {
                    /* Get the Address of the Register. */
                    uint256_t hashAddress;
                    stream >> hashAddress;

                    /* Deserialize the register from stream. */
                    std::vector<uint8_t> vchData;
                    stream >> vchData;

                    /* Execute the operation method. */
                    if(!Write(hashAddress, vchData, hashOwner))
                        return false;
                }


                /* Create a new register. */
                case OP::REGISTER:
                {
                    /* Extract the address from the stream. */
                    uint256_t hashAddress;
                    stream >> hashAddress;

                    /* Extract the register type from stream. */
                    uint8_t nType;
                    stream >> nType;

                    /* Extract the register data from the stream. */
                    std::vector<uint8_t> vchData;
                    stream >> vchData;

                    /* Execute the operation method. */
                    if(!Register(hashAddress, nType, vchData, hashOwner))
                        return false;
                }


                /* Transfer ownership of a register to another signature chain. */
                case OP::TRANSFER:
                {
                    /* Extract the address from the stream. */
                    uint256_t hashAddress;
                    stream >> hashAddress;

                    /* Read the register transfer recipient. */
                    uint256_t hashTransfer;
                    stream >> hashTransfer;

                    /* Execute the operation method. */
                    if(!Transfer(hashAddress, hashTransfer, hashOwner))
                        return false;
                }

                /* Debit tokens from an account you own. */
                case OP::DEBIT:
                {
                    uint256_t hashFrom; //the register address debit is being sent from. Hard reject if this register isn't account id
                    stream >> hashFrom;

                    uint256_t hashTo;   //the register address debit is being sent to. Hard reject if this register isn't an account id
                    stream >> hashTo;

                    uint64_t  nAmount;  //the amount to be transfered
                    stream >> nAmount;

                    /* Execute the operation method. */
                    if(!Debit(hashFrom, hashTo, nAmount, hashOwner))
                        return false;
                }

                case OP::CREDIT:
                {
                    /* The transaction that this credit is claiming. */
                    uint512_t hashTx;
                    stream >> hashTx;

                    /* The proof this credit is using to make claims. */
                    uint256_t hashProof;
                    stream >> hashProof;

                    /* The account that is being credited. */
                    uint256_t hashAccount;
                    stream >> hashAccount;

                    /* The total to be credited. */
                    uint64_t  nCredit;
                    stream >> nCredit;

                    /* Execute the operation method. */
                    if(!Credit(hashTx, hashProof, hashAccount, nCredit, hashOwner))
                        return false;
                }

                case OP::AUTHORIZE:
                {
                    /* The transaction that you are authorizing. */
                    uint512_t hashTx;
                    stream >> hashTx;

                    /* The proof you are using that you have rights. */
                    uint256_t hashProof;
                    stream >> hashProof;

                    /* Execute the operation method. */
                    if(!Authorize(hashTx, hashProof, hashOwner))
                        return false;
                }

                case OP::VOTE:
                {
                    //voting mechanism. OP_VOTE can be to any random number. Something that can be regarded as a vote for thie hashOWner
                    //consider how to index this from API to OP_READ the votes without having to parse too deep into the register layer
                    //OP_VOTE doesn't need to change states. IT could be a vote read only object register
                }

                /*
                case OP_EXCHANGE:
                {
                    //exchange contracts validation register. hashProof in credits can be used as an exchange medium if OP_DEBIT is to
                    //an exchange object register which holds the token identifier and total in exchange for deposited amount.
                    //OP_DEBIT goes to exchange object and sets its state. another OP_DEBIT from the other token locks this contract in
                    //hash proof for OP_CREDIT on each side allows the OP_DEBIT from opposite token to be claimed
                }
                */

                case OP::SIGNATURE:
                {
                    //a transaction proof that designates the hashOwner or genesisID has signed published data
                    //>> vchData. to prove this data was signed by being published. This can be a hash if needed.
                }
            }
        }

        /* If nothing failed, return true for evaluation. */
        return true;
    }
}

#endif
