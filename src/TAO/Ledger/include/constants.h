
/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#include <LLC/types/bignum.h>

#include <TAO/Ledger/types/state.h>


/* Global TAO namespace. */
namespace TAO::Ledger
{
    /** Very first block hash in the blockchain. **/
    const uint1024_t hashGenesis
    (
          std::string("0x")
        + std::string("00000bb8601315185a0d001e19e63551c34f1b7abeb3445105077522a9cbabf3")
        + std::string("e1da963e3bfbb87d260218b7c6207cb9c6df90b86e6a3ce84366434763bc8bcb")
        + std::string("f6ccbd1a7d5958996aecbe8205c20b296818efb3a59c74acbc7a2d1a5a6b64aa")
        + std::string("b63839b8b11a6b41c4992f835cbbc576d338404fb1217bdd7909ca7db63bbc02")
    );


    /** Very first tritium block in EVER! **/
    const uint1024_t hashTritium
    (
          std::string("0x")
        + std::string("9e804d2d1e1d3f64629939c6f405f15bdcf8cd18688e140a43beb2ac049333a2")
        + std::string("30d409a1c4172465b6642710ba31852111abbd81e554b4ecb122bdfeac9f73d4")
        + std::string("f1570b6b976aa517da3c1ff753218e1ba940a5225b7366b0623e4200b8ea97ba")
        + std::string("09cb93be7d473b47b5aa75b593ff4b8ec83ed7f3d1b642b9bba9e6eda653ead9")
    );


    /** Hash to start the Test Net Blockchain. **/
    const uint1024_t hashGenesisTestnet
    (
          std::string("0x")
        + std::string("00002a0ccd35f2e1e9e1c08f5a2109a82834606b121aca891d5862ba12c6987d")
        + std::string("55d1e789024fcd5b9adaf07f5445d24e78604ea136a0654497ed3db0958d63e7")
        + std::string("2f146fae2794e86323126b8c3d8037b193ce531c909e5222d090099b4d574782")
        + std::string("d15c135ddd99d183ec14288437563e8a6392f70259e761e62d2ea228977dd2f7")
    );


    /** Hash to start a hybrid Blockchain. **/
    extern uint1024_t hashGenesisHybrid;


    /** Minimum channels difficulty. **/
    const LLC::CBigNum bnProofOfWorkLimit[] =
    {
        LLC::CBigNum(~uint1024_t(0) >> 5),
        LLC::CBigNum(20000000),
        LLC::CBigNum(~uint1024_t(0) >> 17)
    };


    /** Starting channels difficulty. **/
    const LLC::CBigNum bnProofOfWorkStart[] =
    {
        LLC::CBigNum(~uint1024_t(0) >> 7),
        LLC::CBigNum(25000000),
        LLC::CBigNum(~uint1024_t(0) >> 22)
    };


    /** Minimum prime zero bits (1016-bits). **/
    const LLC::CBigNum bnPrimeMinOrigins = LLC::CBigNum(~uint1024_t(0) >> 8); //minimum prime origins of 1016 bits


    /** Minimum proof of work for first transaction. **/
    const uint512_t FIRST_REQUIRED_WORK = (~uint512_t(0) >> 16);


    /** Maximum size a block can be in transit. **/
    const uint32_t MAX_BLOCK_SIZE = 1024 * 1024 * 2;


    /** Maximum size a block can be generated as. **/
    const uint32_t MAX_BLOCK_SIZE_GEN = 1000000;


    /** Maximum signature operations per block. **/
    const uint32_t MAX_BLOCK_SIGOPS = 40000;


    /** Maximum contracts per transction. **/
    const uint32_t MAX_TRANSACTION_CONTRACTS = 100;


    /** Nexus Coinbase/Coinstake Maturity Settings **/
    const uint32_t TESTNET_MATURITY_BLOCKS = 10;


    /** Mainnet maturity for blocks. */
    const uint32_t NEXUS_MATURITY_LEGACY = 100;


    /** Mainnet maturity for coinbase. **/
    const uint32_t NEXUS_MATURITY_COINBASE = 500;


    /** Mainnet maturity for coinstake. **/
    const uint32_t NEXUS_MATURITY_COINSTAKE = 250;


    /** Stake reward rate is annual. Define one year (364 days) of time for reward calculations **/
    const uint32_t ONE_YEAR = 60 * 60 * 24 * 28 * 13;


    /** nVersion 4 and earlier trust keys expire after 24 hours. **/
    const uint32_t TRUST_KEY_EXPIRE   = 60 * 60 * 24;


    /** nVersion > 4 - timespan is 3 days (max block age before decay) **/
    const uint32_t TRUST_KEY_TIMESPAN = 60 * 60 * 24 * 3;


    /** Timespan of trust key for testnet. (3 hours) **/
    const uint32_t TRUST_KEY_TIMESPAN_TESTNET = 60 * 60 * 3;


    /** The maximum allowed value for trust score (364 days). Legacy value. **/
    const uint32_t TRUST_SCORE_MAX = 60 * 60 * 24 * 28 * 13;


    /** The maximum allowed value for trust score for testnet (364 hrs) **/
    const uint32_t TRUST_SCORE_MAX_TESTNET = 60 * 60 * 28 * 13;


    /** The base value for calculating trust weight (84 days) **/
    const uint32_t TRUST_WEIGHT_BASE = 60 * 60 * 24 * 28 * 3;


    /** The base value for calculating Testnet trust weight (84 hours) **/
    const uint32_t TRUST_WEIGHT_BASE_TESTNET = 60 * 60 * 28 * 3;


    /** Minimum average coin age to stake Genesis **/
    const uint32_t MINIMUM_GENESIS_COIN_AGE = TRUST_KEY_TIMESPAN;


    /** Minimum average coin age to stake Genesis on Testnet (10 minutes) **/
    const uint32_t MINIMUM_GENESIS_COIN_AGE_TESTNET = 60 * 10;


    /** Minimum span between trust blocks testnet **/
    const uint32_t TESTNET_MINIMUM_INTERVAL = 10;


    /** Minimum span between trust blocks, legacy mainnet (allows validation of stake blocks from legacy wallets during transition period). **/
    const uint32_t MAINNET_MINIMUM_INTERVAL_LEGACY = 120;


    /** Minimum span between trust blocks mainnet prior to v9 when pooled staking implemented. **/
    const uint32_t MAINNET_MINIMUM_INTERVAL_PREPOOL = 250;


    /** Minimum span between trust blocks mainnet. **/
    const uint32_t MAINNET_MINIMUM_INTERVAL = 60;


    /** NXS token default digits. **/
    const uint8_t NXS_DIGITS = 6;


    /** Integral value for one NXS coin (1 * 10 ^ NXS_DIGITS). **/
    const uint64_t NXS_COIN = 1000000;


    /** Integral value for one NXS coin (1 * 10 ^ NXS_DIGITS). **/
    const uint64_t NXS_CENT = 10000;


    /** The stake threshold weight multiplier. **/
    const uint64_t MAX_STAKE_WEIGHT = 1000 * NXS_COIN;


    /* The fee-free interval between transactions in seconds*/
    const uint64_t TX_FEE_INTERVAL = 10;


    /* The cost per contract when creating transactions faster than the fee-free threshold */
    const uint64_t TX_FEE = 0.01 * NXS_COIN;



    /** MaturityCoinbase
     *
     *  Retrieve the number of blocks (confirmations) required for coinbase maturity for a given block.
     *
     *  @param[in] block - Block to which this maturity requirement will apply
     *
     *  @return maturity setting for coinbase producer, based on current testnet or mainnet and block version
     *
     **/
    uint32_t MaturityCoinBase(const BlockState& block);



    /** MaturityCoinstake
     *
     *  Retrieve the number of blocks (confirmations) required for coinstake maturity for a given block.
     *
     *  @param[in] block - Block to which this maturity requirement will apply
     *
     *  @return maturity setting for coinstake producer, based on current testnet or mainnet and block version
     *
     **/
    uint32_t MaturityCoinStake(const BlockState& block);
}
