/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Util/include/json.h>
#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /** AddExpires
    *
    *  Checks the params for the existence of the "expires" field.
    *
    *  @param[in] params The parameters passed in the request
    *  @param[in] hashCaller The genesis hash of the API caller
    *  @param[in] contract The contract to add the conditions to
    *  @param[in] fTokenizedDebit flag indicating if the contract is a debit to a tokenized asset
    *
    **/
    void AddExpires(const encoding::json& params, const uint256_t& hashCaller, TAO::Operation::Contract& contract, bool fTokenizedDebit);


    /** AddVoid
     *
     *  Creates a void contract for the specified transaction
     *
     *  @param[in] contract The contract to void
     *  @param[in] the ID of the contract in the transaction
     *  @param[out] voidContract The void contract to be created
     *
     *  @return True if a void contract was created.
     *
     **/
    bool AddVoid(const TAO::Operation::Contract& contract, const uint32_t nContract, TAO::Operation::Contract &voidContract);

}/* End TAO namespace */
