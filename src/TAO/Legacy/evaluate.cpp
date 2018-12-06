/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <Util/include/base58.h>

#include <TAO/Legacy/types/enum.h>
#include <TAO/Legacy/types/script.h>

#include <string>
#include <vector>

namespace Legacy
{

    /* Evaluate a script to true or false based on operation codes. */
    bool EvalScript(std::vector<std::vector<uint8_t> >& stack, const CScript& script, const Core::CTransaction& txTo, uint32_t nIn, int nHashType);


    /* Extract data from a script object. */
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<std::vector<uint8_t> >& vSolutionsRet);


    /* Used in standard inputs function check in transaction. potential to remove */
    int ScriptSigArgsExpected(TransactionType t, const std::vector<std::vector<uint8_t> >& vSolutions);
    {
        switch (t)
        {
            case TX_NONSTANDARD:
                return -1;
            case TX_PUBKEY:
                return 1;
            case TX_PUBKEYHASH:
                return 2;
            case TX_MULTISIG:
                if (vSolutions.size() < 1 || vSolutions[0].size() < 1)
                    return -1;
                return vSolutions[0][0] + 1;
            case TX_SCRIPTHASH:
                return 1; // doesn't include args needed by the script
        }
        return -1;
    }


    /* Detects if a script object is of a standard type. */
    bool IsStandard(const CScript& scriptPubKey)
    {
        vector<std::vector<unsigned char> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        if (whichType == TX_MULTISIG)
        {
            unsigned char m = vSolutions.front()[0];
            unsigned char n = vSolutions.back()[0];
            // Support up to x-of-3 multisig txns as standard
            if (n < 1 || n > 3)
                return false;
            if (m < 1 || m > n)
                return false;
        }

        return whichType != TX_NONSTANDARD;
    }


    /* Checks an output to your keystore to detect if you have a key that is involed in the output or transaction. */
    bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);



    /* Extract a Nexus Address from a public key script. */
    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet)
    {
        vector<std::vector<unsigned char> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        if (whichType == TX_PUBKEY)
        {
            addressRet.SetPubKey(vSolutions[0]);
            return true;
        }
        else if (whichType == TX_PUBKEYHASH)
        {
            addressRet.SetHash256(uint256(vSolutions[0]));
            return true;
        }
        else if (whichType == TX_SCRIPTHASH)
        {
            addressRet.SetScriptHash256(uint256(vSolutions[0]));
            return true;
        }
        // Multisig txns have more than one address...
        return false;
    }


    /* Extract a list of Nexus Addresses from a public key script. */
    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<NexusAddress>& addressRet, int& nRequiredRet);


    /* Sign an input to a transaction from keystore */
    bool SignSignature(const CKeyStore& keystore, const Transaction& txFrom, Transaction& txTo, uint32_t nIn, int nHashType=SIGHASH_ALL);


    /* Verify a signature was valid */
    bool VerifySignature(const Transaction& txFrom, const Transaction& txTo, uint32_t nIn, int nHashType);


    /* Verify a script is a valid */
    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Transaction& txTo, uint32_t nIn, int nHashType);

}
