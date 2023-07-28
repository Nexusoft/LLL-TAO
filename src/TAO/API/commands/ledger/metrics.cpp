/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/evaluate.h>

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
            TAO::Ledger::ChainState::tStateBest.load();

        /* Get the current time of height of blockchain. */
        const uint64_t nBestTime =
            ExtractInteger<uint64_t>(jParams, "timestamp", tBestBlock.GetBlockTime());

        /* Track our contracts change as unsigned. */
        uint64_t nTotalTransactions [3] = {0, 0, 0};
        uint64_t nTotalContracts    [3] = {0, 0, 0};
        uint64_t nTotalDeposits     [3] = {0, 0, 0};
        uint64_t nTotalWithdraw     [3] = {0, 0, 0};

        /* Track our stake change as integer. */
        int64_t nStakeChange[3] = {0, 0, 0};

        /* Track our mining change as unsigned. */
        uint64_t nMiningEmmission [3] = {0, 0, 0};
        uint64_t nStakingEmmission[3] = {0, 0, 0};

        /* Track unique account holders. */
        std::set<uint256_t> setAccounts;

        /* Accumulate these as unsigned values. */
        uint64_t nUniqueAccounts[3] = {0, 0, 0};

        /* Iterate backwards until we have reached one whole day. */
        TAO::Ledger::BlockState tPrevBlock = tBestBlock;
        while(!config::fShutdown.load())
        {
            /* Track the amount of stake changed. */
            int64_t nStake = 0;

            /* Track total contracts for this block. */
            uint64_t nContracts = 0, nDeposits = 0, nWithdraws = 0, nInflation = 0, nMining = 0, nAccounts = 0;

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
                        /* Check for an available address that was modified. */
                        uint256_t hashAddress;
                        if(TAO::Register::Unpack(tx[n], hashAddress) && !setAccounts.count(hashAddress))
                        {
                            /* Insert into our set and increment totals. */
                            setAccounts.insert(hashAddress);
                            ++nAccounts;
                        }

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
                                    nWithdraws += nTotal;
                            }

                            /* Check for a legacy deposit. */
                            else if(tx[n].Primitive() == TAO::Operation::OP::LEGACY)
                                nDeposits += nTotal;

                            /* Check for our coinbase minting. */
                            else if(tx[n].Primitive() == TAO::Operation::OP::COINBASE)
                                nMining += nTotal;

                        }
                    }
                }

                /* Handle for legacy transactions and accounts. */
                if(proof.first == TAO::Ledger::TRANSACTION::LEGACY)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Make sure the transaction is on disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        continue;

                    /* Loop through all of our outputs to check. */
                    for(const Legacy::TxOut& out : tx.vout)
                    {
                        /* See if we are sending to register. */
                        uint256_t hashAddress;
                        if(Legacy::ExtractRegister(out.scriptPubKey, hashAddress) && !setAccounts.count(hashAddress))
                        {
                            /* Insert into our set and increment totals. */
                            setAccounts.insert(hashAddress);
                            ++nAccounts;
                        }

                        /* Check for legacy to legacy transacitons. */
                        Legacy::NexusAddress addrAccount;
                        if(Legacy::ExtractAddress(out.scriptPubKey, addrAccount) && !setAccounts.count(addrAccount.GetHash256()))
                        {
                            /* Insert into our set and increment totals. */
                            setAccounts.insert(addrAccount.GetHash256());
                            ++nAccounts;
                        }
                    }
                }
            }


            /* Check our time for days. */
            if(tPrevBlock.GetBlockTime() + 86400 > nBestTime)
            {
                /* Set our daily volume values. */
                nTotalTransactions[0] += (tPrevBlock.vtx.size());
                nTotalContracts   [0] += nContracts;
                nTotalDeposits    [0] += nDeposits;
                nTotalWithdraw    [0] += nWithdraws;
                nUniqueAccounts   [0] += nAccounts;

                /* Set our emmission values. */
                nMiningEmmission [0]  += nMining;
                nStakingEmmission[0]  += nInflation;

                /* Set our daily accumulation values. */
                nStakeChange[0]       += nStake;
            }


            /* Check our time for weeks. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 > nBestTime)
            {
                /* Set our daily volume values. */
                nTotalTransactions[1] += (tPrevBlock.vtx.size());
                nTotalContracts   [1] += nContracts;
                nTotalDeposits    [1] += nDeposits;
                nTotalWithdraw    [1] += nWithdraws;
                nUniqueAccounts   [1] += nAccounts;

                /* Set our emmission values. */
                nMiningEmmission [1]  += nMining;
                nStakingEmmission[1]  += nInflation;

                /* Set our daily accumulation values. */
                nStakeChange[1]       += nStake;
            }


            /* Check our time for months. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 * 4 > nBestTime)
            {
                /* Set our daily volume values. */
                nTotalTransactions[2] += (tPrevBlock.vtx.size());
                nTotalContracts   [2] += nContracts;
                nTotalDeposits    [2] += nDeposits;
                nTotalWithdraw    [2] += nWithdraws;
                nUniqueAccounts   [2] += nAccounts;

                /* Set our emmission values. */
                nMiningEmmission [2]  += nMining;
                nStakingEmmission[2]  += nInflation;

                /* Set our daily accumulation values. */
                nStakeChange[2]       += nStake;
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
                    { "daily",   nTotalTransactions[0]   },
                    { "weekly",  nTotalTransactions[1]  },
                    { "monthly", nTotalTransactions[2] }
                }
            },
            {
                "contracts",
                {
                    { "daily",   nTotalContracts[0]   },
                    { "weekly",  nTotalContracts[1]  },
                    { "monthly", nTotalContracts[2] }
                }
            },
            {
                "accounts",
                {
                    { "daily",   nUniqueAccounts[0]   },
                    { "weekly",  nUniqueAccounts[1]  },
                    { "monthly", nUniqueAccounts[2] }
                }
            }
        };

        /* Track our list of exchange deposits and withdraws. */
        const encoding::json jExchanges =
        {
            {
                "deposits",
                {
                    { "daily",   FormatBalance(nTotalDeposits[0])   },
                    { "weekly",  FormatBalance(nTotalDeposits[1])  },
                    { "monthly", FormatBalance(nTotalDeposits[2]) }
                }
            },
            {
                "withdraws",
                {
                    { "daily",   FormatBalance(nTotalWithdraw[0])   },
                    { "weekly",  FormatBalance(nTotalWithdraw[1])  },
                    { "monthly", FormatBalance(nTotalWithdraw[2]) }
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
                            { "daily",   FormatBalance(nStakingEmmission[0])   },
                            { "weekly",  FormatBalance(nStakingEmmission[1])  },
                            { "monthly", FormatBalance(nStakingEmmission[2]) }
                        }
                    },
                    {
                        "mining",
                        {
                            { "daily",   FormatBalance(nMiningEmmission[0])   },
                            { "weekly",  FormatBalance(nMiningEmmission[1])  },
                            { "monthly", FormatBalance(nMiningEmmission[2]) }
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
