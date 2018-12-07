/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_INCLUDE_SIGNATURE_H
#define NEXUS_TAO_LEGACY_INCLUDE_SIGNATURE_H

#include <LLC/types/bignum.h>
#include <Util/include/base58.h>

#include <TAO/Legacy/keystore/base.h>

#include <string>
#include <vector>

namespace Legacy
{

    /** Sign 1
     *
     *  Signs for a single signature transaction.
     *
     *  @param[in] address The nexus address to sign for.
     *  @param[in] keystore The keystore that holds keys to sign from.
     *  @param[in] hash The hash to sign.
     *  @param[in] nHashType The hash type used to sign.
     *  @param[out] scriptSigRet The return script object.
     *
     *  @return true if the signature was created successfully.
     *
     **/
    bool Sign1(const NexusAddress& address, const CKeyStore& keystore, uint256_t hash, int32_t nHashType, CScript& scriptSigRet);


    /** Sign N
     *
     *  Sign for a multi-signature transaction.
     *
     *  @param[in] multisigdata The multisignature keys to sign.
     *  @param[in] keystore The keystore object that holds keys to sign.
     *  @param[in] hash The hash to sign with the multisig.
     *  @param[in] nHashType The type of hash to use.
     *  @param[out] scriptSigRet The return signature script.
     *
     *  @return true if the signatures were created successfully.
     *
     **/
    bool SignN(const std::vector< std::vector<uint8_t> >& multisigdata, const CKeyStore& keystore, uint256_t hash, int32_t nHashType, CScript& scriptSigRet);


    /** Signature Hash
     *
     *  Returns a hash that is used to sign inputs or verify the signature is a valid signature of this hash.
     *
     *  @param[in] scriptCode The input script object.
     *  @param[in] txTo The transaction that is making the spend.
     *  @param[in] nIn The input that is being signed.
     *  @param[in] nHashType The hash type that is used to generate this signature hash.
     *
     *  @return The hash for use in signing.
     *
     **/
    uint256_t SignatureHash(CScript scriptCode, const Transaction& txTo, uint32_t nIn, int32_t nHashType);


    /** Check Sig
     *
     *  Checks that the signature supplied is a valid one.
     *
     *  @param[in] vchSig The byte vector of signature data.
     *  @param[in] vchPubKey The byte vector of the public key.
     *  @param[in] scriptCode The input script object to check from.
     *  @param[in] txTo The transaction being sent to.
     *  @param[in] nIn The input being spent.
     *  @param[in] nHashType The hash type used for signature.
     *
     *  @return true if the signature is valid.
     *
     **/
    bool CheckSig(std::vector<uint8_t> vchSig, std::vector<uint8_t> vchPubKey, CScript scriptCode, const Transaction& txTo, uint32_t nIn, int32_t nHashType);


    /** Sign Signature
     *
     *  Sign an input to a transaction from keystore
     *
     *  @param[in] keystore The keystore object to pull key from
     *  @param[in] txFrom The transaction from which is being spent.
     *  @param[in] txTo The destination transaciton being signed.
     *  @param[in] nIn The output that is being signed
     *  @param[in] nHashType The hash type for signature.
     *
     *  @return true if signature was generated successfully.
     *
     **/
    bool SignSignature(const CKeyStore& keystore, const Transaction& txFrom, Transaction& txTo, uint32_t nIn, int32_t nHashType=SIGHASH_ALL);


    /** Verify Signature
     *
     *  Verify a signature was valid
     *
     *  @param[in] txFrom The transaction from which is being spent.
     *  @param[in] txTo The destination transaciton being signed.
     *  @param[in] nIn The output to verify signature for.
     *  @param[in] nHashType The hash type for signature.
     *
     *  @return true if signature was verified successfully.
     *
     **/
    bool VerifySignature(const Transaction& txFrom, const Transaction& txTo, uint32_t nIn, int32_t nHashType);

}

#endif
