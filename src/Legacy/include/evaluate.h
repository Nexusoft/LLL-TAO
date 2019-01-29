/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_INCLUDE_EVALUATE_H
#define NEXUS_LEGACY_INCLUDE_EVALUATE_H

#include <LLC/types/bignum.h>
#include <Util/include/base58.h>

#include <Legacy/wallet/basickeystore.h>
#include <Legacy/types/script.h>

#include <string>
#include <vector>

namespace Legacy
{

    /** Eval Script
     *
     *  Evaluate a script to true or false based on operation codes.
     *
     *  @param[in] stack The stack byte code to execute.
     *  @param[in] script The script object to run.
     *  @param[in] txTo The transaction this is executing for.
     *  @param[in] nIn The input in.
     *  @param[in] nHashType The hash type enumeration.
     *
     *  @return true if the script evaluates to true.
     *
     **/
    bool EvalScript(std::vector< std::vector<uint8_t> >& stack, const CScript& script, const Transaction& txTo, uint32_t nIn, int32_t nHashType);


    /** Solver
     *
     *  Extract data from a script object.
     *
     *  @param[in] scriptPubKey The script object to solve for.
     *  @param[in] typeRet The transaction type being processed.
     *  @param[out] vSolutionsRet The solutions returned.
     *
     *  @return true if the script was solved successfully.
     *
     **/
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<std::vector<uint8_t> >& vSolutionsRet);


    /** Solver
     *
     *  Sign scriptPubKey with private keys, given transaction hash and hash type.
     *
     *  @param[in] keystore The keystore object used to sign.
     *  @param[in] scriptPubKey The script to sign.
     *  @param[in] hash The hash to sign.
     *  @param[out] scriptSigRet The script signature to return.
     *  @param[out] whichTypeRet The return type.
     *
     *  @return returns true if this was solved.
     *
     **/
    bool Solver(const CKeyStore& keystore, const CScript& scriptPubKey, uint256_t hash, int32_t nHashType, CScript& scriptSigRet, TransactionType& whichTypeRet);


    /** Script Sig Args Expected
     *
     *  Used in standard inputs function check in transaction. potential to remove
     *
     *  @param[in] t Transaction type in.
     *  @param[out] vSolutions The solutions to pass back.
     *
     *  @return the total expected arguments.
     *
     **/
    int ScriptSigArgsExpected(TransactionType t, const std::vector<std::vector<uint8_t> >& vSolutions);


    /** Is Standard
     *
     *  Detects if a script object is of a standard type.
     *
     *  @param[in] scriptPubKey The script object to check.
     *
     *  @return true if the output is standard.
     *
     **/
    bool IsStandard(const CScript& scriptPubKey);


    /** Have Keys
     *
     *  Determines if given list of public keys exist in the keystore object.
     *
     *  @param[in] pubKeys The list of public keys to check for.
     *  @param[in] keystore The keystore object to check for keys in.
     *
     *  @return Return the total number of keys that have been found.
     *
     **/
    uint32_t HaveKeys(const std::vector< std::vector<uint8_t> >& pubkeys, const CKeyStore& keystore);


    /** Is Mine
     *
     *  Checks an output to your keystore to detect if you have a key
     *  that is involed in the output or transaction.
     *
     *  @param[in] keystore The keystore to check against
     *  @param[in] scriptPubKey The script object to check
     *
     *  @return true if script object involes a key in keystore.
     *
     **/
    bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);



    /** Extract Address
     *
     *  Extract a Nexus Address from a public key script.
     *
     *  @param[in] scriptPubKey The script object to extract address from.
     *  @param[out] addressRet The address object to return
     *
     *  @return true if address was extracted successfully.
     *
     **/
    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet);


    /** Extract Addresses
     *
     *  Extract a list of Nexus Addresses from a public key script.
     *
     *  @param[in] scriptPubKey The script object to extract address from.
     *  @param[out] typeRet The transaction type to return
     *  @param[out] addressRet The list of addresses to return
     *  @param[out] nRequiredRet requirements return value
     *
     *  @return true if addresses were extracted successfully.
     *
     **/
    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<NexusAddress>& addressRet, int32_t& nRequiredRet);


    /** Verify Script
     *
     *  Verify a script is a valid one
     *
     *  @param[in] scriptSig The script to verify
     *  @param[in] scriptPubKey The script to verify against.
     *  @param[in] txTo The destination transaciton being signed.
     *  @param[in] nIn The output to verify signature for.
     *  @param[in] nHashType The hash type for signature.
     *
     *  @return true if the script was verified valid.
     *
     **/
    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Transaction& txTo, uint32_t nIn, int32_t nHashType);

}

#endif
