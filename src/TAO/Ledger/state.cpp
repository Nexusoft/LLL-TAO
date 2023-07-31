/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/state.h>

#include <string>

#include <LLD/include/global.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>

#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/ambassador.h>
#include <TAO/Ledger/include/developer.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/dispatch.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/retarget.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/client.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Get the block state object. */
        bool GetLastState(BlockState &state, uint32_t nChannel)
        {
            /* Get the genesis block hash. */
            uint1024_t hashGenesis =  ChainState::Genesis();

            /* Loop back 1440 blocks. */
            while(true)
            {
                /* Return false on genesis. */
                if(state.nHeight == 0)
                    return false;

                /* Return true on channel found. */
                if(state.GetChannel() == nChannel)
                    return true;

                /* Iterate backwards. */
                state = state.Prev();
                if(!state)
                    return false;
            }

            /* If the max depth expired, return the genesis. */
            state = ChainState::tStateGenesis;

            return false;
        }


        /** Default Constructor. **/
        BlockState::BlockState()
        : Block            ( )
        , nTime            (runtime::unifiedtimestamp())
        , ssSystem         ( )
        , vtx              ( )
        , nChainTrust      (0)
        , nMoneySupply     (0)
        , nMint            (0)
        , nFees            (0)
        , nChannelHeight   (0)
        , nChannelWeight   {0, 0, 0}
        , nReleasedReserve {0, 0, 0}
        , nFeeReserve      (0)
        , hashNextBlock    (0)
        , hashCheckpoint   (0)
        {
        }


        /* Copy constructor. */
        BlockState::BlockState(const BlockState& block)
        : Block            (block)
        , nTime            (block.nTime)
        , ssSystem         (block.ssSystem)
        , vtx              (block.vtx)
        , nChainTrust      (block.nChainTrust)
        , nMoneySupply     (block.nMoneySupply)
        , nMint            (block.nMint)
        , nFees            (block.nFees)
        , nChannelHeight   (block.nChannelHeight)
        , nChannelWeight   {block.nChannelWeight[0]
                           ,block.nChannelWeight[1]
                           ,block.nChannelWeight[2]}
        , nReleasedReserve {block.nReleasedReserve[0]
                           ,block.nReleasedReserve[1]
                           ,block.nReleasedReserve[2]}
        , nFeeReserve      (block.nFeeReserve)
        , hashNextBlock    (block.hashNextBlock)
        , hashCheckpoint   (block.hashCheckpoint)
        {
        }


        /* Move constructor. */
        BlockState::BlockState(BlockState&& block) noexcept
        : Block            (std::move(block))
        , nTime            (std::move(block.nTime))
        , ssSystem         (std::move(block.ssSystem))
        , vtx              (std::move(block.vtx))
        , nChainTrust      (std::move(block.nChainTrust))
        , nMoneySupply     (std::move(block.nMoneySupply))
        , nMint            (std::move(block.nMint))
        , nFees            (std::move(block.nFees))
        , nChannelHeight   (std::move(block.nChannelHeight))
        , nChannelWeight   {std::move(block.nChannelWeight[0])
                           ,std::move(block.nChannelWeight[1])
                           ,std::move(block.nChannelWeight[2])}
        , nReleasedReserve {std::move(block.nReleasedReserve[0])
                           ,std::move(block.nReleasedReserve[1])
                           ,std::move(block.nReleasedReserve[2])}
        , nFeeReserve      (std::move(block.nFeeReserve))
        , hashNextBlock    (std::move(block.hashNextBlock))
        , hashCheckpoint   (std::move(block.hashCheckpoint))
        {
        }


        /* Copy assignment. */
        BlockState& BlockState::operator=(const BlockState& block)
        {
            nVersion            = block.nVersion;
            hashPrevBlock       = block.hashPrevBlock;
            hashMerkleRoot      = block.hashMerkleRoot;
            nChannel            = block.nChannel;
            nHeight             = block.nHeight;
            nBits               = block.nBits;
            nNonce              = block.nNonce;
            vOffsets            = block.vOffsets;
            vchBlockSig         = block.vchBlockSig;
            vMissing            = block.vMissing;
            hashMissing         = block.hashMissing;
            fConflicted         = block.fConflicted;

            nTime               = block.nTime;
            ssSystem            = block.ssSystem;
            vtx                 = block.vtx;
            nChainTrust         = block.nChainTrust;
            nMoneySupply        = block.nMoneySupply;
            nMint               = block.nMint;
            nFees               = block.nFees;
            nChannelHeight      = block.nChannelHeight;
            nChannelWeight[0]   = block.nChannelWeight[0];
            nChannelWeight[1]   = block.nChannelWeight[1];
            nChannelWeight[2]   = block.nChannelWeight[2];
            nReleasedReserve[0] = block.nReleasedReserve[0];
            nReleasedReserve[1] = block.nReleasedReserve[1];
            nReleasedReserve[2] = block.nReleasedReserve[2];
            nFeeReserve         = block.nFeeReserve;
            hashNextBlock       = block.hashNextBlock;
            hashCheckpoint      = block.hashCheckpoint;

            return *this;
        }


        /* Move assignment. */
        BlockState& BlockState::operator=(BlockState&& block) noexcept
        {
            nVersion            = std::move(block.nVersion);
            hashPrevBlock       = std::move(block.hashPrevBlock);
            hashMerkleRoot      = std::move(block.hashMerkleRoot);
            nChannel            = std::move(block.nChannel);
            nHeight             = std::move(block.nHeight);
            nBits               = std::move(block.nBits);
            nNonce              = std::move(block.nNonce);
            vOffsets            = std::move(block.vOffsets);
            vchBlockSig         = std::move(block.vchBlockSig);
            vMissing            = std::move(block.vMissing);
            hashMissing         = std::move(block.hashMissing);
            fConflicted         = std::move(block.fConflicted);

            nTime               = std::move(block.nTime);
            ssSystem            = std::move(block.ssSystem);
            vtx                 = std::move(block.vtx);
            nChainTrust         = std::move(block.nChainTrust);
            nMoneySupply        = std::move(block.nMoneySupply);
            nMint               = std::move(block.nMint);
            nFees               = std::move(block.nFees);
            nChannelHeight      = std::move(block.nChannelHeight);
            nChannelWeight[0]   = std::move(block.nChannelWeight[0]);
            nChannelWeight[1]   = std::move(block.nChannelWeight[1]);
            nChannelWeight[2]   = std::move(block.nChannelWeight[2]);
            nReleasedReserve[0] = std::move(block.nReleasedReserve[0]);
            nReleasedReserve[1] = std::move(block.nReleasedReserve[1]);
            nReleasedReserve[2] = std::move(block.nReleasedReserve[2]);
            nFeeReserve         = std::move(block.nFeeReserve);
            hashNextBlock       = std::move(block.hashNextBlock);
            hashCheckpoint      = std::move(block.hashCheckpoint);

            return *this;
        }


        /* Copy constructor. */
        BlockState::BlockState(const ClientBlock& block)
        : Block            (block)
        , nTime            (block.nTime)
        , ssSystem         (block.ssSystem)
        , vtx              ( )
        , nChainTrust      (0)
        , nMoneySupply     (block.nMoneySupply)
        , nMint            (0)
        , nFees            (0)
        , nChannelHeight   (block.nChannelHeight)
        , nChannelWeight   {block.nChannelWeight[0]
                           ,block.nChannelWeight[1]
                           ,block.nChannelWeight[2]}
        , nReleasedReserve {block.nReleasedReserve[0]
                           ,block.nReleasedReserve[1]
                           ,block.nReleasedReserve[2]}
        , nFeeReserve      (0)
        , hashNextBlock    (block.hashNextBlock)
        , hashCheckpoint   (0)
        {
        }


        /* Move constructor. */
        BlockState::BlockState(ClientBlock&& block) noexcept
        : Block            (std::move(block))
        , nTime            (std::move(block.nTime))
        , ssSystem         (std::move(block.ssSystem))
        , vtx              ( )
        , nChainTrust      (0)
        , nMoneySupply     (std::move(block.nMoneySupply))
        , nMint            (0)
        , nFees            (0)
        , nChannelHeight   (std::move(block.nChannelHeight))
        , nChannelWeight   {std::move(block.nChannelWeight[0])
                           ,std::move(block.nChannelWeight[1])
                           ,std::move(block.nChannelWeight[2])}
        , nReleasedReserve {std::move(block.nReleasedReserve[0])
                           ,std::move(block.nReleasedReserve[1])
                           ,std::move(block.nReleasedReserve[2])}
        , nFeeReserve      (0)
        , hashNextBlock    (std::move(block.hashNextBlock))
        , hashCheckpoint   (0)
        {
        }


        /* Copy assignment. */
        BlockState& BlockState::operator=(const ClientBlock& block)
        {
            nVersion            = block.nVersion;
            hashPrevBlock       = block.hashPrevBlock;
            hashMerkleRoot      = block.hashMerkleRoot;
            nChannel            = block.nChannel;
            nHeight             = block.nHeight;
            nBits               = block.nBits;
            nNonce              = block.nNonce;
            vOffsets            = block.vOffsets;
            vchBlockSig         = block.vchBlockSig;
            vMissing            = block.vMissing;
            hashMissing         = block.hashMissing;
            fConflicted         = block.fConflicted;

            nTime               = block.nTime;
            ssSystem            = block.ssSystem;
            nMoneySupply        = block.nMoneySupply;
            nChannelHeight      = block.nChannelHeight;
            nChannelWeight[0]   = block.nChannelWeight[0];
            nChannelWeight[1]   = block.nChannelWeight[1];
            nChannelWeight[2]   = block.nChannelWeight[2];
            nReleasedReserve[0] = block.nReleasedReserve[0];
            nReleasedReserve[1] = block.nReleasedReserve[1];
            nReleasedReserve[2] = block.nReleasedReserve[2];
            hashNextBlock       = block.hashNextBlock;

            return *this;
        }


        /* Move assignment. */
        BlockState& BlockState::operator=(ClientBlock&& block) noexcept
        {
            nVersion            = std::move(block.nVersion);
            hashPrevBlock       = std::move(block.hashPrevBlock);
            hashMerkleRoot      = std::move(block.hashMerkleRoot);
            nChannel            = std::move(block.nChannel);
            nHeight             = std::move(block.nHeight);
            nBits               = std::move(block.nBits);
            nNonce              = std::move(block.nNonce);
            vOffsets            = std::move(block.vOffsets);
            vchBlockSig         = std::move(block.vchBlockSig);
            vMissing            = std::move(block.vMissing);
            hashMissing         = std::move(block.hashMissing);
            fConflicted         = std::move(block.fConflicted);

            nTime               = std::move(block.nTime);
            ssSystem            = std::move(block.ssSystem);
            nMoneySupply        = std::move(block.nMoneySupply);
            nChannelHeight      = std::move(block.nChannelHeight);
            nChannelWeight[0]   = std::move(block.nChannelWeight[0]);
            nChannelWeight[1]   = std::move(block.nChannelWeight[1]);
            nChannelWeight[2]   = std::move(block.nChannelWeight[2]);
            nReleasedReserve[0] = std::move(block.nReleasedReserve[0]);
            nReleasedReserve[1] = std::move(block.nReleasedReserve[1]);
            nReleasedReserve[2] = std::move(block.nReleasedReserve[2]);
            hashNextBlock       = std::move(block.hashNextBlock);

            return *this;
        }


        /** Default Destructor **/
        BlockState::~BlockState()
        {
        }


        /* Default Constructor. */
        BlockState::BlockState(const TritiumBlock& block)
        : Block            (block)
        , nTime            (block.nTime)
        , ssSystem         (block.ssSystem)
        , vtx              (block.vtx)
        , nChainTrust      (0)
        , nMoneySupply     (0)
        , nMint            (0)
        , nFees            (0)
        , nChannelHeight   (0)
        , nChannelWeight   {0, 0, 0}
        , nReleasedReserve {0, 0, 0}
        , nFeeReserve      (0)
        , hashNextBlock    (0)
        , hashCheckpoint   (0)
        {
            /* Set producer to be last transaction. */
            vtx.push_back(std::make_pair(TRANSACTION::TRITIUM, block.producer.GetHash()));

            /* Check that sizes are expected. */
            if(vtx.size() != block.vtx.size() + 1)
               throw debug::exception(FUNCTION, "tritium block to state incorrect sizes");
        }


        /* Construct a block state from a legacy block. */
        BlockState::BlockState(const Legacy::LegacyBlock& block)
        : Block            (block)
        , nTime            (block.nTime)
        , ssSystem         ( )
        , vtx              ( )
        , nChainTrust      (0)
        , nMoneySupply     (0)
        , nMint            (0)
        , nFees            (0)
        , nChannelHeight   (0)
        , nChannelWeight   {0, 0, 0}
        , nReleasedReserve {0, 0, 0}
        , nFeeReserve      (0)
        , hashNextBlock    (0)
        , hashCheckpoint   (0)
        {
            for(const auto& tx : block.vtx)
                vtx.push_back(std::make_pair(TRANSACTION::LEGACY, tx.GetHash()));

            if(vtx.size() != block.vtx.size())
                throw debug::exception(FUNCTION, "legacy block to state incorrect sizes");
        }


        /** Equivalence checking **/
        bool BlockState::operator==(const BlockState& state) const
        {
            return
            (
                nVersion            == state.nVersion       &&
                hashPrevBlock       == state.hashPrevBlock  &&
                hashMerkleRoot      == state.hashMerkleRoot &&
                nChannel            == state.nChannel       &&
                nHeight             == state.nHeight        &&
                nBits               == state.nBits          &&
                nNonce              == state.nNonce         &&
                nTime               == state.nTime
            );
        }


        /** Equivalence checking **/
        bool BlockState::operator!=(const BlockState& state) const
        {
            return
            (
                nVersion            != state.nVersion       ||
                hashPrevBlock       != state.hashPrevBlock  ||
                hashMerkleRoot      != state.hashMerkleRoot ||
                nChannel            != state.nChannel       ||
                nHeight             != state.nHeight        ||
                nBits               != state.nBits          ||
                nNonce              != state.nNonce         ||
                nTime               != state.nTime
            );
        }


        /** Not operator overloading. **/
        bool BlockState::operator!(void) const
        {
            return IsNull();
        }


        /* Return the Block's current UNIX timestamp. */
        uint64_t BlockState::GetBlockTime() const
        {
            return nTime;
        }


        /* Get the previous block state in chain. */
        BlockState BlockState::Prev() const
        {
            /* Check for genesis. */
            BlockState state;
            if(hashPrevBlock == 0)
                return state;

            /* Read the previous block from ledger. */
            if(LLD::Ledger->ReadBlock(hashPrevBlock, state))
                return state;

            return BlockState();
        }


        /* Get the next block state in chain. */
        BlockState BlockState::Next() const
        {
            /* Check for genesis. */
            BlockState state;
            if(hashNextBlock == 0)
                return state;

            /* Read next block from the ledger. */
            if(LLD::Ledger->ReadBlock(hashNextBlock, state))
                return state;

            return BlockState();
        }


        /* Accept a block state into chain. */
        bool BlockState::Index()
        {
            /* Runtime calculations. */
            runtime::timer timer;
            timer.Start();

            /* Read leger DB for previous block. */
            BlockState statePrev = Prev();
            if(!statePrev)
                return debug::error(FUNCTION, hashPrevBlock.SubString(), " block state not found");

            /* Compute the Chain Weight. */
            for(uint32_t n = 0; n < 3; ++n)
                nChannelWeight[n] = statePrev.nChannelWeight[n];

            /* Find the last block of this channel. */
            BlockState stateLast = statePrev;
            GetLastState(stateLast, nChannel);

            /* Compute the Channel Height. */
            nChannelHeight = stateLast.nChannelHeight + 1;

            /* Carry over the fee reserves from last block. */
            nFeeReserve = stateLast.nFeeReserve;

            /* Compute the Released Reserves. */
            if(IsProofOfWork())
            {
                /* Calculate the coinbase rewards from the coinbase transaction. */
                uint64_t nCoinbaseRewards[3] = { 0, 0, 0 };
                if(nVersion < 7) //legacy blocks
                {
                    /* Get the coinbase from the memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(vtx[0].second, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "cannot get legacy coinbase tx");

                    /* Double check for coinbase. */
                    if(!tx.IsCoinBase())
                        return debug::error(FUNCTION, "first tx must be coinbase");

                    /* Get the size of the coinbase. */
                    uint32_t nSize = tx.vout.size();

                    /* Add up the Miner Rewards from Coinbase Tx Outputs. */
                    for(uint32_t n = 0; n < nSize - 2; ++n)
                        nCoinbaseRewards[0] += tx.vout[n].nValue;

                    /* Get the ambassador and developer coinbase. */
                    nCoinbaseRewards[1] = tx.vout[nSize - 2].nValue;
                    nCoinbaseRewards[2] = tx.vout[nSize - 1].nValue;
                }
                else
                {
                    /* Get the coinbase from the memory pool. */
                    Transaction disk;
                    if(!LLD::Ledger->ReadTx(vtx.back().second, disk, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "cannot get ledger coinbase tx");

                    /* Get a const reference of transaction. */
                    const Transaction& tx = disk;
                    if(!tx.IsCoinBase())
                        return debug::error(FUNCTION, "last tx must be producer");

                    /* Get starting index. */
                    uint32_t nSize = tx.Size();

                    /* Check for interval. */
                    bool fAmbassador = false;
                    if(stateLast.nChannelHeight %
                        (config::fTestNet.load() ? AMBASSADOR_PAYOUT_THRESHOLD_TESTNET : AMBASSADOR_PAYOUT_THRESHOLD) == 0)
                    {
                        /* Get the total in reserves. */
                        int64_t nBalance = stateLast.nReleasedReserve[1] - (33 * NXS_COIN); //leave 33 coins in the reserve
                        if(nBalance > 0)
                        {
                            /* Set ambassador as active. */
                            fAmbassador = true;

                            /* Reduce size. */
                            nSize -= Ambassador(nVersion).size();
                        }
                    }

                    /* Check for interval. */
                    bool fDeveloper = false;
                    if(stateLast.nChannelHeight %
                        (config::fTestNet.load() ? DEVELOPER_PAYOUT_THRESHOLD_TESTNET : DEVELOPER_PAYOUT_THRESHOLD) == 0)
                    {
                        /* Get the total in reserves. */
                        int64_t nBalance = stateLast.nReleasedReserve[2] - (3 * NXS_COIN); //leave 3 coins in the reserve
                        if(nBalance > 0)
                        {
                            /* Set developer as active. */
                            fDeveloper = true;

                            /* Reduce size. */
                            nSize -= Developer(nVersion).size();
                        }
                    }

                    /* Get starting contract. */
                    uint32_t nContract = nSize;

                    /* Check for ambassador rewards. */
                    if(fAmbassador)
                    {
                        /* Get the total in reserves. */
                        int64_t nBalance = stateLast.nReleasedReserve[1] - (33 * NXS_COIN); //leave 33 coins in the reserve

                        /* Loop through all ambassador sigchains. */
                        for(auto it = Ambassador(nVersion).begin(); it != Ambassador(nVersion).end(); ++it)
                        {
                            /* Seek to Genesis */
                            tx[nContract].Seek(1, Operation::Contract::OPERATIONS, STREAM::BEGIN);

                            /* Get the genesis.. */
                            Genesis genesis;
                            tx[nContract] >> genesis;
                            if(!genesis.IsValid())
                                return debug::error(FUNCTION, "invalid ambassador genesis-id ", genesis.SubString());

                            /* Check for match. */
                            if(genesis != it->first)
                                return debug::error(FUNCTION, "ambassador genesis mismatch ", genesis.SubString());

                            /* The total to be credited. */
                            const uint64_t nCredit = (nBalance * it->second.second) / 1000;

                            /* Check the value */
                            uint64_t nValue = 0;
                            tx[nContract] >> nValue;
                            if(nValue != nCredit)
                                return debug::error(FUNCTION, "invalid ambassador rewards=", nValue, " expected=", nCredit);

                            /* Iterate contract. */
                            ++nContract;

                            /* Update coinbase rewards. */
                            nCoinbaseRewards[1] += nValue;

                            /* Log the genesis. */
                            debug::log(2, "AMBASSADOR GENESIS ", genesis.ToString());
                        }
                    }

                    /* Check for developer rewards. */
                    if(fDeveloper)
                    {
                        /* Get the total in reserves. */
                        int64_t nBalance = stateLast.nReleasedReserve[2] - (3 * NXS_COIN); //leave 3 coins in the reserve

                        /* Loop through the embassy sigchains. */
                        for(auto it = Developer(nVersion).begin(); it != Developer(nVersion).end(); ++it)
                        {
                            /* Seek to Genesis */
                            tx[nContract].Seek(1, Operation::Contract::OPERATIONS, STREAM::BEGIN);

                            /* Get the genesis.. */
                            Genesis genesis;
                            tx[nContract] >> genesis;
                            if(!genesis.IsValid())
                                return debug::error(FUNCTION, "invalid developer genesis-id ", genesis.SubString());

                            /* Check for match. */
                            if(genesis != it->first)
                                return debug::error(FUNCTION, "developer genesis mismatch ", genesis.SubString());

                            /* The total to be credited. */
                            uint64_t nCredit = (nBalance * it->second.second) / 1000;

                            /* Check the value */
                            uint64_t nValue = 0;
                            tx[nContract] >> nValue;
                            if(nValue != nCredit)
                                return debug::error(FUNCTION, "invalid developer rewards=", nValue, " expected=", nCredit);

                            /* Iterate contract. */
                            ++nContract;

                            /* Update coinbase rewards. */
                            nCoinbaseRewards[2] += nValue;

                            /* Log the genesis. */
                            debug::log(2, "DEVELOPER GENESIS ", genesis.ToString());
                        }
                    }


                    /* Check coinbase rewards to expected rewards. */
                    for(uint32_t n = 0; n < nSize; ++n)
                    {
                        /* Seek to Genesis */
                        tx[n].Seek(1, Operation::Contract::OPERATIONS, STREAM::BEGIN);

                        /* Get the genesis.. */
                        Genesis genesis;
                        tx[n] >> genesis;
                        if(!genesis.IsValid())
                            return debug::error(FUNCTION, "invalid coinbase genesis-id ", genesis.SubString());

                        /* Get the reward. */
                        uint64_t nValue = 0;
                        tx[n] >> nValue;

                        /* Update the coinbase rewards. */
                        nCoinbaseRewards[0] += nValue;
                    }

                    /* Check the coinbase rewards. */
                    uint64_t nExpected = GetCoinbaseReward(statePrev, nChannel, 0);
                    if(nCoinbaseRewards[0] != nExpected)
                        return debug::error(FUNCTION, "miner reward=", nCoinbaseRewards[0], " expected=", nExpected);
                }

                /* Calculate the new reserve amounts. */
                for(int nType = 0; nType < 3; ++nType)
                {
                    /* Calculate the Reserves from the Previous Block in Channel's reserve and new Release. */
                    uint64_t nReserve  = stateLast.nReleasedReserve[nType] + GetReleasedReserve(*this, GetChannel(), nType);

                    /* Disable Reserves from going below 0. */
                    if(nVersion != 2 && nCoinbaseRewards[nType] >= nReserve)
                        return debug::error(FUNCTION, "out of reserve limits");

                    /* Check coinbase rewards. */
                    nReleasedReserve[nType] =  (nReserve - nCoinbaseRewards[nType]);

                    /* Verbose output. */
                    debug::log(2, "Reserve Balance ", nType, " | ",
                        std::fixed, nReleasedReserve[nType] / (double)TAO::Ledger::NXS_COIN,
                        " Nexus | Released ",
                        std::fixed, (nReserve - stateLast.nReleasedReserve[nType]) / (double)TAO::Ledger::NXS_COIN);

                    /* Update the mint values. */
                    nMint += nCoinbaseRewards[nType];
                }
            }

            /* Log the accumulated fee reserves. */
            debug::log(2, "Fee Reserves | ", std::fixed, nFeeReserve / double(TAO::Ledger::NXS_COIN));

            /* Add the Pending Checkpoint into the Blockchain. */
            if(IsNewTimespan(statePrev))
            {
                /* Set new checkpoint hash. */
                hashCheckpoint = GetHash();

                if(config::nVerbose >= 1)
                    debug::log(1, "===== New Pending Checkpoint Hash = ", hashCheckpoint.SubString(15));
            }
            else
            {
                /* Continue the old checkpoint through chain. */
                hashCheckpoint = statePrev.hashCheckpoint;

                if(config::nVerbose >= 1)
                    debug::log(1, "===== Pending Checkpoint Hash = ", hashCheckpoint.SubString(15));
            }

            /* Add new weights for this channel. */
            if(!IsHybrid())
                nChannelWeight[nChannel] += Weight();

            /* Compute the chain trust. */
            nChainTrust = statePrev.nChainTrust + Trust();

            /* Write the block to disk. */
            if(!LLD::Ledger->WriteBlock(GetHash(), *this))
                return debug::error(FUNCTION, "block state failed to write");

            /* Signal to set the best chain. */
            if(nVersion >= 7 && !IsHybrid())
            {
                /* Set the chain trust. */
                uint8_t nEquals  = 0;
                uint8_t nGreater = 0;

                /* Check to best state. */
                for(uint32_t n = 0; n < 3; ++n)
                {
                    /* Check each weight. */
                    if(nChannelWeight[n] == ChainState::tStateBest.load().nChannelWeight[n])
                        ++nEquals;

                    /* Check each weight. */
                    if(nChannelWeight[n] > ChainState::tStateBest.load().nChannelWeight[n])
                        ++nGreater;
                }

                /* Check for better height if it is a battle between two channels. */
                if(nHeight > ChainState::nBestHeight.load() + 1 && (nEquals == 1 && nGreater == 1))
                    ++nGreater;

                /* Check for conflicted blocks. */
                if(fConflicted)
                {
                    debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "CONFLICTED BLOCK: ", ANSI_COLOR_RESET, GetHash().SubString());

                    return true;
                }

                /* Handle single channel having higher weight. */
                else if((nEquals == 2 && nGreater == 1) || nGreater > 1)
                {
                    /* Log the weights. */
                    debug::log(2, FUNCTION, "WEIGHTS [", uint32_t(nGreater), "]",
                        " Prime ", nChannelWeight[1].Get64(),
                        " Hash ",  nChannelWeight[2].Get64(),
                        " Stake ", nChannelWeight[0].Get64());

                    /* Set the best chain. */
                    if(!SetBest())
                        return debug::error(FUNCTION, "failed to set best chain");
                }
            }
            else if(nChainTrust > ChainState::nBestChainTrust.load())
            {
                /* Check for conflicted blocks. */
                if(fConflicted)
                {
                    debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "CONFLICTED BLOCK: ", ANSI_COLOR_RESET, GetHash().SubString());

                    return true;
                }

                /* Attempt to set the best chain. */
                if(!SetBest())
                    return debug::error(FUNCTION, "failed to set best chain");
            }

            /* Debug output. */
            debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, "ACCEPTED");

            return true;
        }


        static uint32_t nTotalContracts = 0;
        static runtime::stopwatch swContract;

        static uint32_t nTotalInputs = 0;
        static runtime::stopwatch swScript;

        bool BlockState::SetBest()
        {
            /* Reset timers for meters. */
            swContract.reset();
            swScript.reset();

            /* Get the hash. */
            uint1024_t hash = GetHash();

            /* Watch for genesis. */
            if(!ChainState::tStateGenesis)
            {
                /* Write the best chain pointer. */
                if(!LLD::Ledger->WriteBestChain(hash))
                    return debug::error(FUNCTION, "failed to write best chain");

                /* Write the block to disk. */
                if(!LLD::Ledger->WriteBlock(hash, *this))
                    return debug::error(FUNCTION, "block state already exists");

                /* Set the genesis block. */
                ChainState::tStateGenesis = *this;
            }
            else
            {
                /* Get initial block states. */
                BlockState fork   = ChainState::tStateBest.load();
                BlockState longer = *this;

                /* Get the blocks to connect and disconnect. */
                std::vector<BlockState> vDisconnect;
                std::vector<BlockState> vConnect;
                while(fork != longer)
                {
                    /* Find the root block in common. */
                    while(longer.nHeight > fork.nHeight)
                    {
                        /* Add to connect queue. */
                        vConnect.push_back(longer);

                        /* Iterate backwards in chain. */
                        longer = longer.Prev();
                        if(!longer)
                            return debug::error(FUNCTION, "failed to find longer ancestor block");
                    }

                    /* Break if found. */
                    if(fork == longer)
                        break;

                    /* Iterate backwards to find fork. */
                    vDisconnect.push_back(fork);

                    /* Iterate to previous block. */
                    fork = fork.Prev();
                    if(!fork)
                        return debug::error(FUNCTION, "failed to find ancestor fork block");
                }

                /* Log if there are blocks to disconnect. */
                if(vDisconnect.size() > 0)
                {
                    debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "REORGANIZE:", ANSI_COLOR_RESET,
                        " Disconnect ", vDisconnect.size(), " blocks; ", fork.GetHash().SubString(),
                        "..",  ChainState::tStateBest.load().GetHash().SubString());

                    /* Keep this in vDisconnect check, or it will print every block, but only print on reorg if have at least 1 */
                    if(vConnect.size() > 0)
                        debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "REORGANIZE:", ANSI_COLOR_RESET,
                            " Connect ", vConnect.size(), " blocks; ", fork.GetHash().SubString(),
                            "..", hash.SubString());
                }

                /* Keep track of mempool transactions to delete. */
                std::vector<std::pair<uint8_t, uint512_t>> vResurrect;

                /* Disconnect given blocks. */
                for(auto& state : vDisconnect)
                {
                    /* Output the block state if flagged. */
                    if(config::GetBoolArg("-printstate"))
                        debug::log(0, state.ToString(debug::flags::header | debug::flags::tx));

                    /* Disconnect the block. */
                    if(!state.Disconnect())
                        return debug::error(FUNCTION, "failed to disconnect ", state.GetHash().SubString());

                    /* Erase block if not connecting anything. */
                    if(vConnect.empty())
                    {
                        LLD::Ledger->EraseBlock(state.GetHash());
                        //LLD::Ledger->EraseIndex(state.nHeight);
                    }

                    /* Resurrect transactions that were disconnected. */
                    vResurrect.insert(vResurrect.end(), state.vtx.rbegin(), state.vtx.rend());
                }

                /* Keep track of mempool transactions to delete. */
                std::vector<std::pair<uint8_t, uint512_t>> vDelete;

                /* Reverse the blocks to connect to connect in ascending height. */
                for(auto state = vConnect.rbegin(); state != vConnect.rend(); ++state)
                {
                    /* Output the block state if flagged. */
                    if(config::GetBoolArg("-printstate"))
                        debug::log(0, state->ToString(debug::flags::header | debug::flags::tx));

                    /* Connect the block. */
                    if(!state->Connect())
                        return debug::error(FUNCTION, "failed to connect ", state->GetHash().SubString());

                    /* Harden a checkpoint if there is any. */
                    #ifndef UNIT_TESTS
                    HardenCheckpoint(Prev());
                    #endif

                    /* Insert into delete queue. */
                    vDelete.insert(vDelete.end(), state->vtx.begin(), state->vtx.end());
                }

                /* Reverse the transaction to connect to connect in ascending height. */
                for(auto proof = vResurrect.rbegin(); proof != vResurrect.rend(); ++proof)
                {
                    /* Check for tritium transactions. */
                    if(proof->first == TRANSACTION::TRITIUM)
                    {
                        /* Special case for deleting blocks, delete tx as well. */
                        if(vConnect.empty())
                        {
                            /* Make sure the transaction is not on disk. */
                            if(!LLD::Ledger->EraseTx(proof->second))
                                return debug::error(FUNCTION, "transaction not on disk");
                        }
                        else
                        {
                            /* Make sure the transaction is on disk. */
                            TAO::Ledger::Transaction tx;
                            if(!LLD::Ledger->ReadTx(proof->second, tx))
                                return debug::error(FUNCTION, "transaction not on disk");

                            /* Check for producer transaction. */
                            if(tx.IsCoinBase() || tx.IsCoinStake())
                                continue;

                            /* Add back into memory pool. */
                            mempool.Accept(tx);

                            if(config::nVerbose >= 3)
                                tx.print();
                        }
                    }
                    else if(proof->first == TRANSACTION::LEGACY)
                    {
                        /* Special case for deleting blocks, delete tx as well. */
                        if(vConnect.empty())
                        {
                            /* Make sure the transaction is not on disk. */
                            if(!LLD::Legacy->EraseTx(proof->second))
                                return debug::error(FUNCTION, "transaction not on disk");
                        }
                        else
                        {
                            /* Make sure the transaction is on disk. */
                            Legacy::Transaction tx;
                            if(!LLD::Legacy->ReadTx(proof->second, tx))
                                return debug::error(FUNCTION, "transaction not on disk");

                            /* Check for producer transaction. */
                            if(tx.IsCoinBase() || tx.IsCoinStake())
                                continue;

                            /* Add back into memory pool. */
                            mempool.Accept(tx);

                            if(config::nVerbose >= 3)
                                tx.print();
                        }
                    }
                }

                /* Delete from mempool. */
                for(const auto& proof : vDelete)
                    mempool.Remove(proof.second);

                /* Debug output about the best chain. */
                uint64_t nElapsed = (GetBlockTime() - ChainState::tStateBest.load().GetBlockTime());
                uint64_t nContractTime = swContract.ElapsedMicroseconds();
                uint64_t nInputsTime   = swScript.ElapsedMicroseconds();

                /* Only output best chain data when not syncing. */
                if(config::nVerbose >= TAO::Ledger::ChainState::Synchronizing() ? 1 : 0)
                    debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION,
                        "New Best Block hash=", hash.SubString(),
                        " height=", nHeight,
                        " trust=", nChainTrust,
                        " tx=", vtx.size(),
                        " [", (nElapsed == 0 ? 0 : double(nTotalContracts / nElapsed)), " tx/s]"
                        " [processed at ", (nTotalContracts * 1000000.0) / (nContractTime + 1), " contract/s",
                        " | ", (nTotalInputs * 1000000.0) / (nInputsTime + 1), " script/s]",
                        " [", std::setw(3), (::GetSerializeSize(*this, SER_LLD, nVersion) / 1024.0), " kb]");

                /* Set the best chain variables. */
                ChainState::tStateBest          = *this; //XXX: we are not getting all the data from connect, consider using pointer
                ChainState::hashBestChain      = hash;
                ChainState::nBestChainTrust    = nChainTrust;
                ChainState::nBestHeight        = nHeight;

                /* Write the best chain pointer. */
                if(!LLD::Ledger->WriteBestChain(hash))
                    return debug::error(FUNCTION, "failed to write best chain");

                /* Reset contract meters. */
                nTotalContracts = 0;
                nTotalInputs    = 0;

                /* Broadcast the block to nodes if not synchronizing. */
                #ifndef UNIT_TESTS
                if(!ChainState::Synchronizing())
                    Dispatch::Instance().PushRelay(ChainState::hashBestChain.load());
                #endif
            }

            return true;
        }


        /** Connect a block state into chain. **/
        bool BlockState::Connect()
        {
            /* Get a copy of our block hash. */
            const uint1024_t hashBlock = GetHash();

            /* Reset the transaction fees. */
            nFees = 0;

            debug::log(3, "BLOCK BEGIN-------------------------------------");

            /* Check through all the transactions. */
            for(const auto& proof : vtx)
            {
                /* Get the transaction hash. */
                const uint512_t& hash = proof.second;

                /* Only work on tritium transactions for now. */
                if(proof.first == TRANSACTION::TRITIUM)
                {
                    /* Start the contracts stopwatch. */
                    swContract.start();

                    /* Check for existing indexes. */
                    if(LLD::Ledger->HasIndex(hash))
                        return debug::error(FUNCTION, "transaction overwrites not allowed");

                    /* Make sure the transaction is on disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction not on disk");

                    if(config::nVerbose >= 3)
                        tx.print();

                    /* Check the ledger rules for sigchain at end. */
                    if(!tx.IsFirst())
                    {
                        /* Check for the last hash. */
                        uint512_t hashLast = 0;
                        if(!LLD::Ledger->ReadLast(tx.hashGenesis, hashLast))
                            return debug::error(FUNCTION, "failed to read last on non-genesis");

                        /* Check that the last transaction is correct. */
                        if(tx.hashPrevTx != hashLast)
                            return debug::error(FUNCTION, "last hash mismatch ", VARIABLE(hashLast.SubString()));
                    }

                    /* Verify the Ledger Pre-States. */
                    if(!tx.Verify(FLAGS::BLOCK)) //NOTE: double checking this for now in post-processing
                        return false;

                    /* Connect the transaction. */
                    if(!tx.Connect(FLAGS::BLOCK, this))
                        return debug::error(FUNCTION, "failed to connect transaction");

                    /* Add legacy transactions to the wallet where appropriate */
                    #ifndef NO_WALLET
                    Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, *this, true);
                    #endif

                    /* Accumulate the fees. */
                    nFees += tx.Fees();

                    /* If tx is coinstake, also write the last stake. */
                    if(tx.IsCoinStake())
                    {
                        /* Check the coinstake trust and reward values. */
                        if(!tx.CheckTrust(this))
                            return debug::error(FUNCTION, "trust checks failed");

                        /* Write the last stake value into the database. */
                        if(!LLD::Ledger->WriteStake(tx.hashGenesis, hash))
                            return debug::error(FUNCTION, "failed to write last stake");

                        /* If local database has a stake change request not marked as processed, update it.
                         *
                         * This updates a request that was reset because coinstake was disconnected and now is reconnected, such
                         * as if execute a forkblocks and re-sync.
                         *
                         * It also handles marking stake changes in stake pool as processed, because the block is likely
                         * mined by another node on the network, and stake change must be marked when that block is received.
                         */
                        StakeChange tRequest;
                        if(LLD::Local->ReadStakeChange(tx.hashGenesis, tRequest))
                        {
                            /* Update stake change request if not processed. */
                            if(!tRequest.fProcessed)
                            {
                                /* Mark as processed. */
                                tRequest.fProcessed = true;
                                tRequest.hashTx     = hash;

                                /* Erase if we can't update it. */
                                if(!LLD::Local->WriteStakeChange(tx.hashGenesis, tRequest))
                                    LLD::Local->EraseStakeChange(tx.hashGenesis);
                            }
                        }
                    }

                    /* Keep track of total contracts processed. */
                    nTotalContracts += tx.Size();
                    swContract.stop();
                }
                else if(proof.first == TRANSACTION::LEGACY)
                {
                    /* Start the script stopwatch. */
                    swScript.start();

                    /* Check for existing indexes. */
                    if(LLD::Ledger->HasIndex(hash))
                        return debug::error(FUNCTION, "transaction overwrites not allowed");

                    /* Make sure the transaction isn't on disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction not on disk");

                    /* Fetch the inputs. */
                    std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
                    if(!tx.FetchInputs(inputs))
                        return debug::error(FUNCTION, "failed to fetch the inputs");

                    /* Connect the inputs. */
                    if(!tx.Connect(inputs, *this, FLAGS::BLOCK))
                        return debug::error(FUNCTION, "failed to connect inputs");

                    /* Add legacy transactions to the wallet where appropriate */
                    #ifndef NO_WALLET
                    Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, *this, true);
                    #endif

                    /* Keep track of total inputs processed. */
                    nTotalInputs += tx.vin.size();
                    swScript.stop();
                }
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");

                /* Write the indexing entries. */
                LLD::Ledger->IndexBlock(proof.second, hashBlock);

                /* Push to our logical indexing in API. */
                if(nTime > NEXUS_TRITIUM_TIMELOCK)
                    TAO::API::Indexing::PushTransaction(hash);
            }

            if(config::nVerbose >= 3)
                debug::log(3, "Block Height ", nHeight, " Hash ", hashBlock.SubString());


            debug::log(3, "BLOCK END-------------------------------------");

            /* Update the previous state's next pointer. */
            BlockState prev = Prev();

            /* Update the money supply. */
            nMoneySupply = (prev.IsNull() ? 0 : prev.nMoneySupply) + nMint;

            /* Update the fee reserves. */
            nFeeReserve += nFees;

            /* Log how much was generated / destroyed. */
            debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, nMint > 0 ? "Generated " : "Destroyed ", std::fixed, (double)nMint / TAO::Ledger::NXS_COIN, " Nexus | Money Supply ", std::fixed, (double)nMoneySupply / TAO::Ledger::NXS_COIN);

            /* Write the updated block state to disk. */
            if(!LLD::Ledger->WriteBlock(hashBlock, *this))
                return debug::error(FUNCTION, "failed to update block state");

            /* Index the block by height if enabled. */
            if(config::GetBoolArg("-indexheight"))
                LLD::Ledger->IndexBlock(nHeight, hashBlock);

            /* Update chain pointer for previous block. */
            if(!prev.IsNull())
            {
                prev.hashNextBlock = hashBlock;
                if(!LLD::Ledger->WriteBlock(hashPrevBlock, prev))
                    return debug::error(FUNCTION, "failed to update previous block state");

                /* If we just updated hashNextBlock for genesis block, update the in-memory genesis */
                if(hashPrevBlock == ChainState::Genesis())
                    ChainState::tStateGenesis = prev;
            }

            return true;
        }


        /** Disconnect a block state from the chain. **/
        bool BlockState::Disconnect()
        {
            /* Disconnect the transactions in reverse order to preserve sigchain ordering. */
            for(auto proof = vtx.rbegin(); proof != vtx.rend(); ++proof)
            {
                /* Only work on tritium transactions for now. */
                if(proof->first == TRANSACTION::TRITIUM)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof->second;

                    /* Read from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction is not on disk");

                    /* Disconnect the transaction. */
                    if(!tx.Disconnect())
                        return debug::error(FUNCTION, "failed to disconnect transaction");

                    /* Make sure this sigchain needs to be de-indexed. */
                    if(LLD::Logical->HasFirst(tx.hashGenesis))
                    {
                        /* Get a reference of our transaction. */
                        TAO::API::Transaction wtx = TAO::API::Transaction(tx);

                        /* Make sure indexes are deleted. */
                        if(!wtx.Delete(hash))
                        {
                            debug::warning(FUNCTION, "failed to erase our API indexes for ", hash.SubString());
                            continue;
                        }

                        /* TODO: delete this debug info for < verbose=3 */
                        debug::log(0, FUNCTION, "deleted API session indexes for ", hash.SubString());
                    }
                }
                else if(proof->first == TRANSACTION::LEGACY)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof->second;

                    /* Read from disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction is not on disk");

                    /* Disconnect the inputs. */
                    if(!tx.Disconnect(*this))
                        return debug::error(FUNCTION, "failed to connect inputs");

                    /* Wallets need to refund inputs when disconnecting coinstake */
                    #ifndef NO_WALLET
                    if(tx.IsCoinStake() && Legacy::Wallet::Instance().IsFromMe(tx))
                       Legacy::Wallet::Instance().DisableTransaction(tx);
                    #endif
                }

                /* Write the indexing entries. */
                LLD::Ledger->EraseIndex(proof->second);
            }

            /* Erase the index for block by height. */
            if(config::GetBoolArg("-indexheight"))
                LLD::Ledger->EraseIndex(nHeight);

            /* Update the previous state's next pointer. */
            BlockState prev = Prev();
            if(!prev.IsNull())
            {
                prev.hashNextBlock = 0;
                LLD::Ledger->WriteBlock(prev.GetHash(), prev);
            }

            return true;
        }


        /* USed to determine the trust of a block in the chain. */
        uint64_t BlockState::Trust() const
        {
            /** Give higher block trust if last block was of different channel **/
            BlockState prev = Prev();
            if(!prev.IsNull() && prev.GetChannel() != GetChannel())
                return 3;

            return 1;
        }


        /* Get the weight of this block. */
        uint64_t BlockState::Weight() const
        {
            /* Switch between the weights of the channels. */
            switch(nChannel)
            {
                /* Hash is the weight of the found hash. */
                case CHANNEL::HASH:
                {
                    /* Get the proof hash. */
                    uint1024_t hashProof = ProofHash();

                    /* Get the total weighting. */
                    uint64_t nWeight = ((~hashProof / (hashProof + 1)) + 1).Get64() / 10000;

                    return nWeight + 1;
                }

                /* Proof of stake channel. */
                case CHANNEL::STAKE:
                {
                    /* Get the proof hash. */
                    uint1024_t hashProof = StakeHash();

                    /* Trust information variables. */
                    uint64_t nTrust = 0;
                    uint64_t nStake = 0;

                    /* Handle for version 7 blocks. */
                    if(nVersion >= 7)
                    {
                        /* Get the producer. */
                        Transaction tx;
                        if(!LLD::Ledger->ReadTx(vtx.back().second, tx))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "could not read producer"));

                        /* Get trust information */
                        uint64_t nBalance;
                        if(!tx.GetTrustInfo(nBalance, nTrust, nStake))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get trust info"));

                        /* If this is a genesis then there will be nothing in stake at this point, as the balance is not moved
                           to the stake until the operations layer.  So for the purpose of the weight calculation here we will
                           use the balance from the trust account for the stake amount */
                        if(tx.IsGenesis())
                            nStake = nBalance;
                    }

                    /* Handle for version 5 blocks. */
                    else if(nVersion >= 5)
                    {
                        /* Get the coinstake. */
                        Legacy::Transaction tx;
                        if(!LLD::Legacy->ReadTx(vtx[0].second, tx))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "could not read coinstake"));

                        /* Check for genesis. */
                        if(tx.IsTrust())
                        {
                            /* Temp variables */
                            uint1024_t hashLastBlock;
                            uint32_t   nSequence;
                            uint32_t   nTrustScore;

                            /* Extract trust from coinstake. */
                            if(!tx.ExtractTrust(hashLastBlock, nSequence, nTrustScore))
                                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get trust info"));

                            /* Get the trust score. */
                            nTrust = uint64_t(nTrustScore);
                        }

                        /* Get the stake amount. */
                        nStake = uint64_t(tx.vout[0].nValue);
                    }
                    else
                    {
                        /* Get the coinstake. */
                        Legacy::Transaction tx;
                        if(!LLD::Legacy->ReadTx(vtx[0].second, tx))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "could not read coinstake"));

                        /* Check for genesis. */
                        if(tx.IsTrust())
                        {
                            /* Extract the trust key from the coinstake. */
                            uint576_t cKey;
                            if(!tx.TrustKey(cKey))
                                throw std::runtime_error(debug::safe_printstr(FUNCTION, "trust key not found in script"));

                            /* Get the trust key. */
                            Legacy::TrustKey trustKey;
                            if(!LLD::Trust->ReadTrustKey(cKey, trustKey))
                                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to read trust key"));

                            /* Get trust score. */
                            nTrust = trustKey.Age(TAO::Ledger::ChainState::tStateBest.load().GetBlockTime());

                        }

                        /* Get the stake amount. */
                        nStake = uint64_t(tx.vout[0].nValue);
                    }

                    /* Get the total weighting. */
                    uint64_t nWeight = (((~hashProof / (hashProof + 1)) + 1).Get64() / 10000) * (nStake / NXS_COIN);

                    /* Include trust info in weighting. */
                    return nWeight + nTrust + 1;
                }

                /* Prime (for now) is the weight of the prime difficulty. */
                case CHANNEL::PRIME:
                {
                    /* Check for offset patterns. */
                    if(vOffsets.empty())
                        return nBits * 25;

                    /* Get the prime difficulty. */
                    uint64_t nWeight = SetBits(GetPrimeDifficulty(GetPrime(), vOffsets, false)) * 25;

                    return nWeight + 1;
                }
            }

            return 0;
        }


        /* Function to determine if this block has been connected into the main chain. */
        bool BlockState::IsInMainChain() const
        {
            return (hashNextBlock != 0 || GetHash() == ChainState::hashBestChain.load());
        }


        /* For debugging Purposes seeing block state data dump */
        std::string BlockState::ToString(uint8_t nState) const
        {
            std::string strDebug = "";

            /* Handle verbose output for just header. */
            if(nState & debug::flags::header)
            {
                strDebug += debug::safe_printstr("Block(",
                VALUE("hash") " = ", GetHash().SubString(), ", ",
                VALUE("nVersion") " = ", nVersion, ", ",
                VALUE("hashPrevBlock") " = ", hashPrevBlock.SubString(), ", ",
                VALUE("hashMerkleRoot") " = ", hashMerkleRoot.SubString(), ", ",
                VALUE("nChannel") " = ", nChannel, ", ",
                VALUE("nHeight") " = ", nHeight, ", ",
                VALUE("nDiff") " = ", GetDifficulty(nBits, nChannel), ", ",
                VALUE("nNonce") " = ", nNonce, ", ",
                VALUE("nTime") " = ", nTime, ", ",
                VALUE("blockSig") " = ", HexStr(vchBlockSig.begin(), vchBlockSig.end()));
            }

            /* Handle the verbose output for chain state. */
            if(nState & debug::flags::chain)
            {
                strDebug += debug::safe_printstr(", ",
                VALUE("nChainTrust") " = ", nChainTrust, ", ",
                VALUE("nMoneySupply") " = ", nMoneySupply, ", ",
                VALUE("nChannelHeight") " = ", nChannelHeight, ", ",
                VALUE("nMinerReserve") " = ", nReleasedReserve[0], ", ",
                VALUE("nAmbassadorReserve") " = ", nReleasedReserve[1], ", ",
                VALUE("nDeveloperReserve") " = ", nReleasedReserve[2], ", ",
                VALUE("hashNextBlock") " = ", hashNextBlock.SubString(), ", ",
                VALUE("hashCheckpoint") " = ", hashCheckpoint.SubString());
            }

            strDebug += ")";

            /* Handle the verbose output for transactions. */
            if(nState & debug::flags::tx)
            {
                for(const auto& tx : vtx)
                    strDebug += debug::safe_printstr("\nProof(nType = ", (uint32_t)tx.first, ", hash = ", tx.second.SubString(), ")");
            }

            return strDebug;
        }


        /* For debugging purposes, printing the block to stdout */
        void BlockState::print() const
        {
            debug::log(0, ToString(debug::flags::header | debug::flags::chain));
        }


        /* Get the Signature Hash of the block. Used to verify work claims. */
        uint1024_t BlockState::SignatureHash() const
        {
            /* Signature hash for version 7 blocks. */
            if(nVersion >= 7)
            {
                /* Create a data stream to get the hash. */
                DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
                ss.reserve(256);

                /* Serialize the data to hash into a stream. */
                ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << nTime << vOffsets;

                return LLC::SK1024(ss.begin(), ss.end());
            }

            /* Create a data stream to get the hash. */
            DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
            ss.reserve(256);

            /* Serialize the data to hash into a stream. */
            ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << uint32_t(nTime);

            return LLC::SK1024(ss.begin(), ss.end());
        }


        /* Prove that you staked a number of seconds based on weight. */
        uint1024_t BlockState::StakeHash() const
        {
            /* Version 7 or later stake block should have Tritium coinstake producer, stored as last tx in vtx */
            if(nVersion >= 7 && vtx.back().first == TRANSACTION::TRITIUM)
            {
                /* Get the tritium transaction from the database*/
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(vtx.back().second, tx))
                    return debug::error(FUNCTION, "transaction is not on disk");

                return Block::StakeHash(tx.IsGenesis(), tx.hashGenesis);
            }

            /* pre-version 7 should have Legacy coinstake stored as vtx[0] */
            else if(nVersion < 7 && vtx[0].first == TRANSACTION::LEGACY)
            {
                /* Get the legacy transaction from the database. */
                Legacy::Transaction tx;
                if(!LLD::Legacy->ReadTx(vtx[0].second, tx))
                    return debug::error(FUNCTION, "transaction is not on disk");

                /* Get the trust key. */
                uint576_t keyTrust;
                tx.TrustKey(keyTrust);

                return Block::StakeHash(tx.IsGenesis(), keyTrust);
            }
            else
                throw debug::exception(FUNCTION, "StakeHash called on invalid BlockState");
        }
    }
}
