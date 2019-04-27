/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>

#if defined USE_FALCON
#include <LLC/include/flkey.h>
#else
#include <LLC/include/eckey.h>
#endif

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <Util/templates/datastream.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
        bool Transaction::IsValid() const
        {
            /* Read the previous transaction from disk. */
            if(!IsGenesis())
            {
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashPrevTx, tx))
                    return debug::error(FUNCTION, "failed to read previous transaction");

                /* Check the previous next hash that is being claimed. */
                if(tx.hashNext != PrevHash())
                    return debug::error(FUNCTION, "next hash mismatch with previous transaction");

                /* Check the previous sequence number. */
                if(tx.nSequence + 1 != nSequence)
                    return debug::error(FUNCTION, "previous sequence ", tx.nSequence, " not sequential ", nSequence);

                /* Check the previous genesis. */
                if(tx.hashGenesis != hashGenesis)
                    return debug::error(FUNCTION, "previous genesis ", tx.hashGenesis.ToString().substr(0, 20), " mismatch ",  hashGenesis.ToString().substr(0, 20));
            }
            else
            {
                /* System memory cannot be allocated on genesis. */
                if(ssSystem.size() != 0)
                    return debug::error(FUNCTION, "no system memory available on genesis");
            }

            /* Check for Trust. */
            if(!IsTrust() && ssSystem.size() != 0)
                return debug::error(FUNCTION, "no system memory available when not trust");

            /* Checks for coinbase. */
            if(IsCoinbase())
            {
                /* Check the coinbase size. */
                if(ssOperation.size() != 17)
                    return debug::error(FUNCTION, "operation data inconsistent with fixed size rules ", ssOperation.size());
            }

            /* Check for genesis valid numbers. */
            if(hashGenesis == 0)
                return debug::error(FUNCTION, "genesis cannot be zero");

            /* Check the timestamp. */
            if(nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "transaction timestamp too far in the future ", nTimestamp);

            /* Check the size constraints of the ledger data. */
            if(ssOperation.size() > 1024) //TODO: implement a constant max size
                return debug::error(FUNCTION, "ledger data outside of maximum size constraints");

            /* Check the more expensive ECDSA verification. */
            #if defined USE_FALCON
            LLC::FLKey key;
            #else
            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
            #endif

            key.SetPubKey(vchPubKey);
            if(!key.Verify(GetHash().GetBytes(), vchSig))
                return debug::error(FUNCTION, "invalid transaction signature");

            return true;
        }


        /* Extract the trust data from the input script. */
        bool Transaction::ExtractTrust(uint1024_t& hashLastBlock, uint32_t& nSequence, uint32_t& nTrustScore) const
        {
            /* Don't extract trust if not coinstake. */
            if(!IsTrust())
                return debug::error(FUNCTION, "not proof of stake");

            /* Seek the stream to the beginning. */
            ssOperation.seek(1, STREAM::BEGIN);

            /* The account that is being staked. */
            uint256_t hashAccount;
            ssOperation >> hashAccount;

            /* Deserialize the values from stream. */
            ssOperation >> hashLastBlock >> nSequence >> nTrustScore;

            return true;
        }


        /* Extract the stake data from the input script. */
        bool Transaction::ExtractStake(uint64_t& nStake) const
        {
            /* Don't extract trust if not coinstake. */
            if(!IsTrust())
                return debug::error(FUNCTION, "not proof of stake");

            /* Seek the stream to the beginning. */
            ssOperation.seek(169, STREAM::BEGIN);

            /* Deserialize the values from stream. */
            ssOperation >> nStake;

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
            #if defined USE_FALCON
            LLC::FLKey key;
            #else
            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
            #endif
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
            #if defined USE_FALCON
            LLC::FLKey key;
            #else
            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
            #endif
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
            debug::log(0, IsGenesis() ? "Genesis" : "Tritium", "(",
                "nVersion = ", nVersion, ", ",
                "nSequence = ", nSequence, ", ",
                "nTimestamp = ", nTimestamp, ", ",
                "hashNext  = ",  hashNext.ToString().substr(0, 20), ", ",
                "hashPrevTx = ", hashPrevTx.ToString().substr(0, 20), ", ",
                "hashGenesis = ", hashGenesis.ToString().substr(0, 20), ", ",
                "pub = ", HexStr(vchPubKey).substr(0, 20), ", ",
                "sig = ", HexStr(vchSig).substr(0, 20), ", ",
                "hash = ", GetHash().ToString().substr(0, 20), ", ",
                "register.size() = ", ssRegister.size(), ", ",
                "operation.size() = ", ssOperation.size(), ")" );
        }

        /* Short form of the debug output. */
        std::string Transaction::ToStringShort() const
        {
            std::string str;
            std::string txtype = GetTxTypeString();
            str += debug::safe_printstr(GetHash().ToString(), " ", txtype);
            return str;
        }

        /*  User readable description of the transaction type. */
        std::string Transaction::GetTxTypeString() const
        {
            std::string txtype = "tritium ";
            if(IsCoinbase())
                txtype += "base";
            else if(IsTrust())
                txtype += "trust";
            else if(IsGenesis())
                txtype += "genesis";
            else
                txtype += "user";

            return txtype;
        }
    }
}
