/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <LLC/hash/SK.h>

#include <Util/include/base58.h>

#include <Legacy/include/signature.h>
#include <Legacy/include/evaluate.h>

#include <Legacy/types/enum.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/script.h>

#include <string>
#include <vector>

namespace Legacy
{

    /* Signs for a single signature transaction. */
    bool Sign1(const NexusAddress& address, const CKeyStore& keystore, uint256_t hash, int32_t nHashType, CScript& scriptSigRet)
    {
        LLC::ECKey key;
        if (!keystore.GetKey(address, key))
            return false;

        std::vector<uint8_t> vchSig;
        if (!key.Sign(hash, vchSig, 256))
            return false;

        vchSig.push_back((uint8_t)nHashType);
        scriptSigRet << vchSig;

        return true;
    }


    /* Sign for a multi-signature transaction. */
    bool SignN(const std::vector< std::vector<uint8_t> >& multisigdata, const CKeyStore& keystore, uint256_t hash, int32_t nHashType, CScript& scriptSigRet)
    {
        int32_t nSigned = 0;
        int32_t nRequired = multisigdata.front()[0];
        for (std::vector<std::vector<uint8_t> >::const_iterator it = multisigdata.begin() + 1; it != multisigdata.begin() + multisigdata.size() - 1; it++)
        {
            const std::vector<uint8_t>& pubkey = *it;
            NexusAddress address;
            address.SetPubKey(pubkey);
            if (Sign1(address, keystore, hash, nHashType, scriptSigRet))
            {
                nSigned ++;
                if (nSigned == nRequired) break;
            }
        }

        return nSigned == nRequired;
    }


    /* Returns a hash that is used to sign inputs or verify the signature is a valid signature of this hash. */
    uint256_t SignatureHash(CScript scriptCode, const Transaction& txTo, uint32_t nIn, int32_t nHashType)
    {
        if (nIn >= txTo.vin.size())
        {
            debug::error("SignatureHash() : nIn=", nIn, " out of range");
            return 1;
        }

        Transaction txTmp(txTo);

        // In case concatenating two scripts ends up with two codeseparators,
        // or an extra one at the end, this prevents all those possible incompatibilities.
        scriptCode.FindAndDelete(CScript(OP_CODESEPARATOR));

        // Blank out other inputs' signatures
        for (uint32_t i = 0; i < txTmp.vin.size(); i++)
            txTmp.vin[i].scriptSig = CScript();

        txTmp.vin[nIn].scriptSig = scriptCode;

        // Blank out some of the outputs
        if ((nHashType & 0x1f) == SIGHASH_NONE)
        {
            // Wildcard payee
            txTmp.vout.clear();

            // Let the others update at will
            for (uint32_t i = 0; i < txTmp.vin.size(); i++)
                if (i != nIn)
                    txTmp.vin[i].nSequence = 0;
        }
        else if ((nHashType & 0x1f) == SIGHASH_SINGLE)
        {
            // Only lockin the txout payee at same index as txin
            uint32_t nOut = nIn;
            if (nOut >= txTmp.vout.size())
            {
                debug::error("SignatureHash() : nOut=", nOut, " out of range");
                return 1;
            }
            txTmp.vout.resize(nOut+1);
            for (uint32_t i = 0; i < nOut; i++)
                txTmp.vout[i].SetNull();

            // Let the others update at will
            for (uint32_t i = 0; i < txTmp.vin.size(); i++)
                if (i != nIn)
                    txTmp.vin[i].nSequence = 0;
        }

        // Blank out other inputs completely, not recommended for open transactions
        if (nHashType & SIGHASH_ANYONECANPAY)
        {
            txTmp.vin[0] = txTmp.vin[nIn];
            txTmp.vin.resize(1);
        }

        // Serialize and hash
        DataStream ss(SER_GETHASH, 0);
        ss.reserve(10000);
        ss << txTmp << nHashType;

        return LLC::SK256(ss.begin(), ss.end());
    }


    /* Checks that the signature supplied is a valid one. */
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

        LLC::ECKey key;
        if (!key.SetPubKey(vchPubKey))
            return false;
        if (!key.Verify(sighash, vchSig, 256))
            return false;

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

}
