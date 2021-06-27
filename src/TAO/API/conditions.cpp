/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/list.h>

#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Determines if a contract contains a specific condition.  The method searches the nBytes of the conditions stream to see
    *  if the pattern for condition exists somewhere within it */
    bool HasCondition(const TAO::Operation::Contract& contract, const TAO::Operation::Contract& condition)
    {
        bool fHasCondition = false;

        /* Reset the contract conditions stream */
        contract.Reset(TAO::Operation::Contract::CONDITIONS);

        /* search the contract to find the first instance of the same nByte */
        while(!contract.End(TAO::Operation::Contract::CONDITIONS) && !fHasCondition)
        {
            /* Reset the conditions stream on the expiration as we are starting a new comparison */
            condition.Reset(TAO::Operation::Contract::CONDITIONS);

            /* Get nByte from the expiration condition */
            uint8_t nByte = 0;
            condition >= nByte;

            /* Get nByte to nCompare from the comparison contract */
            uint8_t nCompare = 0;
            contract >= nCompare;

            /* Cache the stream position so we can reset to here if a match is not found */
            uint64_t nPos = contract.Position(TAO::Operation::Contract::CONDITIONS);

            /* If the nByte matches then start the rest of the comparison */
            if(nCompare == nByte)
            {
                while(!condition.End(TAO::Operation::Contract::CONDITIONS) && !contract.End(TAO::Operation::Contract::CONDITIONS))
                {
                    condition >= nByte;
                    contract >= nCompare;

                    /* Check the nBytes match.  If not then we can break out of this comparison */
                    if(nByte != nCompare)
                        break;

                    /* If this is an OP::TYPES then we need to skip both stream ahead by the number of nBytes, as this is caller
                       specific data and won't match - we are only comparing the operations and logic, not the data */
                    switch(nByte)
                    {
                        case TAO::Operation::OP::TYPES::UINT8_T :
                        {
                            condition.Seek(1, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(1, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::UINT16_T :
                        {
                            condition.Seek(2, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(2, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::UINT32_T :
                        {
                            condition.Seek(4, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(4, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::UINT64_T :
                        {
                            condition.Seek(8, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(8, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::UINT256_T :
                        {
                            condition.Seek(32, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(32, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::UINT512_T :
                        {
                            condition.Seek(64, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(64, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::UINT1024_T :
                        {
                            condition.Seek(128, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(128, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                        case TAO::Operation::OP::TYPES::BYTES :
                        {
                            std::vector<uint8_t> nBytes;
                            condition >= nBytes;
                            contract >= nBytes;
                            break;
                        }
                        case TAO::Operation::OP::TYPES::STRING :
                        {
                            std::string string;
                            condition >= string;
                            contract >= string;
                            break;
                        }
                        case TAO::Operation::OP::SUBDATA :
                        {
                            /* OP::SUBDATA has two uint16_t's for the start pos and length, so skip 4 nBytes */
                            condition.Seek(4, TAO::Operation::Contract::CONDITIONS);
                            contract.Seek(4, TAO::Operation::Contract::CONDITIONS);
                            break;
                        }
                    }
                }

                /* If we got to the end of the expiration contract without breaking out then we must have a match */
                if(condition.End(TAO::Operation::Contract::CONDITIONS))
                    fHasCondition = true;
            }

            /* If we didn't find a condition at this point, then seek to the point that we last checked and continue checking */
            if(!fHasCondition)
                contract.Seek(nPos, TAO::Operation::Contract::CONDITIONS, STREAM::BEGIN);
        }


        return fHasCondition;
    }


    /* Checks the params for the existence of the "expires" field.  If supplied, a condition will be added to this contract
    *  for the expiration */
    bool AddExpires(const encoding::json& params, const uint256_t& hashCaller, TAO::Operation::Contract& contract, bool fTokenizedDebit)
    {
        using namespace TAO::Operation;

        /* Flag indicating the condition was added */
        bool fAdded = false;

        /* The optional expiration time */
        uint64_t nExpires = 0;

        /* Check that the caller has supplied the 'expires' parameter */
        if(params.find("expires") != params.end())
        {
            /* The expiration time as a string */
            std::string strExpires = params["expires"].get<std::string>();

            /* Check that the expiration time contains only numeric characters and that it can be converted to a uint64
               before attempting to convert it */
            if(!IsAllDigit(strExpires) || !IsUINT64(strExpires))
                throw Exception(-168, "Invalid expiration time");

            /* Convert the expiration time to uint64 */
            nExpires = stoull(strExpires);
        }
        else
        {
            /* Default Expiration of 7 days (604800 seconds) */
            nExpires = 604800;
        }

        /* Check if there is an expiry set for the contract */
        if(nExpires > 0)
        {
            /* Add conditional statements to only allow the transaction to be credited before the expiration time. */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::NOTEQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::AND);
            contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
            contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::LEDGER::TIMESTAMP);
            contract <= uint8_t(OP::UNGROUP);

            contract <= uint8_t(OP::OR);

            /* Add condition to prevent the sender from reversing the transaction until after the expiration time */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::AND);
            contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
            contract <= uint8_t(OP::LESSTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);
            contract <= uint8_t(OP::UNGROUP);

            /* If the contract is a debit to a tokenized asset then add an additional clause to bypass the expiration if the
            recipient is from the sending sig chain  */
            if(fTokenizedDebit)
            {
                contract <= uint8_t(OP::OR);

                contract <= uint8_t(OP::GROUP);
                contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
                contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
                contract <= uint8_t(OP::CALLER::OPERATIONS)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
                contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::LEDGER::TIMESTAMP);
                contract <= uint8_t(OP::UNGROUP);
            }
        }
        else
        {
            /* Add a conditional statement to check that the sender is not the receiver. */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::NOTEQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::UNGROUP);

            /* If the contract is a debit to a tokenized asset then add an additional clause to bypass the expiration if the
            recipient is from the sending sig chain  */
            if(fTokenizedDebit)
            {
                contract <= uint8_t(OP::OR);

                contract <= uint8_t(OP::GROUP);
                contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
                contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
                contract <= uint8_t(OP::CALLER::OPERATIONS)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof
                contract <= uint8_t(OP::UNGROUP);
            }
        }

        fAdded = true;

        return fAdded;

    }

    /* Determines if a contract has an expiration condition.  The method searches the nBytes of the conditions stream to see if
    *  if the pattern for an expiration condition exists somewhere in the conditions */
    bool HasExpires(const TAO::Operation::Contract& contract)
    {
        /* Create a contract with a dummy expiration condition that we can use for comparison */
        TAO::Operation::Contract expiration;
        AddExpires( encoding::json(), 0, expiration, false);

        /* Check to see if the contract contains the expiration condition */
        return HasCondition(contract, expiration);

    }


    /* Creates a void contract for the specified transaction  */
    bool AddVoid(const TAO::Operation::Contract& contract, const uint32_t nContract, TAO::Operation::Contract &voidContract)
    {
        /* The return flag indicating the contract was voided */
        bool fVoided = false;

        /* Get the transaction hash */
        const uint512_t hashTx = contract.Hash();

        /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
        contract.Reset();

        /* Get the operation nByte. */
        uint8_t nType = 0;
        contract >> nType;

        /* Check for conditional OP */
        if(nType == TAO::Operation::OP::CONDITION)
            contract >> nType;

        /* Ensure that it is a debit or transfer */
        if(nType != TAO::Operation::OP::DEBIT && nType != TAO::Operation::OP::TRANSFER)
            return false;

        /* Process crediting a debit */
        if(nType == TAO::Operation::OP::DEBIT)
        {
            /* Get the hashFrom from the debit transaction. This is the account we are going to return the credit to*/
            TAO::Register::Address hashFrom;
            contract >> hashFrom;

            /* Get the hashTo from the debit transaction. */
            TAO::Register::Address hashTo;
            contract >> hashTo;

            /* Get the amount to respond to. */
            uint64_t nAmount = 0;
            contract >> nAmount;

            /* Get the token / account object that the debit was made to. */
            TAO::Register::Object debit;
            if(!LLD::Register->ReadState(hashTo, debit))
                return false;

            /* Parse the object register. */
            if(!debit.Parse())
                throw Exception(-41, "Failed to parse object from debit transaction");

            /* Check to see whether there are any partial credits already claimed against the debit */
            uint64_t nClaimed = 0;
            if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL))
                nClaimed = 0;

            /* Check that there is something to be claimed */
            if(nClaimed == nAmount)
                throw Exception(-173, "Cannot void debit transaction as it has already been fully credited by all recipients");

            /* Reduce the amount to credit by the amount already claimed */
            nAmount -= nClaimed;

            /* Create the credit contract  */
            voidContract<< uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashFrom <<  hashFrom << nAmount;

            fVoided = true;
        }
        /* Process voiding a transfer */
        else if(nType == TAO::Operation::OP::TRANSFER)
        {
            /* Get the address of the asset being transferred from the transaction. */
            TAO::Register::Address hashAddress;
            contract >> hashAddress;

            /* Get the genesis hash (recipient) of the transfer*/
            uint256_t hashGenesis = 0;
            contract >> hashGenesis;

            /* Read the force transfer flag */
            uint8_t nForceFlag = 0;
            contract >> nForceFlag;

            /* Ensure this wasn't a forced transfer (which requires no Claim) */
            if(nForceFlag == TAO::Operation::TRANSFER::FORCE)
                return false;

            /* Create the claim contract  */
            voidContract << (uint8_t)TAO::Operation::OP::CLAIM << hashTx << uint32_t(nContract) << hashAddress;

            fVoided = true;
        }

        return fVoided;
    }
}/* End TAO namespace */
