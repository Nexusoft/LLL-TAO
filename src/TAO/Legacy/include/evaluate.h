/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_INCLUDE_EVALUATE_H
#define NEXUS_TAO_LEGACY_INCLUDE_EVALUATE_H

#include <LLC/types/bignum.h>

#include <Util/include/base58.h>

#include <string>
#include <vector>

namespace Legacy
{

    /** Eval Script
     *
     *  Evaluate a script to true or false based on operation codes.
     *
     *  @param[in] stack The stack byte code to execute
     *  @param[in] script The script object to run
     *  @param[in] txTo The transaction this is executing for.
     *  @param[in] nIn The input in.
     *  @param[in] nHashType The hash type enumeration.
     *
     *  @return true if the script evaluates to true.
     *
     **/
    bool EvalScript(std::vector<std::vector<uint8_t> >& stack, const CScript& script, const Core::CTransaction& txTo, uint32_t nIn, int nHashType);


    /** Solver
     *
     *  Extract data from a script object.
     *
     *  @param[in] scriptPubKey The script object to solve for
     *  @param[in] typeRet The transaction type being processed
     *  @param[out] vSolutionsRet The solutions returned
     *
     *  @return true if the script was solved successfully.
     *
     **/
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<std::vector<uint8_t> >& vSolutionsRet);

    //used in standard inputs function check in transaction. potential to remove
    int ScriptSigArgsExpected(TransactionType t, const std::vector<std::vector<uint8_t> >& vSolutions);

    bool IsStandard(const CScript& scriptPubKey);

    bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);




    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet);


    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<NexusAddress>& addressRet, int& nRequiredRet);


    //important to keep
    bool SignSignature(const CKeyStore& keystore, const Transaction& txFrom, Transaction& txTo, uint32_t nIn, int nHashType=SIGHASH_ALL);

    //important to keep
    bool VerifySignature(const Transaction& txFrom, const Transaction& txTo, uint32_t nIn, int nHashType);

    //used twice. Potential to clean up TODO
    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Transaction& txTo, uint32_t nIn, int nHashType);

}

#endif
