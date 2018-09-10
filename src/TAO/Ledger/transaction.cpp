/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/include/key.h>

#include <LLP/include/version.h>

#include <Util/templates/serialize.h>
#include <Util/include/hex.h>

#include <TAO/Ledger/types/transaction.h>

namespace TAO
{

    namespace Ledger
    {

        /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
        bool Transaction::IsValid() const
        {
            //1. read hash genesis
            //2. check the previous nexthash claims (need INDEX)

            LLC::ECKey keyVerify(NID_brainpoolP512t1, 64);
            keyVerify.SetPubKey(vchPubKey);

            uint512_t hashTx = GetHash();
            return keyVerify.Verify(hashTx.GetBytes(), vchSig);
        }

        /* Determines if the transaction is a genesis transaction */
        bool Transaction::IsGenesis() const
        {
            return (nSequence == 0 && hashGenesis == 0);
        }


        /* Gets the hash of the transaction object. */
        uint512_t Transaction::GetHash() const
        {
            CDataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            return LLC::SK512(ss.begin(), ss.end());
        }


        /* Sets the Next Hash from the key */
        void Transaction::NextHash(uint512_t hashSecret)
        {
            CDataStream ssData(SER_NETWORK, nVersion);
            ssData << hashSecret;

            LLC::CSecret vchSecret(ssData.begin(), ssData.end());
            LLC::ECKey key(NID_brainpoolP512t1, 64);
            if(!key.SetSecret(vchSecret, true))
                return;

            hashNext = LLC::SK256(key.GetPubKey());
        }


        /* Gets the nextHash from the previous transaction */
        uint256_t Transaction::PrevHash() const
        {
            return LLC::SK256(vchPubKey);
        }


        /* Signs the transaction with the private key and sets the public key */
         bool Transaction::Sign(uint512_t hashSecret)
         {
            CDataStream ssData(SER_NETWORK, nVersion);
            ssData << hashSecret;

            LLC::CSecret vchSecret(ssData.begin(), ssData.end());

            LLC::ECKey key(NID_brainpoolP512t1, 64);
            if(!key.SetSecret(vchSecret, true))
                return false;

            vchPubKey = key.GetPubKey();
            uint512_t hashTx = GetHash();
            return key.Sign(hashTx.GetBytes(), vchSig);
         }

         void Transaction::print() const
         {
             printf("Tritium(nVersion = %u, nTimestamp = %" PRIu64 ", hashNext = %s, hashGenesis = %s, pub=%s, sig=%s, hash=%s, ledger=%s)\n", nVersion, nTimestamp, hashNext.ToString().c_str(), hashGenesis.ToString().c_str(), HexStr(vchPubKey).c_str(), HexStr(vchSig).c_str(), GetHash().ToString().c_str(), HexStr(vchLedgerData.begin(), vchLedgerData.end()).c_str());
         }
    }
}
