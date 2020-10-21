/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/stakepool.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/stake_minter.h>

#include <TAO/Operation/include/enum.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <random>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        Stakepool stakepool;

        /** Default Constructor. **/
        Stakepool::Stakepool()
        : MUTEX( )
        , mapPool( )
        , mapPending()
        , mapGenesis()
        , hashLastBlock(0)
        , hashProof(0)
        , nTimeBegin(0)
        , nTimeEnd(0)
        , nSizeMax(0)
        {
            nSizeMax = config::fTestNet.load() ? TAO::Ledger::POOL_MAX_SIZE_BASE_TESTNET : TAO::Ledger::POOL_MAX_SIZE_BASE;
        }


        /** Default Destructor. **/
        Stakepool::~Stakepool()
        {
        }


        /* Adds a transaction to the stake pool, making it available for pooled staking */
        bool Stakepool::AddToPool(const TAO::Ledger::Transaction& tx)
        {
            uint64_t nBalance;
            uint64_t nBlockAge;

            /* Get the trust account for tx user genesis */
            TAO::Register::Object account;
            bool fTrust;
            uint64_t nReward;
            uint64_t nPoolReward;

            if(!TAO::Ledger::FindTrustAccount(tx.hashGenesis, account, fTrust))
                return debug::error(FUNCTION, "Unable to retrieve trust account for pooled coinstake");

            /* Get the transaction's block age and balance for current mining round */
            if(fTrust)
            {
                /* trust */
                if(!tx.IsTrustPool())
                    return debug::error(FUNCTION, "Invalid operation for pooled trust");

                uint512_t hashLast;
                TAO::Ledger::BlockState stateLast;

                if(!LLD::Ledger->ReadStake(tx.hashGenesis, hashLast))
                {
                    /* If last stake is not directly available, search for it */
                    Transaction txLast;
                    if(TAO::Ledger::FindLastStake(tx.hashGenesis, txLast))
                        hashLast = txLast.GetHash();

                    else
                        return debug::error(FUNCTION, "Unable to find last stake for pooled coinstake");
                }

                if(!LLD::Ledger->ReadBlock(hashLast, stateLast))
                    return debug::error(FUNCTION, "Failed to get last block for pooled coinstake account");

                const TAO::Ledger::BlockState& stateBest = ChainState::stateBest.load();

                /* Check interval */
                const uint32_t nInterval = stateBest.nHeight - stateLast.nHeight + 1; //+1 because using prev block
                if(nInterval <= MinStakeInterval(stateBest))
                    return debug::error(FUNCTION, "pooled coinstake interval ", nInterval, " below minimum interval");

                /* Calculate time since last stake block (block age = age of previous stake block at time of current stateBest). */
                nBlockAge = stateBest.GetBlockTime() - stateLast.GetBlockTime();

                nBalance = account.get<uint64_t>("stake");

                /* Extract trust and post-fee reward from coinstake */
                uint64_t nTrust;

                tx[0].Reset();

                tx[0].Seek(113, TAO::Operation::Contract::OPERATIONS);

                tx[0] >> nTrust;

                tx[0].Seek(8, TAO::Operation::Contract::OPERATIONS);

                tx[0] >> nPoolReward;

                /* Calculate pre-fee trust reward */
                nReward = GetCoinstakeReward(nBalance, nBlockAge, nTrust);
            }
            else
            {
                /* genesis */
                if(!tx.IsGenesisPool())
                    return debug::error(FUNCTION, "Invalid operation for pooled genesis");

                nBlockAge = ChainState::stateBest.load().GetBlockTime() - account.nModified;

                nBalance = account.get<uint64_t>("balance");

                /* Extract post-fee reward from coinstake */
                tx[0].Reset();

                tx[0].Seek(49, TAO::Operation::Contract::OPERATIONS);

                tx[0] >> nPoolReward;

                /* Calculate pre-fee genesis reward */
                nReward = GetCoinstakeReward(nBalance, nBlockAge, 0, true);

                /* The value for block age used by the selection mechanism contributes to which coinstakes get chosen from pool.
                 * For genesis, account must have minimum age of one trust key timespan before it begins staking, so this
                 * is subtracted out.
                 */
                uint64_t nReduction = config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET
                                                              : TAO::Ledger::TRUST_KEY_TIMESPAN;

                if(nReduction > nBlockAge) //sanity check
                    nBlockAge = 0;
                else
                    nBlockAge -= nReduction;
            }

            if(nBalance < TAO::Ledger::POOL_MIN_STAKE_BALANCE)
                return debug::error(FUNCTION, "Pooled coinstake below minimum stake balance");

            uint64_t nFee = GetPoolStakeFee(nReward);

            if(nPoolReward != (nReward - nFee))
                return debug::error(FUNCTION, "Pooled coinstake invalid reward");

            /* Add to pool */
            mapPool[tx.GetHash()] = std::make_tuple(tx, nBlockAge, nBalance, nFee, fTrust);

            return true;
        }


        /* Accepts a transaction with validation rules. */
        bool Stakepool::CheckProofs(const TAO::Ledger::Transaction& tx)
        {
            /* Get proof values from coinstake contract */
            uint256_t txProof = 0;
            uint64_t txTimeBegin = 0;
            uint64_t txTimeEnd = 0;

            tx[0].Reset();

            if(tx.IsTrustPool())
                tx[0].Seek(65);
            else if(tx.IsGenesisPool())
                tx[0].Seek(1);
            else
                return debug::error(FUNCTION, "Invalid pooled coinstake transaction");

            tx[0] >> txProof;
            tx[0] >> txTimeBegin;
            tx[0] >> txTimeEnd;

            /* Check coinstake proofs against local proofs for current mining round */
            if(hashProof == txProof && nTimeBegin == txTimeBegin && nTimeEnd == txTimeEnd)
                return true;

            return false;
        }


        /* Accepts a transaction with validation rules. */
        bool Stakepool::Accept(const TAO::Ledger::Transaction& tx, LLP::TritiumNode* pnode)
        {
            RLOCK(MUTEX);

            /* Get the transaction hash. */
            uint512_t hashTx = tx.GetHash();

            debug::log(3, "STAKE POOL ACCEPT ----------------------------");
            if(config::nVerbose >= 3)
                tx.print();
            debug::log(3, "STAKE POOL END ACCEPT ------------------------");

            /* Transaction must be a pooled staking coinstake. */
            if(!(tx.IsTrustPool() || tx.IsGenesisPool()))
                return debug::error(FUNCTION, "Pooled stake tx ", hashTx.SubString(), " REJECTED: not a pooled staking coinstake");

            /* Check that the transaction is in a valid state. */
            if(!tx.Check())
                return debug::error(FUNCTION, "Pooled stake tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

            /* Check memory and disk for previous transaction. */
            if(!LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::MEMPOOL))
            {
                debug::error(FUNCTION, "Pooled stake tx ", hashTx.SubString(), " REJECTED:  missing previous tx ",
                             tx.hashPrevTx.SubString());

                /* Increment consecutive orphans. */
                if(pnode)
                    ++pnode->nConsecutiveOrphans;

                return false;
            }

            /* Verify transaction operations */
            if(!tx.Verify(FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "Pooled stake tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

            /* Check for duplicate (same user genesis) */
            if(mapGenesis.count(tx.hashGenesis))
            {
                uint512_t hashDup = mapGenesis[tx.hashGenesis];

                /* When tx hashes match, just received the same coinstake twice. Ignore it.
                 * If they don't match, we received two different coinstakes for the same hashGenesis. Use the first, log the dup.
                 */
                if(hashTx != hashDup)
                    debug::log(2, FUNCTION, "Pooled stake tx ", hashTx.SubString(), " REJECTED: Duplicate user genesis");

                return false;
            }

            /* Stake pool will only save tx when pooled staking is enabled and started */
            if(config::fPoolStaking.load() && StakeMinter::GetInstance().IsStarted())
            {
                /* When current pool proofs are stale, save tx as pending */
                if(hashLastBlock != TAO::Ledger::ChainState::hashBestChain.load())
                {
                    mapPending[hashTx] = tx;

                    debug::log(3, FUNCTION, "Pooled stake tx ", hashTx.SubString(), " ACCEPTED as pending");
                }

                /* Height not stale, save tx to pool if proofs verify.*/
                else if(CheckProofs(tx) && mapPool.size() < nSizeMax)
                {
                    if(!AddToPool(tx))
                        return debug::error(FUNCTION, "Pooled stake tx ", hashTx.SubString(), " FAILED: Unable to add to pool");

                    mapGenesis[tx.hashGenesis] = hashTx;

                    /* The coinstake produced by local node is in pool so it can be relayed, but should not be in Select results.
                     * Save the hash so we can filter in Select.
                     */
                    if(!pnode)
                        hashLocal = tx.GetHash();

                    debug::log(3, FUNCTION, "Pooled stake tx ", hashTx.SubString(), " ACCEPTED");
                }

                /* Proofs don't verify or max size reached. Don't use for local pooled staking, but do relay it */
                else
                    debug::log(3, FUNCTION, "Pooled stake tx ", hashTx.SubString(), " RELAYED");
            }

            /* Pooled staking not enabled or stake minter not running. tx not processed into pool, but is relayed as appropriate */
            else
                debug::log(3, FUNCTION, "Pooled stake tx ", hashTx.SubString(), " RELAYED");

            return true;
        }


        /* Gets a transaction from stakepool */
        bool Stakepool::Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx) const
        {
            RLOCK(MUTEX);

            /* Check in ledger memory. */
            if(mapPool.count(hashTx))
            {
                tx = std::get<0>(mapPool.at(hashTx));

                return true;
            }

            return false;
        }


        /* Check if a transaction exists in the stake pool */
        bool Stakepool::Has(const uint512_t& hashTx) const
        {
            RLOCK(MUTEX);

            /* Check if pool map contains the requested hashTx */
            if(mapPool.count(hashTx))
                    return true;

            return false;
        }


        /*  Checks whether a particular pool entry is a trust transaction. */
        bool Stakepool::IsTrust(const uint512_t& hashTx) const
        {
            RLOCK(MUTEX);

            /* Find the transaction in pool. */
            if(mapPool.count(hashTx))
                return std::get<4>(mapPool.at(hashTx));

            return false;
        }


        /* Remove a transaction from pool. */
        bool Stakepool::Remove(const uint512_t& hashTx)
        {
            RLOCK(MUTEX);

            /* Find the transaction in pool. */
            if(mapPool.count(hashTx))
            {
                TAO::Ledger::Transaction tx = std::get<0>(mapPool.at(hashTx));
                mapGenesis.erase(tx.hashGenesis);

                mapPool.erase(hashTx);

                return true;
            }

            return false;
        }


        /* Clears all transactions and metadata from the stake pool  */
        void Stakepool::Clear()
        {
            RLOCK(MUTEX);

            /* Clear all map data */
            mapPool.clear();
            mapGenesis.clear();
            mapPending.clear();

            /* Reset proofs */
            hashLastBlock = 0;
            hashProof = 0;
            nTimeBegin = 0;
            nTimeEnd = 0;

            return;
        }


        /* List coinstake transactions in stake pool. */
        bool Stakepool::List(std::vector<uint512_t> &vHashes, const uint32_t nCount)
        {
            RLOCK(MUTEX);

            /* Loop through all the transactions. */
            for(const auto& item : mapPool)
            {
                vHashes.push_back(item.first);

                /* Check count. */
                if(vHashes.size() >= nCount)
                    return true;
            }

            return vHashes.size() > 0;
        }


        /* Select a list of coinstake transactions to use from the stake pool. */
        bool Stakepool::Select(std::vector<uint512_t> &vHashes, uint64_t &nStakeTotal, uint64_t &nFeeTotal,
                               const uint64_t& nStake, const uint32_t& nCount)
        {
            RLOCK(MUTEX);

            const uint64_t nTimespan = config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET
                                                               : TAO::Ledger::TRUST_KEY_TIMESPAN;

            vHashes.clear();
            nStakeTotal = 0;
            nFeeTotal = 0;
            bool fGenesisAdded = false;

            /* If ask for more than the current pool size, just return all of them */
            if(nCount >= mapPool.size())
            {
                for(const auto& item : mapPool)
                {
                    /* Skip the coinstake created by the local node */
                    if(item.first == hashLocal)
                        continue;

                    bool fTrust = IsTrust(item.first);

                    if(fTrust || !fGenesisAdded)
                    {
                        vHashes.push_back(item.first);

                        /* Add selected stake amount to total, capped to local stake amount */
                        nStakeTotal += std::min(std::get<2>(item.second), nStake);

                        nFeeTotal += std::get<3>(item.second);

                        /* Only select at most one genesis for addition to the block */
                        if(!fTrust)
                            fGenesisAdded = true;
                    }
                }

                return vHashes.size() > 0;
            }

            std::vector<uint512_t> vAvailable;
            std::vector<double> vWeights;

            /* Loop through all the transactions to build available hash list and corresponding selection weight list. */
            for(const auto& item : mapPool)
            {
                /* Skip the coinstake created by the local node */
                if(item.first == hashLocal)
                    continue;

                vAvailable.push_back(item.first);

                /* Get block age and stake balance for coinstake in pool */
                uint64_t nBlockAge = std::get<1>(item.second);
                uint64_t nStakeBalance = std::get<2>(item.second);

                /* Genesis stake gets reduced priority for selection */
                if(!IsTrust(item.first))
                {
                    nBlockAge = std::max(nBlockAge / 10, (uint64_t)1);
                    nStakeBalance = std::max(nStakeBalance / 10, TAO::Ledger::NXS_COIN);
                }

                /* Calculate selection weight from block age and stake balance */
                double nWeightAge = log((double)nBlockAge) * (double)nBlockAge / (double)nTimespan;
                double nWeightStake = log((double)nStakeBalance / (double)TAO::Ledger::NXS_COIN);

                double nWeight = nWeightAge + nWeightStake;

                vWeights.push_back(nWeight);
            }

            /* Set up the random generator */
            std::mt19937 g(runtime::timestamp());

            /* It is possible to run out of available tx before reach nCount if skip over multiple genesis coinstakes,
             * so need to include vAvailable size check.
             */
            while((vHashes.size() < nCount) && (vHashes.size() < mapPool.size()) && (vAvailable.size() > 0))
            {
                /* Randomly select from the weighted list, with each weight defining the relative probability it is chosen.
                 * For example, if weights total to 100, an entry with weight 20 will have 20/100 = 0.2 probability it is selected.
                 */
                std::discrete_distribution<unsigned int> dist(vWeights.begin(), vWeights.end());

                uint32_t nSelection = static_cast<uint32_t>(dist(g));

                /* Add selection to the results */
                uint512_t hashSelected = vAvailable.at(nSelection);

                bool fTrust = IsTrust(hashSelected);

                if(fTrust || !fGenesisAdded)
                {
                    vHashes.push_back(hashSelected);

                    /* Add selected stake amount to total, capped to local stake amount */
                    nStakeTotal += std::min(std::get<2>(mapPool[hashSelected]), nStake);

                    nFeeTotal += std::get<3>(mapPool[hashSelected]);

                    /* Only select at most one genesis for addition to the block */
                    if(!fTrust)
                        fGenesisAdded = true;
                }

                /* Remove the chosen entry from the lists before next iteration */
                vAvailable.erase(vAvailable.begin() + nSelection);
                vWeights.erase(vWeights.begin() + nSelection);
            }

            return vHashes.size() > 0;
        }


        /*  Updates the stake pool with data for the current mining round. */
        void Stakepool::SetProofs(const uint1024_t& hashLastBlock, const uint256_t& hashProof,
                                  const uint64_t& nTimeBegin, const uint64_t& nTimeEnd)
        {
            RLOCK(MUTEX);

            /* Assign stake pool values */
            this->hashLastBlock = hashLastBlock;
            this->hashProof = hashProof;
            this->nTimeBegin = nTimeBegin;
            this->nTimeEnd = nTimeEnd;

            /* Verify any pending tx, and add matching to pool */
            for(const auto& item : mapPending)
            {
                TAO::Ledger::Transaction tx = item.second;

                /* When the proofs match and no duplicate, add the pending tx to the pool. Otherwise, ignore it. */
                if(mapGenesis.count(tx.hashGenesis) == 0 && CheckProofs(tx) && mapPool.size() < nSizeMax)
                {
                    if(!AddToPool(tx))
                        continue;

                    mapGenesis[tx.hashGenesis] = tx.GetHash();
                }
            }

            debug::log(2, FUNCTION, "Setting hashProof = ", hashProof.SubString(),
                                    " nTimeBegin = ", nTimeBegin, " nTimeEnd = ", nTimeEnd);

            mapPending.clear();

            return;
        }


        /* Retrieve the current setting for maximum number of transactions to accept into the pool. */
        uint32_t Stakepool::GetMaxSize() const
        {
            return nSizeMax;
        }


        /* Assigns the maximum number of transactions to accept into the pool. */
        void Stakepool::SetMaxSize(const uint32_t nSizeMax)
        {
            this->nSizeMax = nSizeMax;
        }


        /* Gets the size of the stake pool. */
        uint32_t Stakepool::Size()
        {
            RLOCK(MUTEX);

            return static_cast<uint32_t>(mapPool.size());
        }
    }
}
