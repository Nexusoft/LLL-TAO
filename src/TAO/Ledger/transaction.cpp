/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/argon2.h>

#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/Operation/include/cost.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/condition.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/build.h>
#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/ambassador.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Default Constructor. */
        Transaction::Transaction()
        : vContracts   ( )
        , nVersion     (1)
        , nSequence    (0)
        , nTimestamp   (runtime::unifiedtimestamp())
        , hashNext     (0)
        , hashRecovery (0)
        , hashGenesis  (0)
        , hashPrevTx   (0)
        , nKeyType     (0)
        , nNextType    (0)
        , vchPubKey    ( )
        , vchSig       ( )
        {
        }


        /* Copy constructor. */
        Transaction::Transaction(const Transaction& tx)
        : vContracts   (tx.vContracts)
        , nVersion     (tx.nVersion)
        , nSequence    (tx.nSequence)
        , nTimestamp   (tx.nTimestamp)
        , hashNext     (tx.hashNext)
        , hashRecovery (tx.hashRecovery)
        , hashGenesis  (tx.hashGenesis)
        , hashPrevTx   (tx.hashPrevTx)
        , nKeyType     (tx.nKeyType)
        , nNextType    (tx.nNextType)
        , vchPubKey    (tx.vchPubKey)
        , vchSig       (tx.vchSig)
        {
        }


        /* Move constructor. */
        Transaction::Transaction(Transaction&& tx) noexcept
        : vContracts   (std::move(tx.vContracts))
        , nVersion     (std::move(tx.nVersion))
        , nSequence    (std::move(tx.nSequence))
        , nTimestamp   (std::move(tx.nTimestamp))
        , hashNext     (std::move(tx.hashNext))
        , hashRecovery (std::move(tx.hashRecovery))
        , hashGenesis  (std::move(tx.hashGenesis))
        , hashPrevTx   (std::move(tx.hashPrevTx))
        , nKeyType     (std::move(tx.nKeyType))
        , nNextType    (std::move(tx.nNextType))
        , vchPubKey    (std::move(tx.vchPubKey))
        , vchSig       (std::move(tx.vchSig))
        {
        }


        /* Copy assignment. */
        Transaction& Transaction::operator=(const Transaction& tx)
        {
            vContracts   = tx.vContracts;
            nVersion     = tx.nVersion;
            nSequence    = tx.nSequence;
            nTimestamp   = tx.nTimestamp;
            hashNext     = tx.hashNext;
            hashRecovery = tx.hashRecovery;
            hashGenesis  = tx.hashGenesis;
            hashPrevTx   = tx.hashPrevTx;
            nKeyType     = tx.nKeyType;
            nNextType    = tx.nNextType;
            vchPubKey    = tx.vchPubKey;
            vchSig       = tx.vchSig;

            return *this;
        }


        /* Move assignment. */
        Transaction& Transaction::operator=(Transaction&& tx) noexcept
        {
            vContracts   = std::move(tx.vContracts);
            nVersion     = std::move(tx.nVersion);
            nSequence    = std::move(tx.nSequence);
            nTimestamp   = std::move(tx.nTimestamp);
            hashNext     = std::move(tx.hashNext);
            hashRecovery = std::move(tx.hashRecovery);
            hashGenesis  = std::move(tx.hashGenesis);
            hashPrevTx   = std::move(tx.hashPrevTx);
            nKeyType     = std::move(tx.nKeyType);
            nNextType    = std::move(tx.nNextType);
            vchPubKey    = std::move(tx.vchPubKey);
            vchSig       = std::move(tx.vchSig);

            return *this;
        }


        /* Default Destructor. */
        Transaction::~Transaction()
        {
        }


        /* Used for sorting transactions by sequence. */
        bool Transaction::operator>(const Transaction& tx) const
        {
            return nSequence > tx.nSequence;
        }


        /* Used for sorting transactions by sequence. */
        bool Transaction::operator<(const Transaction& tx) const
        {
            return nSequence < tx.nSequence;
        }


        /* Access for the contract operator overload. This is for read-only objects. */
        const TAO::Operation::Contract& Transaction::operator[](const uint32_t n) const
        {
            /* Check contract bounds. */
            if(n >= vContracts.size())
                throw debug::exception(FUNCTION, "contract read out of bounds");

            /* Bind this transaction. */
            vContracts[n].Bind(this);

            return vContracts[n];
        }


        /* Write access fot the contract operator overload. This handles writes to create new contracts. */
        TAO::Operation::Contract& Transaction::operator[](const uint32_t n)
        {
            /* Check for contract bounds. */
            if(n >= MAX_TRANSACTION_CONTRACTS)
                throw debug::exception(FUNCTION, "contract create out of bounds");

            /* Allocate a new contract if on write. */
            if(n >= vContracts.size())
                vContracts.resize(n + 1);

            /* Bind this transaction. */
            vContracts[n].Bind(this);

            return vContracts[n];
        }


        /* Get the total contracts in transaction. */
        uint32_t Transaction::Size() const
        {
            return vContracts.size();
        }


        /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
        bool Transaction::Check() const
        {
            /* Check transaction version */
            if(nVersion != 1)
                return debug::error(FUNCTION, "invalid transaction version");

            /* Check for genesis valid numbers. */
            if(hashGenesis == 0)
                return debug::error(FUNCTION, "genesis cannot be zero");

            /* Check for genesis valid numbers. */
            if(hashNext == 0)
                return debug::error(FUNCTION, "nextHash cannot be zero");

            /* Check the timestamp. */
            if(nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "transaction timestamp too far in the future ", nTimestamp);

            /* Check for empty signatures. */
            if(vchSig.size() == 0)
                return debug::error(FUNCTION, "transaction with empty signature");

            /* Check the genesis first byte. */
            if(hashGenesis.GetType() != (config::fTestNet.load() ? 0xa2 : 0xa1))
                return debug::error(FUNCTION, "genesis using incorrect leading byte");

            /* Check for max contracts. */
            if(vContracts.size() > MAX_TRANSACTION_CONTRACTS)
                return debug::error(FUNCTION, "transaction contract limit exceeded", vContracts.size());

            /* Count of non-fee contracts in the transaction */
            uint8_t nContracts = 0;

            /* Run through all the contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for empty contracts. */
                if(contract.Empty(TAO::Operation::Contract::OPERATIONS))
                    return debug::error(FUNCTION, "contract is empty");

                if(contract.Primitive() == TAO::Operation::OP::LEGACY
                && (TAO::Ledger::VersionActive(nTimestamp, 6) || TAO::Ledger::CurrentVersion() < 6))
                    return debug::error(FUNCTION, "no send-to-legacy until version 6 grace period ends");

                if(contract.Primitive() != TAO::Operation::OP::FEE)
                    ++nContracts;
            }

            /* Check contains contracts. */
            if(nContracts == 0)
                return debug::error(FUNCTION, "transaction is empty");

            /* If genesis then check that the only contracts are those for the default registers.
               NOTE: we do not make this limitation in private mode */
            if(IsFirst() && !config::GetBoolArg("-private"))
            {
                //skip proof of work for unit tests
                #ifndef UNIT_TESTS

                /* Check the difficulty of the hash. */
                if(ProofHash() > FIRST_REQUIRED_WORK)
                    return debug::error(FUNCTION, "first transaction not enough work");

                #endif

                /* Number of Name contracts */
                uint8_t nNames = 0;

                /* Number of Trust contracts */
                uint8_t nTrust = 0;

                /* Number of Account contracts */
                uint8_t nAccounts = 0;

                /* Number of Crypto contracts */
                uint8_t nCrypto = 0;

                /* Run through all the contracts. */
                for(const auto& contract : vContracts)
                {
                    /* Reset the contract operation stream */
                    contract.Reset();

                    /* Get the Operation */
                    uint8_t nOP;
                    contract >> nOP;

                    /* Check that the operation is a CREATE as these are the only OP's allowed in the genesis transaction */
                    if(nOP == TAO::Operation::OP::CREATE)
                    {
                        /* The register address */
                        TAO::Register::Address address;

                        /* Deserialize the register address from the contract */
                        contract >> address;

                        /* Check the type of each register and increment the relative counter. */
                        switch(address.GetType())
                        {
                            case TAO::Register::Address::ACCOUNT:
                                ++nAccounts;
                                break;
                            case TAO::Register::Address::TRUST:
                                ++nTrust;
                                break;
                            case TAO::Register::Address::NAME:
                                ++nNames;
                                break;
                            case TAO::Register::Address::CRYPTO:
                                ++nCrypto;
                                break;
                            default:
                                return debug::error(FUNCTION, "genesis transaction contains invalid contracts.");
                        }
                    }
                    else
                    {
                        return debug::error(FUNCTION, "genesis transaction contains invalid contracts.");
                    }
                }

                /* Check that the there are not more than the allowable default contracts */
                if(vContracts.size() > 5 || nNames > 2 || nTrust > 1 || nAccounts > 1 || nCrypto > 1)
                    return debug::error(FUNCTION, "genesis transaction contains invalid contracts.");
            }

            /* Verify the block signature (if not synchronizing) */
            if(!TAO::Ledger::ChainState::Synchronizing())
            {
                /* Switch based on signature type. */
                switch(nKeyType)
                {
                    /* Support for the FALCON signature scheeme. */
                    case SIGNATURE::FALCON:
                    {
                        /* Create the FL Key object. */
                        LLC::FLKey key;

                        /* Set the public key and verify. */
                        key.SetPubKey(vchPubKey);
                        if(!key.Verify(GetHash().GetBytes(), vchSig))
                            return debug::error(FUNCTION, "invalid transaction signature");

                        break;
                    }

                    /* Support for the BRAINPOOL signature scheme. */
                    case SIGNATURE::BRAINPOOL:
                    {
                        /* Create EC Key object. */
                        LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                        /* Set the public key and verify. */
                        key.SetPubKey(vchPubKey);
                        if(!key.Verify(GetHash().GetBytes(), vchSig))
                            return debug::error(FUNCTION, "invalid transaction signature");

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "unknown signature type");
                }
            }

            return true;
        }


        /* Verify a transaction contracts. */
        bool Transaction::Verify(const uint8_t nFlags) const
        {
            /* Create a temporary map for pre-states. */
            std::map<uint256_t, TAO::Register::State> mapStates;

            /* Run through all the contracts. */
            for(const auto& contract : vContracts)
            {
                /* Bind the contract to this transaction. */
                contract.Bind(this);

                /* Verify the register pre-states. */
                if(!TAO::Register::Verify(contract, mapStates, nFlags))
                    return false;
            }

            return true;
        }


        /* Check the trust score that is claimed is correct. */
        bool Transaction::CheckTrust(const BlockState* pblock) const
        {
            /* Check for proof of stake. */
            if(!IsCoinStake())
                return debug::error(FUNCTION, "no trust on non coinstake");

            /* Reset the coinstake contract streams. */
            vContracts[0].Reset();

            /* Get the trust object register. */
            TAO::Register::Object account;

            /* Deserialize from the stream. */
            uint8_t nState = 0;
            vContracts[0] >>= nState;
            vContracts[0] >>= account;

            /* Parse the object. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse object register from pre-state");

            /* Validate that it is a trust account. */
            if(account.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "stake producer account is not a trust account");

            /* Values for trust calculations. */
            uint64_t nTrust       = 0;
            uint64_t nTrustPrev   = 0;
            uint64_t nReward      = 0;
            uint64_t nBlockAge    = 0;
            uint64_t nStake       = 0;
            int64_t  nStakeChange = 0;

            /* Check for trust calculations. */
            if(IsTrust())
            {
                /* Extract values from producer operation */
                vContracts[0].Seek(1, TAO::Operation::Contract::OPERATIONS);

                /* Get last trust hash. */
                uint512_t hashLastClaimed = 0;
                vContracts[0] >> hashLastClaimed;

                /* Get claimed trust score. */
                uint64_t nClaimedTrust = 0;
                vContracts[0] >> nClaimedTrust;
                vContracts[0] >> nStakeChange;

                /* Get claimed rewards. */
                uint64_t nClaimedReward = 0;
                vContracts[0] >> nClaimedReward;

                /* Validate the claimed hash last stake */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadStake(hashGenesis, hashLast))
                    return debug::error(FUNCTION, "last stake not in database");

                /* Check for last hash consistency. */
                if(hashLast != hashLastClaimed)
                    return debug::error(FUNCTION, "list stake ", hashLastClaimed.SubString(), " mismatch ", hashLast.SubString());

                /* Get pre-state trust account values */
                nTrustPrev = account.get<uint64_t>("trust");
                nStake = account.get<uint64_t>("stake");

                /* Get previous block. Block time used for block age/coin age calculation */
                TAO::Ledger::BlockState statePrev;
                if(!LLD::Ledger->ReadBlock(pblock->hashPrevBlock, statePrev))
                    return debug::error(FUNCTION, "prev block not in database");

                /* Get the last stake block. */
                TAO::Ledger::BlockState stateLast;
                if(!LLD::Ledger->ReadBlock(hashLastClaimed, stateLast))
                    return debug::error(FUNCTION, "last block not in database");

                /* Calculate Block Age (time from last stake block until previous block) */
                nBlockAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();
                nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange);

                /* Validate the trust score calculation */
                if(nClaimedTrust != nTrust)
                    return debug::error(FUNCTION, "claimed trust score ", nClaimedTrust,
                                                  " does not match calculated trust score ", nTrust);

                /* Enforce the minimum interval between stake blocks. */
                const uint32_t nInterval = pblock->nHeight - stateLast.nHeight;
                if(nInterval <= MinStakeInterval(*pblock))
                    return debug::error(FUNCTION, "stake block interval ", nInterval, " below minimum interval");

                /* Calculate the coinstake reward */
                const uint64_t nTime = pblock->GetBlockTime() - stateLast.GetBlockTime();
                nReward = GetCoinstakeReward(nStake, nTime, nTrust, false);

                /* Validate the coinstake reward calculation */
                if(nClaimedReward != nReward)
                    return debug::error(FUNCTION, "claimed stake reward ", nClaimedReward,
                                                  " does not match calculated reward ", nReward);
            }

            else if(IsGenesis())
            {
                /* Seek to claimed reward. */
                vContracts[0].Seek(1, TAO::Operation::Contract::OPERATIONS);

                /* Check claimed reward calculations. */
                uint64_t nClaimedReward = 0;
                vContracts[0] >> nClaimedReward;

                /* Get Genesis stake from the trust account pre-state balance. Genesis reward based on balance (that will move to stake) */
                nStake = account.get<uint64_t>("balance");

                /* Calculate the Coin Age. */
                const uint64_t nAge = pblock->GetBlockTime() - account.nModified;

                /* Calculate the coinstake reward */
                nReward = GetCoinstakeReward(nStake, nAge, 0, true);

                /* Validate the coinstake reward calculation */
                if(nClaimedReward != nReward)
                    return debug::error(FUNCTION, "claimed hashGenesis reward ", nClaimedReward, " does not match calculated reward ", nReward);
            }

            else
                return debug::error(FUNCTION, "invalid stake operation");

            /* Set target for logging */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(pblock->nBits);

            /* Verbose logging. */
            if(config::nVerbose >= 2)
                debug::log(2, FUNCTION,
                    "stake hash=", pblock->StakeHash().SubString(), ", ",
                    "target=", bnTarget.getuint1024().SubString(), ", ",
                    "type=", (IsTrust() ? "Trust" : "Genesis"), ", ",
                    "trust score=", nTrust, ", ",
                    "prev trust score=", nTrustPrev, ", ",
                    "trust change=", int64_t(nTrust - nTrustPrev), ", ",
                    "block age=", nBlockAge, ", ",
                    "stake=", nStake, ", ",
                    "reward=", nReward, ", ",
                    "add stake=", ((nStakeChange > 0) ? nStakeChange : 0), ", ",
                    "unstake=", ((nStakeChange < 0) ? (0 - nStakeChange) : 0));

            return true;
        }


        /* Get the total cost of this transaction. */
        uint64_t Transaction::Cost()
        {
            /* Get the cost value. */
            uint64_t nRet = 0;

            /* There are no transaction costs in private mode */
            if(config::GetBoolArg("-private", false))
                return 0;

            /* No transaction cost in the first transaction as this is where we set up default accounts */
            if(IsFirst())
                return 0;

            /* Need the previous transaction timestamp for throttling fees */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(hashPrevTx, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "prev transaction not on disk");

            /* The timestamp of the previous transaction */
            uint64_t nPrevTimestamp = txPrev.nTimestamp;

            /* flag indicating that transaction fees should apply, depending on the time since the last transaction */
            bool fApplyTxFee = nTimestamp - nPrevTimestamp < TX_FEE_INTERVAL;

            /* Run through all the contracts. */
            for(auto& contract : vContracts)
            {
                /* Bind the contract to this transaction. */
                contract.Bind(this);

                /* Calculate the total cost to execute. */
                TAO::Operation::Cost(contract, nRet);

                /* If transaction fees should apply, calculate the additional transaction cost for the contract */
                if(fApplyTxFee)
                    TAO::Operation::TxCost(contract, nRet);
            }

            return nRet;
        }


        /* Build the transaction contracts. */
        bool Transaction::Build()
        {
            /* Create a temporary map for pre-states. */
            std::map<uint256_t, TAO::Register::State> mapStates;

            /* Check for max contracts. */
            if(vContracts.size() > MAX_TRANSACTION_CONTRACTS)
                return debug::error(FUNCTION, "exceeded MAX_TRANSACTION_CONTRACTS");

            /* Run through all the contracts. */
            for(auto& contract : vContracts)
            {
                /* Bind the contract to this transaction. */
                contract.Bind(this);

                /* Calculate the pre-states and post-states. */
                if(!TAO::Register::Build(contract, mapStates, FLAGS::MEMPOOL))
                    return false;
            }


            //skip proof of work for unit tests
            #ifndef UNIT_TESTS

            /* Check for first. */
            if(IsFirst() && !config::GetBoolArg("-private"))
            {
                /* Timer to track proof of work time. */
                runtime::timer timer;
                timer.Reset();

                /* Solve proof of work. */
                while(!config::fShutdown)
                {
                    /* Break when value is found. */
                    if(ProofHash() < FIRST_REQUIRED_WORK)
                        break;

                    ++hashPrevTx;
                }

                debug::log(0, FUNCTION, "Proof ", ProofHash().SubString(), " PoW in ", timer.Elapsed(), " seconds");
            }

            #endif

            return true;
        }


        /* Connect a transaction object to the main chain. */
        bool Transaction::Connect(const uint8_t nFlags, const BlockState* pblock)
        {
            /* Get the transaction's hash. */
            uint512_t hash = GetHash();

            /* flag indicating that transaction fees should apply, depending on the time since the last transaction */
            bool fApplyTxFee = false;

            /* The total cost of this transaction.  This is calculated from executing each contract. */
            uint64_t nCost = 0;

            /* Check for first. */
            if(IsFirst())
            {
                /* Check for ambassador sigchains. */
                if(!config::fTestNet.load() && AMBASSADOR.find(hashGenesis) != AMBASSADOR.end())
                {
                    /* Debug logging. */
                    debug::log(1, FUNCTION, "Processing AMBASSADOR sigchain ", hashGenesis.SubString());

                    /* Check that the hashes match. */
                    if(AMBASSADOR.at(hashGenesis).first != PrevHash())
                        return debug::error(FUNCTION, "AMBASSADOR sigchain using invalid credentials");
                }


                /* Check for ambassador sigchains. */
                if(config::fTestNet.load() && AMBASSADOR_TESTNET.find(hashGenesis) != AMBASSADOR_TESTNET.end())
                {
                    /* Debug logging. */
                    debug::log(1, FUNCTION, "Processing TESTNET AMBASSADOR sigchain ", hashGenesis.SubString());

                    /* Check that the hashes match. */
                    if(AMBASSADOR_TESTNET.at(hashGenesis).first != PrevHash())
                        return debug::error(FUNCTION, "TESTNET AMBASSADOR sigchain using invalid credentials");
                }

                /* Write specific transaction flags. */
                if(nFlags == TAO::Ledger::FLAGS::BLOCK)
                {
                    /* Write the genesis identifier. */
                    if(!LLD::Ledger->WriteGenesis(hashGenesis, hash))
                        return debug::error(FUNCTION, "failed to write genesis");
                }
            }
            else
            {
                /* Make sure the previous transaction is on disk or mempool. */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::Ledger->ReadTx(hashPrevTx, txPrev, nFlags))
                    return debug::error(FUNCTION, "prev transaction not on disk");

                /* Double check sequence numbers here. */
                if(txPrev.nSequence + 1 != nSequence)
                    return debug::error(FUNCTION, "prev transaction incorrect sequence");

                /* Check timestamp to previous transaction. */
                if(nTimestamp < txPrev.nTimestamp)
                    return debug::error(FUNCTION, "timestamp too far in the past ", txPrev.nTimestamp - nTimestamp);

                /* Work out the whether transaction fees should apply based on the interval between transactions */
                fApplyTxFee = ((nTimestamp - txPrev.nTimestamp) < TX_FEE_INTERVAL);

                /* Check the previous next hash that is being claimed. */
                bool fRecovery = false;
                if(txPrev.hashNext != PrevHash())
                {
                    /* Check that previous hash matches recovery. */
                    if(txPrev.hashRecovery == PrevHash())
                    {
                        /* Check that recovery hash is not 0. */
                        if(txPrev.hashRecovery == 0)
                            return debug::error(FUNCTION, "NOTICE: recovery hash disabled");

                        /* Log that transaction is being recovered. */
                        debug::log(0, FUNCTION, "NOTICE: transaction is using recovery hash");

                        /* Set recovery mode to be enabled. */
                        fRecovery = true;
                    }
                    else
                        return debug::error(FUNCTION, "invalid signature chain credentials");
                }

                /* Check recovery hash is sequenced from previous tx (except for changing from 0) */
                if(!fRecovery && txPrev.hashRecovery != hashRecovery && txPrev.hashRecovery != 0)
                    return debug::error(FUNCTION, "recovery hash broken chain"); //this can only be updated when recovery executed

                /* Check the previous genesis. */
                if(txPrev.hashGenesis != hashGenesis)
                    return debug::error(FUNCTION, "genesis hash broken chain");

                /* Check previous transaction from disk hash. */
                if(txPrev.GetHash() != hashPrevTx) //NOTE: this is being extra paranoid. Consider removing.
                    return debug::error(FUNCTION, "prev transaction prevhash mismatch");
            }

            /* Keep for dependants. */
            uint512_t hashPrev = 0;
            uint32_t nContract = 0;

            /* Run through all the contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for confirmations when on a block. */
                if(nFlags == FLAGS::BLOCK || nFlags == FLAGS::MINER)
                {
                    /* Check for dependants. */
                    if(contract.Dependant(hashPrev, nContract))
                    {
                        /* Check that the previous transaction is indexed. */
                        if(nFlags == FLAGS::BLOCK && !LLD::Ledger->HasIndex(hashPrev))
                            return debug::error(FUNCTION, hashPrev.SubString(), " not indexed");

                        /* Read previous transaction from disk. */
                        const TAO::Operation::Contract dependant = LLD::Ledger->ReadContract(hashPrev, nContract, nFlags);
                        switch(dependant.Primitive())
                        {
                            /* Handle coinbase rules. */
                            case TAO::Operation::OP::COINBASE:
                            {
                                /* Get number of confirmations of previous TX */
                                uint32_t nConfirms = 0;
                                if(!LLD::Ledger->ReadConfirmations(hashPrev, nConfirms, pblock))
                                    return debug::error(FUNCTION, "failed to read confirmations for coinbase");

                                /* Check that the previous TX has reached sig chain maturity */
                                if(nConfirms + 1 < MaturityCoinBase(*pblock))
                                    return debug::error(FUNCTION, "coinbase is immature ", nConfirms);

                                break;
                            }
                        }
                    }
                }

                /* Bind the contract to this transaction. */
                contract.Bind(this);

                /* Execute the contracts to final state. */
                if(!TAO::Operation::Execute(contract, nFlags, nCost))
                    return false;

                /* If transaction fees should apply, calculate the additional transaction cost for the contract */
                if(fApplyTxFee)
                    TAO::Operation::TxCost(contract, nCost);
            }

            /* Once we have executed the contracts we need to check the fees.
               NOTE there are fixed fees on the genesis transaction to allow for the default registers to be created.
               NOTE: There are no fees required in private mode.  */
            if(!config::GetBoolArg("-private", false))
            {
                /* The fee applied to this transaction */
                uint64_t nFees = 0;
                if(IsFirst())
                {
                    /* For the genesis transaction we allow a fixed amount of default registers to be created for free. */
                    nFees = 2 * TAO::Ledger::ACCOUNT_FEE    // 2 accounts
                          + 2 * TAO::Ledger::NAME_FEE       // 2 names
                          + 1 * TAO::Ledger::CRYPTO_FEE;    // 1 crypto register
                }
                else
                    /* For all other transactions we check the actual fee contracts included in the transaction */
                    nFees = Fees();

                /* Check that the fees match.  */
                if(nCost > nFees)
                    return debug::error(FUNCTION, "not enough fees supplied ", nFees);
            }

            return true;
        }


        /* Disconnect a transaction object to the main chain. */
        bool Transaction::Disconnect(const uint8_t nFlags)
        {
            /* Disconnect on block needs to manage ledger level indexes. */
            if(nFlags == FLAGS::BLOCK)
            {
                /* Erase last for genesis. */
                if(IsFirst() && !LLD::Ledger->EraseLast(hashGenesis))
                    return debug::error(FUNCTION, "failed to erase last hash");

                /* Write proper last hash index. */
                else if(!LLD::Ledger->WriteLast(hashGenesis, hashPrevTx))
                    return debug::error(FUNCTION, "failed to write last hash");

                /* Revert last stake whan disconnect a coinstake tx */
                if(IsCoinStake())
                {
                    if(IsTrust())
                    {
                        /* Extract the last stake hash from the coinstake contract */
                        uint512_t hashLast = 0;
                        if(!vContracts[0].Previous(hashLast))
                            return debug::error(FUNCTION, "failed to extract last stake hash from contract");

                        /* Revert saved last stake to the prior stake transaction */
                        if(!LLD::Ledger->WriteStake(hashGenesis, hashLast))
                            return debug::error(FUNCTION, "failed to write last stake");

                        /* If local database has a stake change request for this transaction that marked as processed, update it.
                         * This resets the request but keeps the tx hash, so if it is later reconnected it can be marked
                         * as processed again. Otherwise, the stake minter can recognized that the original coinstake was
                         * disconnected and implement the stake change request with the next stake block found.
                         */
                        StakeChange request;
                        if(LLD::Local->ReadStakeChange(hashGenesis, request) && request.fProcessed && request.hashTx == GetHash())
                        {
                            request.fProcessed = false;

                            if(!LLD::Local->WriteStakeChange(hashGenesis, request))
                                debug::error(FUNCTION, "unable to reinstate disconnected stake change request"); //don't fail for this
                        }
                    }
                    else
                    {
                        if(!LLD::Ledger->EraseStake(hashGenesis))
                            return debug::error(FUNCTION, "failed to erase last stake");
                    }
                }
            }

            /* Run through all the contracts in reverse order to disconnect. */
            for(auto contract = vContracts.rbegin(); contract != vContracts.rend(); ++contract)
            {
                contract->Bind(this);
                if(!TAO::Register::Rollback(*contract, nFlags))
                    return false;
            }

            return true;
        }


        /* Determines if the transaction is a coinbase transaction. */
        bool Transaction::IsCoinBase() const
        {
            /* Check all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for empty first contract. */
                if(contract.Empty())
                    return false;

                /* Check for conditions. */
                if(!contract.Empty(TAO::Operation::Contract::CONDITIONS))
                    return false;

                if(contract.Primitive() != TAO::Operation::OP::COINBASE)
                    return false;
            }

            return true;
        }


        /* Determines if the transaction is a coinstake (trust or genesis) transaction. */
        bool Transaction::IsCoinStake() const
        {
            return (IsGenesis() || IsTrust());
        }


        /* Determines if the transaction is for a private block. */
        bool Transaction::IsPrivate() const
        {
            /* Check for single contract. */
            if(vContracts.size() != 1)
                return false;

            /* Check for empty first contract. */
            if(vContracts[0].Empty())
                return false;

            /* Check for conditions. */
            if(!vContracts[0].Empty(TAO::Operation::Contract::CONDITIONS))
                return false;

            return (vContracts[0].Primitive() == TAO::Operation::OP::AUTHORIZE);
        }


        /* Determines if the transaction is a coinstake transaction. */
        bool Transaction::IsTrust() const
        {
            /* Check for single contract. */
            if(vContracts.size() != 1)
                return false;

            /* Check for empty first contract. */
            if(vContracts[0].Empty())
                return false;

            /* Check for conditions. */
            if(!vContracts[0].Empty(TAO::Operation::Contract::CONDITIONS))
                return false;

            return (vContracts[0].Primitive() == TAO::Operation::OP::TRUST);
        }


        /* Determines if the transaction is a genesis transaction */
        bool Transaction::IsGenesis() const
        {
            /* Check for single contract. */
            if(vContracts.size() != 1)
                return false;

            /* Check for empty first contract. */
            if(vContracts[0].Empty())
                return false;

            /* Check for conditions. */
            if(!vContracts[0].Empty(TAO::Operation::Contract::CONDITIONS))
                return false;

            return (vContracts[0].Primitive() == TAO::Operation::OP::GENESIS);
        }


        /* Determines if the transaction is a genesis transaction */
        bool Transaction::IsFirst() const
        {
            return nSequence == 0;
        }


        /*  Gets the total trust and stake of pre-state. */
        bool Transaction::GetTrustInfo(uint64_t &nBalance, uint64_t &nTrust, uint64_t &nStake) const
        {
            /* Check values. */
            if(!IsCoinStake())
                return debug::error(FUNCTION, "transaction is not trust");

            /* Get internal contract. */
            const TAO::Operation::Contract& contract = vContracts[0];

            /* Seek to pre-state. */
            contract.Reset();
            contract.Seek(1, TAO::Operation::Contract::REGISTERS);

            /* Get pre-state. */
            TAO::Register::Object object;
            contract >>= object;

            /* Reset contract. */
            contract.Reset();

            /* Parse object. */
            if(!object.Parse())
                return debug::error(FUNCTION, "failed to parse trust object");

            /* Set Values. */
            nBalance = object.get<uint64_t>("balance");
            nTrust = object.get<uint64_t>("trust");
            nStake = object.get<uint64_t>("stake");

            return true;
        }


        /* Gets the hash of the transaction object. */
        uint512_t Transaction::GetHash() const
        {
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            /* Get the hash. */
            uint512_t hash = LLC::SK512(ss.begin(), ss.end());

            /* Type of 0xff designates tritium tx. */
            hash.SetType(TAO::Ledger::TRITIUM);

            return hash;
        }


        /* Gets a proof hash of the transaction object. */
        uint512_t Transaction::ProofHash() const
        {
            /* Set the return hash. */
            uint512_t hashKey = ~uint512_t(0);

            /* Check for first transaction. */
            if(!IsFirst())
                return hashKey; //this will always fail proof of work checks

            /* Serialize data into binary stream. */
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            /* Create the hash context. */
            argon2_context context =
            {
                /* Hash Return Value. */
                (uint8_t*)&hashKey,
                64,

                /* Password input data. */
                ss.data(),
                static_cast<uint32_t>(ss.size()),

                /* The salt for usernames */
                (uint8_t*)&hashGenesis,
                32,

                /* Optional secret data */
                NULL, 0,

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                1,

                /* Memory Cost. */
                (1 << 8),

                /* The number of threads and lanes */
                1, 1,

                /* Algorithm Version */
                ARGON2_VERSION_13,

                /* Custom memory allocation / deallocation functions. */
                NULL, NULL,

                /* By default only internal memory is cleared (pwd is not wiped) */
                ARGON2_DEFAULT_FLAGS
            };

            /* Run the argon2 computation. */
            int32_t nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                return ~uint512_t(0);

            return hashKey;
        }


        /* Sets the Next Hash from the key */
        void Transaction::NextHash(const uint512_t& hashSecret, const uint8_t nType)
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nType)
            {

                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret))
                        return;

                    /* Calculate the next hash. */
                    hashNext = LLC::SK256(key.GetPubKey());

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return;

                    /* Calculate the next hash. */
                    hashNext = LLC::SK256(key.GetPubKey());

                    break;
                }

                default:
                {
                    /* Unsupported (this is a failure flag). */
                    hashNext = 0;

                    break;
                }
            }
        }


        /* Gets the nextHash for the previous transaction */
        uint256_t Transaction::PrevHash() const
        {
            return LLC::SK256(vchPubKey);
        }


        /* Signs the transaction with the private key and sets the public key */
        bool Transaction::Sign(const uint512_t& hashSecret)
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nKeyType)
            {

                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret))
                        return false;

                    /* Set the public key. */
                    vchPubKey = key.GetPubKey();

                    /* Sign the hash. */
                    return key.Sign(GetHash().GetBytes(), vchSig);
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return false;

                    /* Set the public key. */
                    vchPubKey = key.GetPubKey();

                    /* Sign the hash. */
                    return key.Sign(GetHash().GetBytes(), vchSig);
                }
            }

            return false;
        }


        /* Create a transaction string. */
        std::string Transaction::ToString() const
        {
            return debug::safe_printstr
            (
                IsGenesis() ? "Genesis" : "Tritium", "(",
                "nVersion = ", nVersion, ", ",
                "nSequence = ", nSequence, ", ",
                "nTimestamp = ", nTimestamp, ", ",
                "nextHash  = ",  hashNext.SubString(), ", ",
                "prevHash  = ",  PrevHash().SubString(), ", ",
                "hashPrevTx = ", hashPrevTx.SubString(), ", ",
                "hashGenesis = ", hashGenesis.SubString(), ", ",
                "pub = ", HexStr(vchPubKey).substr(0, 20), ", ",
                "sig = ", HexStr(vchSig).substr(0, 20), ", ",
                "hash = ", GetHash().SubString()
            );
        }


        /* Debug Output. */
        void Transaction::print() const
        {
            debug::log(0, ToString());
        }

        /* Short form of the debug output. */
        std::string Transaction::ToStringShort() const
        {
            std::string str;
            std::string strType = TypeString();
            str += debug::safe_printstr(GetHash().ToString(), " ", strType);
            return str;
        }

        /*  User readable description of the transaction type. */
        std::string Transaction::TypeString() const
        {
            std::string strType = "tritium ";
            if(IsCoinBase())
                strType += "base";
            else if(IsFirst())
                strType += "first";
            else if(IsTrust())
                strType += "trust";
            else if(IsGenesis())
                strType += "genesis";
            else
                strType += "user";

            return strType;
        }

        /* Calculates and returns the total fee included in this transaction */
        uint64_t Transaction::Fees() const
        {
            /* The calculated fee */
            uint64_t nFee = 0;

            /* Thevalue of the contract */
            uint64_t nContractValue = 0;

            /* Iterate through all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Bind the contract to this transaction. */
                contract.Bind(this);

                if(contract.Primitive() == TAO::Operation::OP::FEE)
                {
                    contract.Value(nContractValue);
                    nFee += nContractValue;
                }
            }

            return nFee;
        }
    }
}
