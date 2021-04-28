/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <TAO/Register/types/address.h>

#include <Util/include/json.h>

/* Forward Declarations. */
namespace TAO::Operation { class Contract;    }
namespace TAO::Ledger    { class Transaction; }
//namespace TAO::Register { class Address;     }


/* Global TAO namespace. */
namespace TAO::API
{
    /** ExtractAddress
     *
     *  Extract an address from incoming parameters to derive from name or address field.
     *
     *  @param[in] params The parameters to find address in.
     *  @param[in] strSuffix The suffix to append to end of parameter we are extracting.
     *
     *  @return The register address if valid.
     *
     **/
    uint256_t ExtractAddress(const json::json& params, const std::string strSuffix = "");


    /** ExtractToken
     *
     *  Extract a token address from incoming parameters to derive from name or address field.
     *
     *  @param[in] params The parameters to find address in.
     *
     *  @return The register address if valid.
     *
     **/
    uint256_t ExtractToken(const json::json& params);


    /** Build And Accept
     *
     *  Builds a transaction based on a list of contracts, to be deployed as a single tx or batched.
     *
     *  @param[in] params The json parameters to build the transaction with.
     *  @param[in] vContracts The list of contracts to build tx for.
     *
     *  @return the txid of the transaction that was just built.
     *
     **/
    uint512_t BuildAndAccept(const json::json& params, const std::vector<TAO::Operation::Contract>& vContracts);


    /** AddFee
     *
     *  Calculates the required fee for the transaction and adds the OP::FEE contract to the transaction if necessary.
     *  If a specified fee account is not specified, the method will lookup the "default" NXS account and use this account
     *  to pay the fees.  An exception will be thrownIf there are insufficient funds to pay the fee. .
     *
     *  @param[in] tx The transaction to add the fee to
     *  @param[in] hashFeeAccount Optional address of account to debit the fee from
     *
     *  @return true if the fee was successfully added, otherwise false
     *
     **/
    bool AddFee(TAO::Ledger::Transaction &tx, const TAO::Register::Address& hashFeeAccount = TAO::Register::Address());
    //XXX: this is hacky to use an empty address for default parameter
}
