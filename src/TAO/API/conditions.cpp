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
            contract <= uint8_t(OP::THIS::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
            contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);
            contract <= uint8_t(OP::UNGROUP);

            contract <= uint8_t(OP::OR);

            /* Add condition to prevent the sender from reversing the transaction until after the expiration time */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::AND);
            contract <= uint8_t(OP::THIS::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
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
                contract <= uint8_t(OP::THIS::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
                contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
                contract <= uint8_t(OP::CALLER::OPERATIONS)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::THIS::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
                contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);
                contract <= uint8_t(OP::UNGROUP);
            }

            fAdded = true;

            return fAdded;

        }


    } /* End API namespace */

}/* End TAO namespace */
