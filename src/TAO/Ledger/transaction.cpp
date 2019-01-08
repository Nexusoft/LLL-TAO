/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/include/key.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <Util/templates/serialize.h>
#include <Util/include/hex.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Operation/include/enum.h>

namespace TAO::Ledger
{
    /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
    bool Transaction::IsValid() const
    {
        /* Read the previous transaction from disk. */
        if(!IsGenesis())
        {
            TAO::Ledger::Transaction tx;
            if(!LLD::legDB->ReadTx(hashPrevTx, tx))
                return debug::error(FUNCTION "failed to read previous transaction");

            /* Check the previous next hash that is being claimed. */
            if(tx.hashNext != PrevHash())
                return debug::error(FUNCTION "next hash mismatch with previous transaction");

            /* Check the previous sequence number. */
            if(tx.nSequence + 1 != nSequence)
                return debug::error(FUNCTION "previous sequence %u not sequential %u", tx.nSequence, nSequence);

            /* Check the previous genesis. */
            if(tx.hashGenesis != hashGenesis)
                return debug::error(FUNCTION "previous genesis %s mismatch %s", tx.hashGenesis.ToString().substr(0, 20).c_str(), hashGenesis.ToString().substr(0, 20).c_str());
        }

        /* Checks for coinbase. */
        if(IsCoinbase())
        {
            /* Check the coinbase size. */
            //if(ssOperation.size() != 41)
            //    return debug::error(FUNCTION "operation data too large for coinbase %u", ssOperation.size());
        }

        /* Check the timestamp. */
        if(nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
            return debug::error(FUNCTION "transaction timestamp too far in the future %u", nTimestamp);

        /* Check the size constraints of the ledger data. */
        if(ssOperation.size() > 1024) //TODO: implement a constant max size
            return debug::error(FUNCTION "ledger data outside of maximum size constraints");

        /* Check the more expensive ECDSA verification. */
        LLC::ECKey ecPub(NID_brainpoolP512t1, 64);
        ecPub.SetPubKey(vchPubKey);
        if(!ecPub.Verify(GetHash().GetBytes(), vchSig))
            return debug::error(FUNCTION "invalid transaction signature");

        return true;
    }


    /* Determines if the transaction is a coinbase transaction. */
    bool Transaction::IsCoinbase() const
    {
        if(ssOperation.size() == 0)
            return false;

        return ssOperation.get(0) == TAO::Operation::OP::COINBASE;
    }


    /* Determines if the transaction is a coinstake transaction. */
    bool Transaction::IsTrust() const
    {
        if(ssOperation.size() == 0)
            return false;

        return ssOperation.get(0) == TAO::Operation::OP::TRUST;
    }


    /* Determines if the transaction is a genesis transaction */
    bool Transaction::IsGenesis() const
    {
        return (nSequence == 0 && hashPrevTx == 0);
    }


    /* Gets the hash of the transaction object. */
    uint512_t Transaction::GetHash() const
    {
        DataStream ss(SER_GETHASH, nVersion);
        ss << *this;

        return LLC::SK512(ss.begin(), ss.end());
    }


    /* Sets the Next Hash from the key */
    void Transaction::NextHash(uint512_t hashSecret)
    {
        /* Get the secret from new key. */
        std::vector<uint8_t> vBytes = hashSecret.GetBytes();
        LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

        /* Generate the EC Key. */
        LLC::ECKey key(NID_brainpoolP512t1, 64);
        if(!key.SetSecret(vchSecret, true))
            return;

        /* Calculate the next hash. */
        hashNext = LLC::SK256(key.GetPubKey());
    }


    /* Gets the nextHash for the previous transaction */
    uint256_t Transaction::PrevHash() const
    {
        return LLC::SK256(vchPubKey);
    }


    /* Signs the transaction with the private key and sets the public key */
     bool Transaction::Sign(uint512_t hashSecret)
     {
        /* Get the secret from new key. */
        std::vector<uint8_t> vBytes = hashSecret.GetBytes();
        LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

        /* Generate the EC Key. */
        LLC::ECKey key(NID_brainpoolP512t1, 64);
        if(!key.SetSecret(vchSecret, true))
            return false;

        /* Set the public key. */
        vchPubKey = key.GetPubKey();

        /* Calculate the has of this transaction. */
        uint512_t hashThis = GetHash();

        /* Sign the hash. */
        return key.Sign(hashThis.GetBytes(), vchSig);
     }

     /* Debug output - use ANSI colors. TODO: turn ansi colors on or off with a commandline flag */
     void Transaction::print() const
     {
         debug::log(0, "%s("
         VALUE("nVersion") " = %u, "
         VALUE("nSequence") " = %u, "
         VALUE("nTimestamp") " = %" PRIu64 ", "
         VALUE("hashNext") " = %s, "
         VALUE("hashPrevTx") " = %s, "
         VALUE("hashGenesis") " = %s, "
         VALUE("pub") " = %s, "
         VALUE("sig") " = %s, "
         VALUE("hash") " = %s, "
         VALUE("register.size()") " = %u,"
         VALUE("operation.size()") " = %u)",

         IsGenesis() ? "Genesis" : "Tritium",
         nVersion,
         nSequence,
         nTimestamp,
         hashNext.ToString().substr(0, 20).c_str(),
         hashPrevTx.ToString().substr(0, 20).c_str(),
         hashGenesis.ToString().substr(0, 20).c_str(),
         HexStr(vchPubKey).substr(0, 20).c_str(),
         HexStr(vchSig).substr(0, 20).c_str(),
         GetHash().ToString().substr(0, 20).c_str(),
         ssRegister.size(),
         ssOperation.size() );
     }
}
