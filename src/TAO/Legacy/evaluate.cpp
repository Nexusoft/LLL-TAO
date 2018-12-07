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

    //stack preprocessors
    #define stacktop(i)  (stack.at(stack.size()+(i)))
    #define altstacktop(i)  (altstack.at(altstack.size()+(i)))


    /**
     *
     * Script is a stack machine (like Forth) that evaluates a predicate
     * returning a bool indicating valid or not.  There are no loops.
     *
     **/
    static inline void popstack(vector<std::vector<unsigned char> >& stack)
    {
        if (stack.empty())
            throw runtime_error("popstack() : stack empty");
        stack.pop_back();
    }


    /* Evaluate a script to true or false based on operation codes. */
    bool EvalScript(std::vector<std::vector<uint8_t> >& stack, const CScript& script, const Core::CTransaction& txTo, uint32_t nIn, int32_t nHashType);
    {
        
    }


    /* Extract data from a script object. */
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, std::vector< std::vector<uint8_t> >& vSolutionsRet)
    {
        // Templates
        static std::map<TransactionType, CScript> mTemplates;
        if (mTemplates.empty())
        {
            // Standard tx, sender provides pubkey, receiver adds signature
            mTemplates.insert(make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

            // Nexus address tx, sender provides hash of pubkey, receiver provides signature and pubkey
            mTemplates.insert(make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH256 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

            // Sender provides N pubkeys, receivers provides M signatures
            mTemplates.insert(make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));
        }

        // Shortcut for pay-to-script-hash, which are more constrained than the other types:
        // it is always OP_HASH256 20 [20 byte hash] OP_EQUAL
        if (scriptPubKey.IsPayToScriptHash())
        {
            typeRet = TX_SCRIPTHASH;
            std::vector<uint8_t> hashBytes(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22);
            vSolutionsRet.push_back(hashBytes);
            return true;
        }

        // Scan templates
        const CScript& script1 = scriptPubKey;
        for(auto tplate : mTemplates)
        {
            const CScript& script2 = tplate.second;
            vSolutionsRet.clear();

            opcodetype opcode1, opcode2;
            vector<uint8_t> vch1, vch2;

            // Compare
            CScript::const_iterator pc1 = script1.begin();
            CScript::const_iterator pc2 = script2.begin();
            while(true)
            {
                if (pc1 == script1.end() && pc2 == script2.end())
                {
                    // Found a match
                    typeRet = tplate.first;
                    if (typeRet == TX_MULTISIG)
                    {
                        // Additional checks for TX_MULTISIG:
                        uint8_t m = vSolutionsRet.front()[0];
                        uint8_t n = vSolutionsRet.back()[0];
                        if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                            return false;
                    }
                    return true;
                }
                if (!script1.GetOp(pc1, opcode1, vch1))
                    break;

                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;

                // Template matching opcodes:
                if (opcode2 == OP_PUBKEYS)
                {
                    while (vch1.size() >= 33 && vch1.size() <= 120)
                    {
                        vSolutionsRet.push_back(vch1);
                        if (!script1.GetOp(pc1, opcode1, vch1))
                            break;
                    }
                    if (!script2.GetOp(pc2, opcode2, vch2))
                        break;
                    // Normal situation is to fall through
                    // to other if/else statments
                }

                if (opcode2 == OP_PUBKEY)
                {
                    if (vch1.size() < 33 || vch1.size() > 120)
                        break;
                    vSolutionsRet.push_back(vch1);
                }

                else if (opcode2 == OP_PUBKEYHASH)
                {
                    if (vch1.size() != sizeof(uint256))
                        break;
                    vSolutionsRet.push_back(vch1);
                }

                else if (opcode2 == OP_SMALLINTEGER)
                {   // Single-byte small integer pushed onto vSolutions
                    if (opcode1 == OP_0 ||
                        (opcode1 >= OP_1 && opcode1 <= OP_16))
                    {
                        char n = (int8_t)scriptPubKey.DecodeOP_N(opcode1);
                        vSolutionsRet.push_back(std::vector<uint8_t>(1, n));
                    }
                    else
                        break;
                }
                else if (opcode1 != opcode2 || vch1 != vch2)
                {
                    // Others must match exactly
                    break;
                }
            }
        }

        vSolutionsRet.clear();
        typeRet = TX_NONSTANDARD;
        return false;
    }


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
        std::vector< std::vector<uint8_t> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        if (whichType == TX_MULTISIG)
        {
            uint8_t m = vSolutions.front()[0];
            uint8_t n = vSolutions.back()[0];

            // Support up to x-of-3 multisig txns as standard
            if (n < 1 || n > 3)
                return false;
            if (m < 1 || m > n)
                return false;
        }

        return whichType != TX_NONSTANDARD;
    }


    /* Determines if given list of public keys exist in the keystore object. */
    uint32_t HaveKeys(const std::vector< std::vector<uint8_t> >& pubkeys, const CKeyStore& keystore)
    {
        uint32_t nResult = 0;
        for(auto pubkey : pubkeys)
        {
            NexusAddress address;
            address.SetPubKey(pubkey);
            if (keystore.HaveKey(address))
                nResult ++;
        }

        return nResult;
    }


    /* Checks an output to your keystore to detect if you have a key that is involed in the output or transaction. */
    bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey)
    {
        vector<std::vector<uint8_t> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        NexusAddress address;
        switch (whichType)
        {
            case TX_NONSTANDARD:
                return false;

            case TX_PUBKEY:
                address.SetPubKey(vSolutions[0]);
                return keystore.HaveKey(address);

            case TX_PUBKEYHASH:
                address.SetHash256(uint256(vSolutions[0]));
                return keystore.HaveKey(address);

            case TX_SCRIPTHASH:
            {
                CScript subscript;
                if (!keystore.GetCScript(uint256(vSolutions[0]), subscript))
                    return false;

                return IsMine(keystore, subscript);
            }

            case TX_MULTISIG:
            {
                // Only consider transactions "mine" if we own ALL the
                // keys involved. multi-signature transactions that are
                // partially owned (somebody else has a key that can spend
                // them) enable spend-out-from-under-you attacks, especially
                // in shared-wallet situations.
                std::vector<std::vector<uint8_t> > keys(vSolutions.begin()+1, vSolutions.begin() + vSolutions.size()-1);
                return HaveKeys(keys, keystore) == keys.size();
            }
        }
        return false;
    }



    /* Extract a Nexus Address from a public key script. */
    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet)
    {
        std::vector< std::vector<uint8_t> > vSolutions;
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
    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<NexusAddress>& addressRet, int32_t& nRequiredRet)
    {
        addressRet.clear();
        typeRet = TX_NONSTANDARD;
        std::vector< std::vector<uint32_t> > vSolutions;
        if (!Solver(scriptPubKey, typeRet, vSolutions))
            return false;

        if (typeRet == TX_MULTISIG)
        {
            nRequiredRet = vSolutions.front()[0];
            for (uint32_t i = 1; i < vSolutions.size() - 1; i++)
            {
                NexusAddress address;
                address.SetPubKey(vSolutions[i]);
                addressRet.push_back(address);
            }
        }
        else
        {
            nRequiredRet = 1;
            NexusAddress address;
            if (typeRet == TX_PUBKEYHASH)
                address.SetHash256(uint256(vSolutions.front()));
            else if (typeRet == TX_SCRIPTHASH)
                address.SetScriptHash256(uint256(vSolutions.front()));
            else if (typeRet == TX_PUBKEY)
                address.SetPubKey(vSolutions.front());

            addressRet.push_back(address);
        }

        return true;
    }


    /* Verify a script is a valid */
    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Transaction& txTo, uint32_t nIn, int32_t nHashType)
    {
        std::vector< std::vector<uint8_t> > stack, stackCopy;
        if (!EvalScript(stack, scriptSig, txTo, nIn, nHashType))
            return false;

        stackCopy = stack;
        if (!EvalScript(stack, scriptPubKey, txTo, nIn, nHashType))
            return false;

        if (stack.empty())
            return false;

        if (CastToBool(stack.back()) == false)
            return false;

        // Additional validation for spend-to-script-hash transactions:
        if (scriptPubKey.IsPayToScriptHash())
        {
            if (!scriptSig.IsPushOnly()) // scriptSig must be literals-only
                return false;

            const std::vector<uint8_t>& pubKeySerialized = stackCopy.back();
            CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
            popstack(stackCopy);

            if (!EvalScript(stackCopy, pubKey2, txTo, nIn, nHashType))
                return false;

            if (stackCopy.empty())
                return false;

            return CastToBool(stackCopy.back());
        }

        return true;
    }

}
