/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "include/transaction.h"

#include "../../../Util/templates/serialize.h"
#include "../../../Util/include/hex.h"

#include "../../../LLC/hash/SK.h"
#include "../../../LLC/hash/macro.h"
#include "../../../LLC/include/key.h"

#include "../../../LLP/include/version.h"

namespace TAO
{

    namespace Ledger
    {


        /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
        bool TritiumTransaction::IsValid() const
        {
            //1. read hash genesis
            //2. check the previous nexthash claims (need INDEX)

            LLC::CKey keyVerify(NID_brainpoolP512t1, 64);
            keyVerify.SetPubKey(vchPubKey);

            uint512_t hashTx = GetHash();
            return keyVerify.Verify(hashTx, vchSig, 512);
        }


        /* Gets the hash of the transaction object. */
        uint512_t TritiumTransaction::GetHash() const
        {
            CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
            ss << *this;

            return LLC::SK512(ss.begin(), ss.end());
        }


        /* Sets the Next Hash from the key */
        void TritiumTransaction::NextHash(uint512_t hashSecret)
        {
            std::vector<uint8_t> vchData = hashSecret.GetBytes(); //todo change this a bit

            LLC::CSecret vchSecret(vchData.begin(), vchData.end());

            LLC::CKey key(NID_brainpoolP512t1, 64);
            if(!key.SetSecret(vchSecret, true))
                return;

            hashNext = LLC::SK256(key.GetPubKey());
        }

        /* Signs the transaction with the private key and sets the public key */
         bool TritiumTransaction::Sign(uint512_t hashSecret)
         {
            std::vector<uint8_t> vchData = hashSecret.GetBytes(); //todo change this a bit

            LLC::CSecret vchSecret(vchData.begin(), vchData.end());

            LLC::CKey key(NID_brainpoolP512t1, 64);
            if(!key.SetSecret(vchSecret, true))
                return false;

            vchPubKey = key.GetPubKey();
            uint512_t hashTx = GetHash();
            return key.Sign(hashTx, vchSig, 512);
         }

         void TritiumTransaction::print() const
         {
             printf("Tritium(nVersion = %u, nTimestamp = %" PRIu64 ", hashNext = %s, hashGenesis = %s, pub=%s, sig=%s, hash=%s, ledger=%s)\n", nVersion, nTimestamp, hashNext.ToString().c_str(), hashGenesis.ToString().c_str(), HexStr(vchPubKey).c_str(), HexStr(vchSig).c_str(), GetHash().ToString().c_str(), HexStr(vchLedgerData.begin(), vchLedgerData.end()).c_str());
         }
    }
}
