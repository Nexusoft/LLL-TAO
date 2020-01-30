/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <Util/include/string.h>

using namespace TAO::Operation;

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Determines if a contract contains a specific condition.  The method searches the bytes of the conditions stream to see
        *  if the pattern for condition exists somewhere within it */
        bool HasCondition(const TAO::Operation::Contract& contract, const TAO::Operation::Contract& condition)
        {
            bool fHasCondition = false;

            /* Reset the contract conditions stream */
            contract.Reset(TAO::Operation::Contract::CONDITIONS);

            /* search the contract to find the first instance of the same byte */
            while(!contract.End(TAO::Operation::Contract::CONDITIONS) && !fHasCondition)
            {
                /* Reset the conditions stream on the expiration as we are starting a new comparison */
                condition.Reset(TAO::Operation::Contract::CONDITIONS);

                /* Get byte from the expiration condition */
                uint8_t byte;
                condition >= byte;
                
                /* Get byte to compare from the comparison contract */
                uint8_t compare;
                contract >= compare;

                /* Cache the stream position so we can reset to here if a match is not found */
                uint64_t nPos = contract.Position(TAO::Operation::Contract::CONDITIONS);

                /* If the byte matches then start the rest of the comparison */
                if(compare == byte)
                { 
                    while(!condition.End(TAO::Operation::Contract::CONDITIONS) && !contract.End(TAO::Operation::Contract::CONDITIONS))
                    {
                        condition >= byte;
                        contract >= compare;

                        /* Check the bytes match.  If not then we can break out of this comparison */
                        if(byte != compare)
                            break;

                        /* If this is an OP::TYPES then we need to skip both stream ahead by the number of bytes, as this is caller
                           specific data and won't match - we are only comparing the operations and logic, not the data */
                        switch(byte)
                        {
                            case OP::TYPES::UINT8_T :
                            {
                                condition.Seek(1, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(1, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::UINT16_T :
                            {
                                condition.Seek(2, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(2, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::UINT32_T :
                            {
                                condition.Seek(4, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(4, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::UINT64_T :
                            {
                                condition.Seek(8, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(8, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::UINT256_T :
                            {
                                condition.Seek(32, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(32, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::UINT512_T :
                            {
                                condition.Seek(64, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(64, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::UINT1024_T :
                            {
                                condition.Seek(128, TAO::Operation::Contract::CONDITIONS);
                                contract.Seek(128, TAO::Operation::Contract::CONDITIONS);
                                break;
                            }
                            case OP::TYPES::BYTES :
                            {
                                std::vector<uint8_t> bytes;
                                condition >= bytes;
                                contract >= bytes;
                                break;
                            }
                            case OP::TYPES::STRING :
                            {
                                std::string string;
                                condition >= string;
                                contract >= string;
                                break;
                            }
                            case OP::SUBDATA :
                            {
                                /* OP::SUBDATA has two uint16_t's for the start pos and length, so skip 4 bytes */
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
        bool AddExpires(const json::json& params, const uint256_t& hashCaller, TAO::Operation::Contract& contract, bool fTokenizedDebit)
        {
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
                    throw APIException(-168, "Invalid expiration time");

                /* Convert the expiration time to uint64 */
                nExpires = stoull(strExpires);
            }
            else
            {
                /* Default Expiration of 1 day (86400 seconds) */
                nExpires = 86400;
            }


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
            contract <= uint8_t(OP::LESSTHAN) <= uint8_t(OP::LEDGER::TIMESTAMP);
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

            fAdded = true;

            return fAdded;

        }

        /* Determines if a contract has an expiration condition.  The method searches the bytes of the conditions stream to see if
        *  if the pattern for an expiration condition exists somewhere in the conditions */
        bool HasExpires(const TAO::Operation::Contract& contract)
        {
            bool fHasExpires = false;

            /* Create a contract with a dummy expiration condition that we can use for comparison */
            TAO::Operation::Contract expiration;
            AddExpires( json::json(), 0, expiration, false);

            /* Check to see if the contract contains the expiration condition */
            return HasCondition(contract, expiration);

        }

    } /* End API namespace */

}/* End TAO namespace */
