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
    bool EvalScript(std::vector<std::vector<uint8_t> >& stack, const CScript& script, const Core::CTransaction& txTo, uint32_t nIn, int32_t nHashType);


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


    /** Checks that the signature supplied is a valid one. **/
    bool CheckSig(std::vector<uint8_t> vchSig, std::vector<uint8_t> vchPubKey, CScript scriptCode, const Transaction& txTo, uint32_t nIn, int32_t nHashType)
    {
        // Hash type is one byte tacked on to the end of the signature
        if (vchSig.empty())
            return false;
        if (nHashType == 0)
            nHashType = vchSig.back();
        else if (nHashType != vchSig.back())
            return false;

        vchSig.pop_back();
        uint256_t sighash = SignatureHash(scriptCode, txTo, nIn, nHashType);

        LLC::ECKey key(NID_sect571r1, 72);
        if (!key.SetPubKey(vchPubKey))
            return false;
        if (!key.Verify(sighash, vchSig, 256))
            return false;

        return true;
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


    /* Sign an input to a transaction from keystore */
    bool SignSignature(const CKeyStore& keystore, const Transaction& txFrom, Transaction& txTo, uint32_t nIn, int32_t nHashType)
    {
        assert(nIn < txTo.vin.size());
        CTxIn& txin = txTo.vin[nIn];

        assert(txin.prevout.n < txFrom.vout.size());
        assert(txin.prevout.hash == txFrom.GetHash());
        const CTxOut& txout = txFrom.vout[txin.prevout.n];

        // Leave out the signature from the hash, since a signature can't sign itself.
        // The checksig op will also drop the signatures from its hash.
        uint256_t hash = SignatureHash(txout.scriptPubKey, txTo, nIn, nHashType);

        TransactionType whichType;
        if (!Solver(keystore, txout.scriptPubKey, hash, nHashType, txin.scriptSig, whichType))
            return false;

        if (whichType == TX_SCRIPTHASH)
        {
            // Solver returns the subscript that need to be evaluated;
            // the final scriptSig is the signatures from that
            // and then the serialized subscript:
            CScript subscript = txin.scriptSig;

            // Recompute txn hash using subscript in place of scriptPubKey:
            uint256_t hash2 = SignatureHash(subscript, txTo, nIn, nHashType);
            TransactionType subType;
            if (!Solver(keystore, subscript, hash2, nHashType, txin.scriptSig, subType))
                return false;

            if (subType == TX_SCRIPTHASH)
                return false;

            txin.scriptSig << static_cast< std::vector<uint8_t> >(subscript); // Append serialized subscript
        }

        // Test solution
        if (!VerifyScript(txin.scriptSig, txout.scriptPubKey, txTo, nIn, 0))
            return false;

        return true;
    }


    /* Verify a signature was valid */
    bool VerifySignature(const Transaction& txFrom, const Transaction& txTo, uint32_t nIn, int nHashType)
    {
        assert(nIn < txTo.vin.size());
        const CTxIn& txin = txTo.vin[nIn];
        if (txin.prevout.n >= txFrom.vout.size())
            return false;

        const CTxOut& txout = txFrom.vout[txin.prevout.n];
        if (txin.prevout.hash != txFrom.GetHash())
            return false;

        if (!VerifyScript(txin.scriptSig, txout.scriptPubKey, txTo, nIn, nHashType))
            return false;

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
