/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/client.h>

#include <string>

#include <LLD/include/global.h>

#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>

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
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/retarget.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/string.h>



/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Get the block state object. */
        bool GetLastState(ClientBlock &state, const uint32_t nChannel)
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
            state = ChainState::stateGenesis;

            return false;
        }


        /* Default Constructor. */
        ClientBlock::ClientBlock()
        : Block            ( )
        , nTime            (runtime::unifiedtimestamp())
        , ssSystem         ( )
        , nMoneySupply     (0)
        , nChannelHeight   (0)
        , nChannelWeight   {0, 0, 0}
        , nReleasedReserve {0, 0, 0}
        , hashNextBlock    (0)
        {
        }


        /* Copy constructor. */
        ClientBlock::ClientBlock(const ClientBlock& block)
        : Block            (block)
        , nTime            (block.nTime)
        , ssSystem         (block.ssSystem)
        , nMoneySupply     (block.nMoneySupply)
        , nChannelHeight   (block.nChannelHeight)
        , nChannelWeight   {block.nChannelWeight[0]
                           ,block.nChannelWeight[1]
                           ,block.nChannelWeight[2]}
        , nReleasedReserve {block.nReleasedReserve[0]
                           ,block.nReleasedReserve[1]
                           ,block.nReleasedReserve[2]}
        , hashNextBlock    (block.hashNextBlock)
        {
        }


        /* Move constructor. */
        ClientBlock::ClientBlock(ClientBlock&& block) noexcept
        : Block            (std::move(block))
        , nTime            (std::move(block.nTime))
        , ssSystem         (std::move(block.ssSystem))
        , nMoneySupply     (std::move(block.nMoneySupply))
        , nChannelHeight   (std::move(block.nChannelHeight))
        , nChannelWeight   {std::move(block.nChannelWeight[0])
                           ,std::move(block.nChannelWeight[1])
                           ,std::move(block.nChannelWeight[2])}
        , nReleasedReserve {std::move(block.nReleasedReserve[0])
                           ,std::move(block.nReleasedReserve[1])
                           ,std::move(block.nReleasedReserve[2])}
        , hashNextBlock    (std::move(block.hashNextBlock))
        {
        }


        /* Copy assignment. */
        ClientBlock& ClientBlock::operator=(const ClientBlock& block)
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
        ClientBlock& ClientBlock::operator=(ClientBlock&& block) noexcept
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


        /* Copy Constructor from state. */
        ClientBlock::ClientBlock(const BlockState& block)
        : Block            (block)
        , nTime            (block.nTime)
        , ssSystem         (block.ssSystem)
        , nMoneySupply     (block.nMoneySupply)
        , nChannelHeight   (block.nChannelHeight)
        , nChannelWeight   {block.nChannelWeight[0]
                           ,block.nChannelWeight[1]
                           ,block.nChannelWeight[2]}
        , nReleasedReserve {block.nReleasedReserve[0]
                           ,block.nReleasedReserve[1]
                           ,block.nReleasedReserve[2]}
        , hashNextBlock    (block.hashNextBlock)
        {
        }


        /* Move Constructor from state. */
        ClientBlock::ClientBlock(BlockState&& block)
        : Block            (std::move(block))
        , nTime            (std::move(block.nTime))
        , ssSystem         (std::move(block.ssSystem))
        , nMoneySupply     (std::move(block.nMoneySupply))
        , nChannelHeight   (std::move(block.nChannelHeight))
        , nChannelWeight   {std::move(block.nChannelWeight[0])
                           ,std::move(block.nChannelWeight[1])
                           ,std::move(block.nChannelWeight[2])}
        , nReleasedReserve {std::move(block.nReleasedReserve[0])
                           ,std::move(block.nReleasedReserve[1])
                           ,std::move(block.nReleasedReserve[2])}
        , hashNextBlock    (std::move(block.hashNextBlock))
        {
        }


        /* Copy assignment. */
        ClientBlock& ClientBlock::operator=(const BlockState& block)
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
        ClientBlock& ClientBlock::operator=(BlockState&& block) noexcept
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


        /* Default Destructor */
        ClientBlock::~ClientBlock()
        {
        }


        /** Equivilence checking **/
        bool ClientBlock::operator==(const ClientBlock& state) const
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


        /** Equivilence checking **/
        bool ClientBlock::operator!=(const ClientBlock& state) const
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


        /* Not operator overloading. */
        bool ClientBlock::operator!(void) const
        {
            return IsNull();
        }


        /*  Allows polymorphic copying of blocks
         *  Overridden to return an instance of the TritiumBlock class. */
        ClientBlock* ClientBlock::Clone() const
        {
            return new ClientBlock(*this);
        }


        /* Get the previous block state in chain. */
        ClientBlock ClientBlock::Prev() const
        {
            /* Check for genesis. */
            ClientBlock state;
            if(hashPrevBlock == 0)
                return state;

            /* Read the previous block from ledger. */
            if(!LLD::Client->ReadBlock(hashPrevBlock, state))
                throw debug::exception(FUNCTION, "failed to read previous client block ", hashPrevBlock.SubString());

            return state;
        }


        /* Get the next block state in chain. */
        ClientBlock ClientBlock::Next() const
        {
            /* Check for genesis. */
            ClientBlock state;
            if(hashNextBlock == 0)
                return state;

            /* Read next block from the ledger. */
            if(!LLD::Client->ReadBlock(hashNextBlock, state))
                throw debug::exception(FUNCTION, "failed to read next client block ", hashNextBlock.SubString());

            return state;
        }


        /* Return the Block's current UNIX timestamp. */
        uint64_t ClientBlock::GetBlockTime() const
        {
            return nTime;
        }


        /* Check a block state for consistency. */
        bool ClientBlock::Check() const
        {
            /* Check for client mode since this method should never be called except by a client. */
            if(!config::fClient.load())
                return debug::error(FUNCTION, "cannot process client block if not in -client mode");

            /* Check the Size limits of the Current Block. */
            if(::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
                return debug::error(FUNCTION, "size limits failed ", MAX_BLOCK_SIZE);

            /* Make sure the Block was Created within Active Channel. */
            if(GetChannel() > (config::GetBoolArg("-private") ? 3 : 2))
                return debug::error(FUNCTION, "channel out of range");

            /* Check that the time was within range. */
            if(GetBlockTime() > runtime::unifiedtimestamp() + runtime::maxdrift())
                return debug::error(FUNCTION, "block timestamp too far in the future");

            /* Check the Current Version Block Time-Lock. */
            if(!BlockVersionActive(GetBlockTime(), nVersion))
                return debug::error(FUNCTION, "block created with invalid version");

            /* Check the Network Launch Time-Lock. */
            if(!NetworkActive(GetBlockTime()))
                return debug::error(FUNCTION, "block created before network time-lock");

            /* Check the channel time-locks. */
            if(!ChannelActive(GetBlockTime(), GetChannel()))
                return debug::error(FUNCTION, "block created before channel time-lock");

            /* Proof of stake specific checks. */
            if(IsProofOfStake())
            {
                /* Check for nonce of zero values. */
                if(nNonce == 0)
                    return debug::error(FUNCTION, "proof of stake can't have Nonce value of zero");
            }

            /* Proof of work specific checks. */
            else if(IsProofOfWork())
            {
                /* Check for prime offsets. */
                if(GetChannel() == CHANNEL::PRIME && vOffsets.empty())
                    return debug::error(FUNCTION, "prime block requires valid offsets");

                /* Check that other channels do not have offsets. */
                if(GetChannel() != CHANNEL::PRIME && !vOffsets.empty())
                    return debug::error(FUNCTION, "offsets included in non prime block");

                /* Check the Proof of Work Claims. */
                if(!TAO::Ledger::ChainState::Synchronizing() && !VerifyWork())
                    return debug::error(FUNCTION, "invalid proof of work");
            }

            /* Make sure there is no system memory. */
            if(ssSystem.size() != 0)
                return debug::error(FUNCTION, "cannot allocate system memory");

            return true;
        }


        /* Accept a block state with chain state parameters. */
        bool ClientBlock::Accept() const
        {
            /* Check for client mode since this method should never be called except by a client. */
            if(!config::fClient.load())
                return debug::error(FUNCTION, "cannot process block state if not in -client mode");

            /* Read ledger DB for previous block. */
            TAO::Ledger::ClientBlock clientPrev;
            if(!LLD::Client->ReadBlock(hashPrevBlock, clientPrev))
                return debug::error(FUNCTION, "previous client block not found");

            /* Check the Height of Block to Previous Block. */
            if(clientPrev.nHeight + 1 != nHeight)
                return debug::error(FUNCTION, "incorrect block height.");

            /* Check that the nBits match the current Difficulty. */
            //if(nBits != GetNextTargetRequired(clientPrev, GetChannel()))
            //    return debug::error(FUNCTION, "incorrect proof-of-work/proof-of-stake");

            /* Check That Block timestamp is not before previous block. */
            if(GetBlockTime() <= clientPrev.GetBlockTime())
                return debug::error(FUNCTION, "block's timestamp too early");

            /* Start the database transaction. */
            LLD::TxnBegin();

            /* Write the block to disk. */
            if(!Index())
            {
                LLD::TxnAbort();
                return debug::error(FUNCTION, "client block failed to index");
            }

            /* Commit the transaction to database. */
            LLD::TxnCommit();

            return true;
        }


        /* Accept a block state into chain. */
        bool ClientBlock::Index() const
        {
            /* Runtime calculations. */
            runtime::timer timer;
            timer.Start();

            /* Get the block's hash. */
            uint1024_t hashBlock = GetHash();

            /* Read ledger DB for duplicate block. */
            if(LLD::Client->HasBlock(hashBlock))
                return debug::error(FUNCTION, "already have block ", hashBlock.SubString());

            /* Write the block to disk. */
            if(!LLD::Client->WriteBlock(hashBlock, *this))
                return debug::error(FUNCTION, "block state failed to write");

            /* Signal to set the best chain. */
            if(nVersion >= 7 && !IsPrivate())
            {
                /* Set the chain trust. */
                uint8_t nEquals  = 0;
                uint8_t nGreater = 0;

                /* Check to best state. */
                for(uint32_t n = 0; n < 3; ++n)
                {
                    /* Check each weight. */
                    if(nChannelWeight[n] == ChainState::stateBest.load().nChannelWeight[n])
                        ++nEquals;

                    /* Check each weight. */
                    if(nChannelWeight[n] > ChainState::stateBest.load().nChannelWeight[n])
                        ++nGreater;
                }

                /* Check for better height if it is a battle between two channels. */
                if(nHeight > ChainState::nBestHeight.load() + 1 && (nEquals == 1 && nGreater == 1))
                    ++nGreater;

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

            /* Debug output. */
            debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, "ACCEPTED");

            return true;
        }


        bool ClientBlock::SetBest() const
        {
            /* Runtime calculations. */
            runtime::timer timer;
            timer.Start();

            /* Get the hash. */
            uint1024_t hash = GetHash();

            /* Get initial block states. */
            ClientBlock fork   = ChainState::stateBest.load();
            ClientBlock longer = *this;

            /* Get the blocks to connect and disconnect. */
            std::vector<ClientBlock> vDisconnect;
            std::vector<ClientBlock> vConnect;
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
                    "..",  ChainState::stateBest.load().GetHash().SubString());

                /* Keep this in vDisconnect check, or it will print every block, but only print on reorg if have at least 1 */
                if(vConnect.size() > 0)
                    debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "REORGANIZE:", ANSI_COLOR_RESET,
                        " Connect ", vConnect.size(), " blocks; ", fork.GetHash().SubString(),
                        "..", hash.SubString());
            }

            /* Disconnect given blocks. */
            for(auto& state : vDisconnect)
            {
                /* Output the block state if flagged. */
                if(config::GetBoolArg("-printstate"))
                    debug::log(0, state.ToString(debug::flags::header | debug::flags::tx));

                /* Connect the block. */
                if(!state.Disconnect())
                    return debug::error(FUNCTION, "failed to disconnect ", state.GetHash().SubString());

                /* Erase block if not connecting anything. */
                if(vConnect.empty())
                    LLD::Ledger->EraseBlock(state.GetHash());
            }

            /* Reverse the blocks to connect to connect in ascending height. */
            for(auto state = vConnect.rbegin(); state != vConnect.rend(); ++state)
            {
                /* Output the block state if flagged. */
                if(config::GetBoolArg("-printstate"))
                    debug::log(0, state->ToString(debug::flags::header | debug::flags::tx));

                /* Connect the block. */
                if(!state->Connect())
                    return debug::error(FUNCTION, "failed to connect ", state->GetHash().SubString());
            }

            /* Debug output about the best chain. */
            uint64_t nTimer   = timer.ElapsedMilliseconds();

            if(config::nVerbose >= TAO::Ledger::ChainState::Synchronizing() ? 1 : 0)
                debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION,
                    "New Best Block hash=", hash.SubString(),
                    " height=", nHeight,
                    " supply=", std::fixed, (double)nMoneySupply / TAO::Ledger::NXS_COIN,
                    " [verified in ", nTimer, " ms]",
                    " [", ::GetSerializeSize(*this, SER_LLD, nVersion), " bytes]");

            /* Set the best chain variables. */
            ChainState::stateBest          = *this;
            ChainState::hashBestChain      = hash;
            ChainState::nBestHeight        = nHeight;

            /* Write the best chain pointer. */
            if(!LLD::Ledger->WriteBestChain(ChainState::hashBestChain.load()))
                return debug::error(FUNCTION, "failed to write best chain");

            return true;
        }


        /* Connect a block state into chain. */
        bool ClientBlock::Connect() const
        {
            /* Update the previous state's next pointer. */
            ClientBlock prev = Prev();
            if(!prev.IsNull())
            {
                prev.hashNextBlock = GetHash();
                if(!LLD::Ledger->WriteBlock(prev.GetHash(), prev))
                    return debug::error(FUNCTION, "failed to update previous block state");

                /* If we just updated hashNextBlock for genesis block, update the in-memory genesis */
                if(prev.GetHash() == ChainState::Genesis())
                    ChainState::stateGenesis = prev;
            }

            return true;
        }


        /* Disconnect a block state from the chain. */
        bool ClientBlock::Disconnect() const
        {
            /* Update the previous state's next pointer. */
            ClientBlock prev = Prev();
            if(!prev.IsNull())
            {
                prev.hashNextBlock = 0;
                LLD::Ledger->WriteBlock(prev.GetHash(), prev);
            }

            return true;
        }


        /* Function to determine if this block has been connected into the main chain. */
        bool ClientBlock::IsInMainChain() const
        {
            return (hashNextBlock != 0 || GetHash() == ChainState::hashBestChain.load());
        }


        /* For debugging Purposes seeing block state data dump */
        std::string ClientBlock::ToString(uint8_t nState) const
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
                VALUE("nTime") " = ", nTime);
            }

            /* Handle the verbose output for chain state. */
            if(nState & debug::flags::chain)
            {
                strDebug += debug::safe_printstr(", ",
                VALUE("nMoneySupply") " = ", nMoneySupply, ", ",
                VALUE("nChannelHeight") " = ", nChannelHeight, ", ",
                VALUE("nMinerReserve") " = ", nReleasedReserve[0], ", ",
                VALUE("nAmbassadorReserve") " = ", nReleasedReserve[1], ", ",
                VALUE("nDeveloperReserve") " = ", nReleasedReserve[2], ", ",
                VALUE("hashNextBlock") " = ", hashNextBlock.SubString());
            }

            strDebug += ")";

            return strDebug;
        }


        /* For debugging purposes, printing the block to stdout */
        void ClientBlock::print() const
        {
            debug::log(0, ToString(debug::flags::header | debug::flags::chain));
        }


        /* Get the Signarture Hash of the block. Used to verify work claims. */
        uint1024_t ClientBlock::SignatureHash() const
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
    }
}
