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

namespace TAO::Ledger
{
    /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
    bool Transaction::IsValid() const
    {
        /* Read the previous transaction from disk. */
        TAO::Ledger::Transaction tx;
        if(!LLD::legDB->ReadTx(hashPrevTx, tx))
            return debug::error(FUNCTION "failed to read previous transaction", __PRETTY_FUNCTION__);

        /* Check the previous next hash that is being claimed. */
        if(tx.hashNext != PrevHash())
            return debug::error(FUNCTION "next hash mismatch with previous transaction", __PRETTY_FUNCTION__);

        /* Check the previous sequence number. */
        if(tx.nSequence + 1 != nSequence)
            return debug::error(FUNCTION "previous sequence %u not sequential %u", __PRETTY_FUNCTION__, tx.nSequence, nSequence);

        /* Check the timestamp. */
        if(tx.nTimestamp > UnifiedTimestamp() + MAX_UNIFIED_DRIFT)
            return debug::error(FUNCTION "transaction timestamp too far in the future %u", __PRETTY_FUNCTION__, tx.nTimestamp);

        /* Check the previous genesis. */
        if(tx.hashGenesis != hashGenesis)
            return debug::error(FUNCTION "previous genesis %s mismatch %s", __PRETTY_FUNCTION__, tx.hashGenesis.ToString().substr(0, 20).c_str(), hashGenesis.ToString().substr(0, 20).c_str());

        /* Check the size constraints of the ledger data. */
        if(vchLedgerData.size() > 1024) //TODO: implement a constant max size
            return debug::error(FUNCTION "ledger data outside of maximum size constraints", __PRETTY_FUNCTION__);

        /* Check the more expensive ECDSA verification. */
        LLC::ECKey ecPub(NID_brainpoolP512t1, 64);
        ecPub.SetPubKey(vchPubKey);
        if(!ecPub.Verify(GetHash().GetBytes(), vchSig))
            return debug::error(FUNCTION "invalid transaction signature", __PRETTY_FUNCTION__);
    }


    /* Determines if the transaction is a genesis transaction */
    bool Transaction::IsGenesis() const
    {
        return (nSequence == 0 && hashGenesis == 0);
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
        DataStream ssData(SER_NETWORK, nVersion);
        ssData << hashSecret;

        LLC::CSecret vchSecret(ssData.begin(), ssData.end());
        LLC::ECKey key(NID_brainpoolP512t1, 64);
        if(!key.SetSecret(vchSecret, true))
            return;

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
        DataStream ssData(SER_NETWORK, nVersion);
        ssData << hashSecret;

        LLC::CSecret vchSecret(ssData.begin(), ssData.end());

        LLC::ECKey key(NID_brainpoolP512t1, 64);
        if(!key.SetSecret(vchSecret, true))
            return false;

        vchPubKey = key.GetPubKey();
        uint512_t hashTx = GetHash();
        return key.Sign(hashTx.GetBytes(), vchSig);
     }

     /* Debug output - use ANSI colors. TODO: turn ansi colors on or off with a commandline flag */
     void Transaction::print() const
     {
         debug::log(0, "%s(" ANSI_COLOR_BRIGHT_WHITE "nVersion" ANSI_COLOR_RESET " = %u, " ANSI_COLOR_BRIGHT_WHITE "nTimestamp" ANSI_COLOR_RESET " = %" PRIu64 ", " ANSI_COLOR_BRIGHT_WHITE "hashNext" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "hashPrevTx" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "hashGenesis" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "pub" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "sig" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "hash" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "ledger" ANSI_COLOR_RESET " = %s)",
         IsGenesis() ? "Genesis" : "Tritium", nVersion, nTimestamp, hashNext.ToString().c_str(), hashPrevTx.ToString().c_str(), hashGenesis.ToString().c_str(), HexStr(vchPubKey).c_str(), HexStr(vchSig).c_str(), GetHash().ToString().c_str(), HexStr(vchLedgerData.begin(), vchLedgerData.end()).c_str());
     }
}
