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

    /* Alias for our build function types. */
    using build_function_t = std::function<bool (const encoding::json&, const uint32_t,
                                                 const TAO::Operation::Contract&, std::vector<TAO::Operation::Contract>&)>;

    /** BuildResponse
     *
     *  Build a response object for a transaction that was built.
     *
     *  @param[in] jParams The parameters for the relevant API call.
     *  @param[in] hashRegister The register address that we are responding for.
     *  @param[in] vContracts The list of contracts this call has generated.
     *
     *  @return the formatted JSON response to return with.
     *
     **/
    encoding::json BuildResponse(const encoding::json& jParams, const TAO::Register::Address& hashRegister,
                              const std::vector<TAO::Operation::Contract>& vContracts);


    /** BuildResponse
     *
     *  Build a response object for a transaction that was built, ommitting the address.
     *
     *  @param[in] jParams The parameters for the relevant API call.
     *  @param[in] vContracts The list of contracts this call has generated.
     *
     *  @return the formatted JSON response to return with.
     *
     **/
    encoding::json BuildResponse(const encoding::json& jParams, const std::vector<TAO::Operation::Contract>& vContracts);


    /** Build And Accept
     *
     *  Builds a transaction based on a list of contracts, to be deployed as a single tx or batched.
     *
     *  @param[in] jParams The json parameters to build the transaction with.
     *  @param[in] vContracts The list of contracts to build tx for.
     *
     *  @return the txid of the transaction that was just built.
     *
     **/
    uint512_t BuildAndAccept(const encoding::json& jParams, const std::vector<TAO::Operation::Contract>& vContracts);


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


    /** BuildContracts
     *
     *  Builds contracts based on given txid and function.
     *
     *  @param[in] jParams The parameters to use for this call.
     *  @param[in] hashTx The txid to check credit against.
     *  @param[out] vContracts The contracts built based on the received contract.
     *  @param[in] xBuild The function specializing for this contract.
     *
     *  @return true if contracts were generated, false if credit has no contracts available.
     *
     **/
    bool BuildContracts(const encoding::json& jParams, const uint512_t& hashTx, std::vector<TAO::Operation::Contract> &vContracts,
                        const build_function_t& xBuild);


    /** BuildCredit
     *
     *  Builds a credit contract based on given contract and related parameters.
     *
     *  @param[in] jParams The parameters to use for this call.
     *  @param[in] nContract The contract we are building for.
     *  @param[in] rContract The debit contract to credit from.
     *  @param[out] vContracts The contracts built based on this credit.
     *
     *  @return true if contracts were generated, false if credit has no contracts available.
     *
     **/
    bool BuildCredit(const encoding::json& jParams, const uint32_t nContract,
        const TAO::Operation::Contract& rDebit, std::vector<TAO::Operation::Contract> &vContracts);


    /** BuildClaim
     *
     *  Builds a claim contract based on given contract and related parameters.
     *
     *  @param[in] jParams The parameters to use for this call.
     *  @param[in] nContract The contract we are building for.
     *  @param[in] rContract The transfer contract to claim from.
     *  @param[out] vContracts The contracts built based on this claim.
     *
     *  @return true if contracts were generated, false if credit has no contracts available.
     *
     **/
    bool BuildClaim(const encoding::json& jParams, const uint32_t nContract,
        const TAO::Operation::Contract& rTransfer, std::vector<TAO::Operation::Contract> &vContracts);


    /** BuildName
     *
     *  Builds a name contract based on given input parameters.
     *
     *  @param[in] jParams The parameters used to derive name.
     *  @param[in] hashRegister The register address to build name for.
     *  @param[out] vContracts The list of contracts to add name to.
     *
     **/
    void BuildName(const encoding::json& jParams, const uint256_t& hashRegister, std::vector<TAO::Operation::Contract> &vContracts);
}
