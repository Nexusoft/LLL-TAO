/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/templates/sector.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <LLP/templates/ddos.h>
#include <Util/include/runtime.h>

#include <list>
#include <variant>

#include <Util/include/softfloat.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>
{
public:
    TestDB()
    : SectorDatabase("testdb"
    , LLD::FLAGS::CREATE | LLD::FLAGS::FORCE
    , 256 * 256 * 64
    , 1024 * 1024 * 4)
    {
    }

    ~TestDB()
    {

    }

    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }


    bool ReadLast(uint1024_t& last)
    {
        return Read(std::string("last"), last);
    }


    bool WriteHash(const uint1024_t& hash, const uint1024_t& hash2)
    {
        return Write(std::make_pair(std::string("hash"), hash), hash2, "hash");
    }

    bool WriteHash2(const uint1024_t& hash, const uint1024_t& hash2)
    {
        return Write(std::make_pair(std::string("hash2"), hash), hash2, "hash");
    }

    bool IndexHash(const uint1024_t& hash)
    {
        return Index(std::make_pair(std::string("hash2"), hash), std::make_pair(std::string("hash"), hash));
    }


    bool ReadHash(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash"), hash), hash2);
    }


    bool ReadHash2(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash2"), hash), hash2);
    }


    bool EraseHash(const uint1024_t& hash)
    {
        return Erase(std::make_pair(std::string("hash"), hash));
    }
};


/*
Hash Tables:

Set max tables per timestamp.

Keep disk index of all timestamps in memory.

Keep caches of Disk Index files (LRU) for low memory footprint

Check the timestamp range of bucket whether to iterate forwards or backwards


_hashmap.000.0000
_name.shard.file
  t0 t1 t2
  |  |  |

  timestamp each hashmap file if specified
  keep indexes in TemplateLRU

  search from nTimestamp < timestamp[nShard][nHashmap]

*/

namespace TAO
{
    namespace Ledger
    {
        namespace Test
        {

            /* Calculate the proof of stake block weight for a given block age. */
            cv::softdouble BlockWeight(const uint64_t nBlockAge)
            {
                /* Block Weight reaches maximum of 10.0 when Block Age equals the max block age */
                cv::softdouble nBlockRatio = cv::softdouble(nBlockAge) / cv::softdouble(MaxBlockAge());

                return std::min(cv::softdouble(10.0), (cv::softdouble(9.0) * cv::log((cv::softdouble(2.0) * nBlockRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(3))) + cv::softdouble(1.0));
            }


            /* Calculate the equivalent proof of stake trust weight for staking Genesis with a given coin age. */
            cv::softdouble GenesisWeight(const uint64_t nCoinAge)
            {
                /* Trust Weight For Genesis is based on Coin Age. Genesis trust weight is less than normal trust weight,
                 * reaching a maximum of 10.0 after average Coin Age reaches trust weight base.
                 */
                cv::softdouble nWeightRatio = cv::softdouble(nCoinAge) / cv::softdouble(TrustWeightBase());

                return std::min(cv::softdouble(10.0), (cv::softdouble(9.0) * cv::log((cv::softdouble(2.0) * nWeightRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(3))) + cv::softdouble(1.0));
            }


            /* Calculate the proof of stake trust weight for a given trust score. */
            cv::softdouble TrustWeight(const uint64_t nTrust)
            {
                /* Trust Weight base is time for 50% score. Weight continues to grow with Trust Score until it reaches max of 90.0
                 * This formula will reach 45.0 (50%) after accumulating 84 days worth of Trust Score (Mainnet base),
                 * while requiring close to a year to reach maximum.
                 */
                cv::softdouble nWeightRatio = cv::softdouble(nTrust) / cv::softdouble(TrustWeightBase());

                return std::min(cv::softdouble(90.0), (cv::softdouble(44.0) * cv::log((cv::softdouble(2.0) * nWeightRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(3))) + cv::softdouble(1.0));
            }


            /* Calculate the current threshold value for Proof of Stake. */
            cv::softdouble GetCurrentThreshold(const uint64_t nBlockTime, const uint64_t nNonce)
            {
                return (cv::softdouble(nBlockTime) * cv::softdouble(100.0)) / cv::softdouble(nNonce);
            }


            /* Calculate the minimum Required Energy Efficiency Threshold. */
            cv::softdouble GetRequiredThreshold(cv::softdouble nTrustWeight, cv::softdouble nBlockWeight, const uint64_t nStake)
            {
                /*  Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
                 *  Weight from staking balance reduces the required threshold by increasing the denominator.
                 */
                return ((cv::softdouble(108.0) - cv::softdouble(nTrustWeight) - cv::softdouble(nBlockWeight)) * cv::softdouble(TAO::Ledger::MAX_STAKE_WEIGHT)) / cv::softdouble(nStake);
            }


            /* Calculate the stake rate corresponding to a given trust score. */
            cv::softdouble StakeRate(const uint64_t nTrust, const bool fGenesis)
            {
                /* Stake rate fixed at 0.005 (0.5%) when staking Genesis */
                if(fGenesis)
                    return cv::softdouble(0.005);

                /* No trust score cap in Tritium staking, but use testnet max for testnet stake rate (so it grows faster) */
                static const uint32_t nRateBase = config::fTestNet ? TRUST_SCORE_MAX_TESTNET : ONE_YEAR;

                /* Stake rate starts at 0.005 (0.5%) and grows to 0.03 (3%) when trust score reaches or exceeds one year */
                cv::softdouble nTrustRatio = cv::softdouble(nTrust) / cv::softdouble(nRateBase);

                return std::min(cv::softdouble(0.03), (cv::softdouble(0.025) * cv::log((cv::softdouble(9.0) * nTrustRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(10))) + cv::softdouble(0.005));
            }


            /* Calculate the coinstake reward for a given stake. */
            uint64_t GetCoinstakeReward(const uint64_t nStake, const uint64_t nStakeTime, const uint64_t nTrust, const bool fGenesis)
            {

                double nStakeRate = StakeRate(nTrust, fGenesis);

                /* Reward rate for time period is annual rate * (time period / annual time) or nStakeRate * (nStakeTime / ONE_YEAR)
                 * Then, overall nStakeReward = nStake * reward rate
                 *
                 * Thus, the appropriate way to write this (for clarity) would be:
                 *      StakeReward = nStake * nStakeRate * (nStakeTime / ONE_YEAR)
                 *
                 * However, with integer arithmetic (nStakeTime / ONE_YEAR) would evaluate to 0 or 1, etc. and the nStakeReward
                 * would be erroneous.
                 *
                 * Therefore, it performs the full multiplication portion first.
                 */
                uint64_t nStakeReward = (nStake * nStakeRate * nStakeTime) / ONE_YEAR;

                return nStakeReward;
            }
        }
    }
}




/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    TAO::Ledger::Block block;
    debug::log(0, "Hash ", block.GetHash().ToString());

    TAO::Ledger::TritiumBlock tritium(block);
    debug::log(0, "Hash ", tritium.GetHash().ToString());

    Legacy::LegacyBlock legacy(block);
    debug::log(0, "Hash ", legacy.GetHash().ToString());


    return 0;

    for(uint64_t i = 0; i < 1000000; ++i)
    {
        double dWeight = TAO::Ledger::StakeRate(i, false);
        cv::softdouble dWeight2 = TAO::Ledger::Test::StakeRate(i, false);

        if(cv::softdouble(dWeight) != dWeight2)
        {
            debug::log(0, "Failed at ", i);
            debug::log(0, std::fixed, dWeight);
            debug::log(0, std::fixed, double(dWeight2));

            //runtime::sleep(1000);
        }

    }

    return 0;
}
