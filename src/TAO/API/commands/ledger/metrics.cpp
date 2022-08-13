/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns an object containing mining-related information. */
    encoding::json Ledger::GetMetrics(const encoding::json& jParams, const bool fHelp)
    {
        /* Grab our best block. */
        const TAO::Ledger::BlockState tBestBlock =
            TAO::Ledger::ChainState::stateBest.load();

        /* Get the current time of height of blockchain. */
        const uint64_t nBestTime =
            ExtractInteger<uint64_t>(jParams, "timestamp", tBestBlock.GetBlockTime());

        /* Get total amount of transactions processed. */
        uint64_t nDaily[5]   = {0, 0, 0, 0, 0};
        uint64_t nWeekly[5]  = {0, 0, 0, 0, 0};
        uint64_t nMonthly[5] = {0, 0, 0, 0, 0};

        /* Track our stake change as integer. */
        int64_t nStakeChange[3] = {0, 0, 0};

        /* Track our mining change as unsigned. */
        uint64_t nMiningChange[3] = {0, 0, 0};

        /* Iterate backwards until we have reached one whole day. */
        TAO::Ledger::BlockState tPrevBlock = tBestBlock;
        while(!config::fShutdown.load())
        {
            /* Track the amount of stake changed. */
            int64_t nStake = 0;

            /* Track total contracts for this block. */
            uint64_t nContracts = 0, nLegacy = 0, nTritium = 0, nInflation = 0, nMining = 0;

            /* Check through all the transactions. */
            for(const auto& proof : tPrevBlock.vtx)
            {
                /* Only work on tritium transactions for now. */
                if(proof.first == TAO::Ledger::TRANSACTION::TRITIUM)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Make sure the transaction is on disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        continue;

                    /* Increment total contracts. */
                    nContracts += tx.Size();

                    /* Iterate all of our contracts. */
                    for(uint32_t n = 0; n < tx.Size(); ++n)
                    {
                        /* Get our total NXS being spent. */
                        uint64_t nTotal = 0;

                        /* Unpack our total now from contracts. */
                        if(TAO::Register::Unpack(tx[n], nTotal))
                        {
                            /* Check for trust transactions. */
                            if(tx[n].Primitive() == TAO::Operation::OP::TRUST)
                            {
                                /* Accumulate our trust totals. */
                                nInflation += nTotal;

                                /* Jump to starting OP */
                                tx[n].SeekToPrimitive();

                                /* Skip to stake change. */
                                tx[n].Seek(73);

                                /* Get our stake change value. */
                                int64_t nChange = 0;
                                tx[n] >> nChange;

                                /* Adjust our current stake. */
                                nStake += nChange;
                            }

                            /* Check for trust transactions. */
                            else if(tx[n].Primitive() == TAO::Operation::OP::GENESIS)
                            {
                                /* Accumulate our inflation totals. */
                                nInflation += nTotal;

                                /* Get our pre-state to find stake. */
                                TAO::Register::Object tPreState =
                                    tx[n].PreState();

                                /* Parse our object. */
                                if(!tPreState.Parse())
                                    continue;

                                /* Our balance is our committed stake. */
                                nStake += tPreState.get<uint64_t>("balance");
                            }

                            /* Check only for debits. */
                            else if(tx[n].Primitive() == TAO::Operation::OP::CREDIT)
                            {
                                /* Check only for credits from legagy. */
                                uint512_t hashPrevTx;
                                if(TAO::Register::Unpack(tx[n], hashPrevTx) && hashPrevTx.GetType() == TAO::Ledger::LEGACY)
                                    nTritium += nTotal;
                            }

                            /* Check for a legacy deposit. */
                            else if(tx[n].Primitive() == TAO::Operation::OP::LEGACY)
                                nLegacy += nTotal;

                            /* Check for our coinbase minting. */
                            else if(tx[n].Primitive() == TAO::Operation::OP::COINBASE)
                                nMining += nTotal;

                        }
                    }
                }
            }


            /* Check our time for days. */
            if(tPrevBlock.GetBlockTime() + 86400 > nBestTime)
            {
                /* Increase our daily transaction volume. */
                nDaily[0] += (tPrevBlock.vtx.size());
                nDaily[1] += nContracts;
                nDaily[2] += nLegacy;
                nDaily[3] += nTritium;
                nDaily[4] += nInflation;

                /* Accumulate our stake. */
                nStakeChange[0] += nStake;

                /* Accumulate our mining. */
                nMiningChange[0] += nMining;
            }


            /* Check our time for weeks. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 > nBestTime)
            {
                /* Increase our weekly transaction volume. */
                nWeekly[0] += (tPrevBlock.vtx.size());
                nWeekly[1] += nContracts;
                nWeekly[2] += nLegacy;
                nWeekly[3] += nTritium;
                nWeekly[4] += nInflation;

                /* Accumulate our stake. */
                nStakeChange[1] += nStake;

                /* Accumulate our mining. */
                nMiningChange[1] += nMining;
            }


            /* Check our time for months. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 * 4 > nBestTime)
            {
                /* Increase our weekly transaction volume. */
                nMonthly[0] += (tPrevBlock.vtx.size());
                nMonthly[1] += nContracts;
                nMonthly[2] += nLegacy;
                nMonthly[3] += nTritium;
                nMonthly[4] += nInflation;

                /* Accumulate our stake. */
                nStakeChange[2] += nStake;

                /* Accumulate our mining. */
                nMiningChange[2] += nMining;
            }
            else
                break;

            /* Iterate to previous block. */
            tPrevBlock = tPrevBlock.Prev();
        }

        /* Track our list of volumes on network. */
        const encoding::json jVolumes =
        {
            {
                "transactions",
                {
                    { "daily",   nDaily[0]   },
                    { "weekly",  nWeekly[0]  },
                    { "monthly", nMonthly[0] }
                }
            },
            {
                "contracts",
                {
                    { "daily",   nDaily[1]   },
                    { "weekly",  nWeekly[1]  },
                    { "monthly", nMonthly[1] }
                }
            }
        };

        /* Track our list of exchange deposits and withdraws. */
        const encoding::json jExchanges =
        {
            {
                "deposits",
                {
                    { "daily",   FormatBalance(nDaily[2])   },
                    { "weekly",  FormatBalance(nWeekly[2])  },
                    { "monthly", FormatBalance(nMonthly[2]) }
                }
            },
            {
                "withdraws",
                {
                    { "daily",   FormatBalance(nDaily[3])   },
                    { "weekly",  FormatBalance(nWeekly[3])  },
                    { "monthly", FormatBalance(nMonthly[3]) }
                }
            }
        };

        /* Track our list of network activity. */
        const encoding::json jNetwork =
        {
            {
                "mint",
                {
                    {
                        "staking",
                        {
                            { "daily",   FormatBalance(nDaily[4])   },
                            { "weekly",  FormatBalance(nWeekly[4])  },
                            { "monthly", FormatBalance(nMonthly[4]) }
                        }
                    },
                    {
                        "mining",
                        {
                            { "daily",   FormatBalance(nMiningChange[0])   },
                            { "weekly",  FormatBalance(nMiningChange[1])  },
                            { "monthly", FormatBalance(nMiningChange[2]) }
                        }
                    }
                }
            },
            {
                "stake",
                {
                    { "daily",   FormatStake(nStakeChange[0]) },
                    { "weekly",  FormatStake(nStakeChange[1]) },
                    { "monthly", FormatStake(nStakeChange[2]) }
                }
            }
        };

        /* Add chain-state data. */
        encoding::json jRet;
        jRet["volumes"]    = jVolumes;
        jRet["exchanges"]  = jExchanges;
        jRet["network"]    = jNetwork;

        /* Filter our fieldname. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
} /* End Ledger Namespace*/
