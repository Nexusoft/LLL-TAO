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
        uint64_t nDaily[4]   = {0, 0, 0, 0};
        uint64_t nWeekly[4]  = {0, 0, 0, 0};
        uint64_t nMonthly[4] = {0, 0, 0, 0};

        /* Iterate backwards until we have reached one whole day. */
        TAO::Ledger::BlockState tPrevBlock = tBestBlock;
        while(!config::fShutdown.load())
        {
            /* Track total contracts for this block. */
            uint64_t nContracts = 0, nLegacy = 0, nTritium = 0;

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
                            /* Check for a legacy deposit. */
                            if(tx[n].Primitive() == TAO::Operation::OP::LEGACY)
                                nLegacy += nTotal;
                            else
                                nTritium += nTotal;
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
            }


            /* Check our time for weeks. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 > nBestTime)
            {
                /* Increase our weekly transaction volume. */
                nWeekly[0] += (tPrevBlock.vtx.size());
                nWeekly[1] += nContracts;
                nWeekly[2] += nLegacy;
                nWeekly[3] += nTritium;
            }


            /* Check our time for months. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 * 4 > nBestTime)
            {
                /* Increase our weekly transaction volume. */
                nMonthly[0] += (tPrevBlock.vtx.size());
                nMonthly[1] += nContracts;
                nMonthly[2] += nLegacy;
                nMonthly[3] += nTritium;
            }
            else
                break;

            /* Iterate to previous block. */
            tPrevBlock = tPrevBlock.Prev();
        }

        /* Build our list of transactions. */
        const encoding::json jTransactions =
        {
            { "daily",   nDaily[0]   },
            { "weekly",  nWeekly[0]  },
            { "monthly", nMonthly[0] }
        };

        /* Build our list of contracts. */
        const encoding::json jContracts =
        {
            { "daily",   nDaily[1]   },
            { "weekly",  nWeekly[1]  },
            { "monthly", nMonthly[1] }
        };

        /* Build our list of contracts. */
        const encoding::json jLegacy =
        {
            { "daily",   FormatBalance(nDaily[2])   },
            { "weekly",  FormatBalance(nWeekly[2])  },
            { "monthly", FormatBalance(nMonthly[2]) }
        };

        /* Build our list of contracts. */
        const encoding::json jTritium =
        {
            { "daily",   FormatBalance(nDaily[3])   },
            { "weekly",  FormatBalance(nWeekly[3])  },
            { "monthly", FormatBalance(nMonthly[3]) }
        };

        /* Add chain-state data. */
        encoding::json jRet;
        jRet["transactions"] = jTransactions;
        jRet["contracts"]    = jContracts;
        jRet["legacy"]       = jLegacy;
        jRet["tritium"]      = jTritium;

        /* Filter our fieldname. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
} /* End Ledger Namespace*/
