/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/argon2.h>

#include <LLC/include/argon2.h>
#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Operation/include/cost.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/condition.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/build.h>
#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/ambassador.h>
#include <TAO/Ledger/include/developer.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/dispatch.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/merkle.h>
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
        , nVersion     (TAO::Ledger::CurrentTransactionVersion())
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
        , hashCache    (0)
        {
        }


        /* Constructor to set txid cache. */
        Transaction::Transaction(const uint512_t& hashCacheIn)
        : vContracts   ( )
        , nVersion     (TAO::Ledger::CurrentTransactionVersion())
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
        , hashCache    (hashCacheIn)
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
        , hashCache    (tx.hashCache)
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
        , hashCache    (std::move(tx.hashCache))
        {
        }


        /* Copy constructor. */
        Transaction::Transaction(const MerkleTx& tx)
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
        , hashCache    (tx.hashCache)
        {
        }


        /* Move constructor. */
        Transaction::Transaction(MerkleTx&& tx) noexcept
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
        , hashCache    (std::move(tx.hashCache))
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
            hashCache    = tx.hashCache;

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
            hashCache    = std::move(tx.hashCache);

            return *this;
        }


        /* Copy assignment. */
        Transaction& Transaction::operator=(const MerkleTx& tx)
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
            hashCache    = tx.hashCache;

            return *this;
        }


        /* Move assignment. */
        Transaction& Transaction::operator=(MerkleTx&& tx) noexcept
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
            hashCache    = std::move(tx.hashCache);

            return *this;
        }


        /* Default Destructor. */
        Transaction::~Transaction()
        {
        }


        /* Add contracts to the internal vector. */
        Transaction& Transaction::operator<<(const TAO::Operation::Contract& rContract)
        {
            /* We just push to internal vector here. */
            vContracts.push_back(rContract);

            return *this;
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
            vContracts[n].Bind(this, true);

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
            vContracts[n].Bind(this, true);

            return vContracts[n];
        }


        /* Gets the list of contracts internal to transaction. */
        const std::vector<TAO::Operation::Contract>& Transaction::Contracts() const
        {
            return vContracts;
        }


        /* Get the total contracts in transaction. */
        uint32_t Transaction::Size() const
        {
            return vContracts.size();
        }


        /* Determines if the transaction is a valid transaction and passes ledger level checks. */
        bool Transaction::Check(const uint8_t nFlags) const
        {
            /* Check transaction version */
            if(!TransactionVersionActive(nTimestamp, nVersion))
                return debug::error(FUNCTION, "invalid transaction version ", nVersion);

            /* Check for genesis valid numbers. */
            if(hashGenesis == 0)
                return debug::error(FUNCTION, "genesis cannot be zero");

            /* Check for genesis valid numbers. */
            if(hashNext == 0)
                return debug::error(FUNCTION, "nextHash cannot be zero");

            /* Check the timestamp. */
            if(nTimestamp > runtime::unifiedtimestamp() + runtime::maxdrift())
                return debug::error(FUNCTION, "transaction timestamp too far in the future ", nTimestamp);

            /* Check for empty signatures. */
            if(vchSig.size() == 0)
                return debug::error(FUNCTION, "transaction with empty signature");

            /* Check the genesis first byte. */
            if(hashGenesis.GetType() != GENESIS::UserType())
                return debug::error(FUNCTION, "user type [", std::hex, uint32_t(hashGenesis.GetType()), "] invalid, expected [", std::hex, uint32_t(GENESIS::UserType()), "]");

            /* Check for max contracts. */
            if(vContracts.size() > MAX_TRANSACTION_CONTRACTS)
                return debug::error(FUNCTION, "transaction contract limit exceeded", vContracts.size());

            /* Check producer for coinstake transaction */
            if(IsCoinStake() || IsHybrid())
            {
                /* Check for single contract. */
                if(vContracts.size() != 1)
                    return debug::error(FUNCTION, "coinstake cannot exceed one contract");

                /* Check for conditions. */
                if(!vContracts[0].Empty(TAO::Operation::Contract::CONDITIONS))
                    return debug::error(FUNCTION, "coinstake cannot contain conditions");
            }

            /* Count of non-fee contracts in the transaction */
            uint8_t nNames     = 0;
            uint8_t nTrust     = 0;
            uint8_t nAccounts  = 0;
            uint8_t nCrypto    = 0;
            uint8_t nContracts = 0;

            /* Check coinbase. */
            bool fCoinBase = IsCoinBase();
            for(const auto& contract : vContracts)
            {
                /* Check for coinbase. */
                if(fCoinBase)
                {
                    /* Check for foreign operations. */
                    if(contract.Primitive() != TAO::Operation::OP::COINBASE)
                        return debug::error(FUNCTION, "coinbase cannot include foreign ops");

                    /* Check for extra conditions. */
                    if(!contract.Empty(TAO::Operation::Contract::CONDITIONS))
                        return debug::error(FUNCTION, "coinbase cannot include conditions");
                }

                /* Check for empty contracts. */
                if(contract.Empty(TAO::Operation::Contract::OPERATIONS))
                    return debug::error(FUNCTION, "contract is empty");

                /* Skip over fees as counting against total contracts. */
                if(contract.Primitive() != TAO::Operation::OP::FEE)
                    ++nContracts;

                /* For the first transaction. */
                if(IsFirst())
                {
                    /* Check for transaction version 3. */
                    if(!(nFlags & TAO::Ledger::FLAGS::LOOKUP) && nVersion >= 3 && LLD::Ledger->HasFirst(hashGenesis))
                        return debug::error(FUNCTION, "duplicate genesis-id ", hashGenesis.SubString());

                    /* Reset the contract operation stream */
                    contract.Reset();

                    /* Get the Operation */
                    uint8_t nOP = 0;
                    contract >> nOP;

                    /* Check that the operation is a CREATE as these are the only OP's allowed in the genesis transaction */
                    if(nOP == TAO::Operation::OP::CREATE)
                    {
                        /* The register address */
                        TAO::Register::Address address;

                        /* Deserialize the register address from the contract */
                        contract >> address;
                        switch(address.GetType())
                        {
                            /* Tally total accounts created. */
                            case TAO::Register::Address::ACCOUNT:
                                ++nAccounts;
                                break;

                            /* Tally total trust accounts. */
                            case TAO::Register::Address::TRUST:
                                ++nTrust;
                                break;

                            /* Tally total name objects. */
                            case TAO::Register::Address::NAME:
                                ++nNames;
                                break;

                            /* Tally total crypto objects. */
                            case TAO::Register::Address::CRYPTO:
                                ++nCrypto;
                                break;

                            default:
                            {
                                /* Contract is strict for genesis when not in private mode. */
                                if(!config::fHybrid.load())
                                    return debug::error(FUNCTION, "genesis transaction contains invalid contracts.");

                                break;
                            }
                        }
                    }
                    else if(!config::fHybrid.load())
                        return debug::error(FUNCTION, "genesis transaction contains invalid contracts.");

                }
            }

            /* Check contains contracts. */
            if(nContracts == 0)
                return debug::error(FUNCTION, "transaction is empty");

            /* If genesis then check that the only contracts are those for the default registers.
             * We do not make this limitation in private mode */
            if(IsFirst())
            {
                /* Check for main-net proof of work. */
                if(!config::fHybrid.load())
                {
                    //skip proof of work for unit tests
                    #ifndef UNIT_TESTS

                    /* Check the difficulty of the hash. */
                    if(ProofHash() > FIRST_REQUIRED_WORK)
                        return debug::error(FUNCTION, "first transaction not enough work");

                    #endif

                    /* Check that the there are not more than the allowable default contracts */
                    if(vContracts.size() > 5 || nNames > 2 || nTrust > 1 || nAccounts > 1 || nCrypto > 1)
                        return debug::error(FUNCTION, "genesis transaction contains invalid contracts.");
                }

                /* Check our hybrid proofs. */
                else
                {
                    /* Grab our hybrid network-id. */
                    const std::string strHybrid = config::GetArg("-hybrid", "");

                    /* Check our expected values. */
                    const uint512_t hashCheck = LLC::SK512(strHybrid.begin(), strHybrid.end());
                    if(hashCheck != hashPrevTx)
                        return debug::error(FUNCTION, "invalid network-id (", hashPrevTx.ToString().substr(108, 128), ")");
                }
            }

            /* Verify the block signature (if not synchronizing) */
            if(!TAO::Ledger::ChainState::Synchronizing())
            {
                /* Switch based on signature type. */
                switch(nKeyType)
                {
                    /* Support for the FALCON signature scheme. */
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
        static const uint256_t hashConsistencyCheck = uint256_t("0xa15efdcd1969a9a645eda0296b52678f1ef3d9e91ec9f54a4f82f9ab7ce65a6c");
        bool Transaction::CheckTrust(BlockState* pblock) const
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
                    return debug::error(FUNCTION, "last stake ", hashLastClaimed.SubString(), " mismatch ", hashLast.SubString());

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

                /* Check for previous version 7 and current version 8. */
                uint64_t nTrustRet = 0;
                if(pblock->nVersion == 8 && stateLast.nVersion == 7
                && hashGenesis == hashConsistencyCheck &&!CheckConsistency(hashLast, nTrustRet))
                    nTrust = GetTrustScore(nTrustRet, nBlockAge, nStake, nStakeChange, pblock->nVersion);
                else //when consistency is correct, calculate like normal
                    nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange, pblock->nVersion);

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

                /* Update mint values. */
                pblock->nMint += nReward;
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

                /* Update mint values. */
                pblock->nMint += nReward;
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
            if(config::fHybrid.load())
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
            bool fApplyTxFee = (nTimestamp - nPrevTimestamp) < TX_FEE_INTERVAL;

            /* Run through all the contracts. */
            for(auto& contract : vContracts)
            {
                /* Bind the contract to this transaction. */
                contract.Bind(nTimestamp, hashGenesis);

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
                contract.Bind(this, false); //don't bind txid yet, because it depends on build for its final state

                /* Create a temporary map for pre-states. */
                std::map<uint256_t, TAO::Register::State> mapStates;

                /* Run through all the contracts. */
                for(auto& contract : vContracts)
                {
                    /* Bind the contract to this transaction. */
                    contract.Bind(nTimestamp, hashGenesis);

                    /* Calculate the pre-states and post-states. */
                    if(!TAO::Register::Build(contract, mapStates, FLAGS::MEMPOOL))
                        return false;
                }
            }


            //skip proof of work for unit tests
            #ifndef UNIT_TESTS

            /* Check for first. */
            if(IsFirst() && !config::fHybrid.load())
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
        bool Transaction::Connect(const uint8_t nFlags, const BlockState* pblock) const
        {
            /* Get the transaction's hash. */
            const uint512_t hash = GetHash();

            /* flag indicating that transaction fees should apply, depending on the time since the last transaction */
            bool fApplyTxFee = false;

            /* The total cost of this transaction.  This is calculated from executing each contract. */
            uint64_t nCost = 0;

            /* Check for first. */
            if(IsFirst())
            {
                /* Check for transaction version 3. */
                if(nVersion >= 3 && LLD::Ledger->HasFirst(hashGenesis))
                    return debug::error(FUNCTION, "duplicate genesis-id ", hashGenesis.SubString());

                /* Check ambassador sigchains based on all versions, not the smaller subset of versions. */
                for(uint32_t nSwitchVersion = 7; nSwitchVersion <= CurrentBlockVersion(); ++nSwitchVersion)
                {
                    /* Check switch time-lock for version 8. */
                    if(runtime::unifiedtimestamp() < StartBlockTimelock(nSwitchVersion))
                        continue;

                    /* Check for ambassador. */
                    if(Ambassador(nSwitchVersion).find(hashGenesis) != Ambassador(nSwitchVersion).end())
                    {
                        /* Debug logging. */
                        debug::log(1, FUNCTION, "Processing AMBASSADOR sigchain ", hashGenesis.SubString());

                        /* Check that the hashes match. */
                        if(Ambassador(nSwitchVersion).at(hashGenesis).first != PrevHash())
                            return debug::error(FUNCTION, "AMBASSADOR sigchain using invalid credentials");
                    }

                    /* Check for developer. */
                    if(Developer(nSwitchVersion).find(hashGenesis) != Developer(nSwitchVersion).end())
                    {
                        /* Debug logging. */
                        debug::log(1, FUNCTION, "Processing DEVELOPER sigchain ", hashGenesis.SubString());

                        /* Check that the hashes match. */
                        if(Developer(nSwitchVersion).at(hashGenesis).first != PrevHash())
                            return debug::error(FUNCTION, "DEVELOPER sigchain using invalid credentials");
                    }
                }

                /* Write specific transaction flags. */
                if(nFlags == TAO::Ledger::FLAGS::BLOCK)
                {
                    /* Write the genesis identifier. */
                    if(!LLD::Ledger->WriteFirst(hashGenesis, hash))
                        return debug::error(FUNCTION, "failed to write genesis");
                }
            }
            else
            {
                /* We want this to trigger for times not in -client mode. */
                if(!config::fClient.load() || (TAO::API::Authentication::Active(hashGenesis) && nFlags != FLAGS::LOOKUP))
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
                }
            }

            /* Keep for dependants. */
            uint512_t hashPrev = 0;
            uint32_t nContract = 0;

            /* Run through all the contracts. */
            std::set<uint256_t> setAddresses;
            for(const auto& contract : vContracts)
            {
                /* Check for dependants. */
                if(contract.Dependant(hashPrev, nContract) && nFlags != FLAGS::LOOKUP)
                {
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
                            if(nConfirms + 1 < MaturityCoinBase((pblock ? *pblock : ChainState::tStateBest.load())))
                                return debug::error(FUNCTION, "coinbase is immature ", nConfirms);

                            break;
                        }
                    }

                    /* Check that the previous transaction is indexed. */
                    if((nFlags == FLAGS::BLOCK || nFlags == FLAGS::MINER) && !LLD::Ledger->HasIndex(hashPrev))
                        return debug::error(FUNCTION, hashPrev.SubString(), " not indexed");
                }

                /* Bind the contract to this transaction. */
                contract.Bind(this, hash);

                /* Execute the contracts to final state. */
                if(!TAO::Operation::Execute(contract, nFlags, nCost))
                    return false;

                /* If transaction fees should apply, calculate the additional transaction cost for the contract */
                if(fApplyTxFee)
                    TAO::Operation::TxCost(contract, nCost);

                /* Index our registers here now if not -client mode and setting enabled. */
                if(!config::fClient.load() && config::fIndexRegister.load())
                {
                    /* Unpack the address we will be working on. */
                    uint256_t hashAddress;
                    if(!TAO::Register::Unpack(contract, hashAddress))
                        continue;

                    /* Check for duplicate entries. */
                    if(setAddresses.count(hashAddress))
                        continue;

                    /* Check fo register in database. */
                    if(!LLD::Logical->PushRegisterTx(hashAddress, hash))
                        debug::warning(FUNCTION, "failed to push register tx ", TAO::Register::Address(hashAddress).ToString());

                    /* Push the address now. */
                    setAddresses.insert(hashAddress);
                }
            }

            /* Once we have executed the contracts we need to check the fees. */
            if(!config::fHybrid.load())
            {
                /* The fee applied to this transaction */
                uint64_t nFees = 0;
                if(IsFirst())
                {
                    /* For the genesis transaction we allow a fixed amount of default registers to be created for free. */
                    nFees = 2 * TAO::Register::ACCOUNT_FEE    // 2 accounts
                          + 2 * TAO::Register::NAME_FEE       // 2 names
                          + 1 * TAO::Register::CRYPTO_FEE;    // 1 crypto register
                }
                else
                    /* For all other transactions we check the actual fee contracts included in the transaction */
                    nFees = Fees();

                /* Check that the fees match.  */
                if(nCost > nFees)
                    return debug::error(FUNCTION, "not enough fees supplied ", nFees);
            }

            /* Write the last to disk. */
            if(nFlags == FLAGS::BLOCK && !LLD::Ledger->WriteLast(hashGenesis, hash))
                return debug::error(FUNCTION, "failed to write last hash");

            return true;
        }


        /* Disconnect a transaction object to the main chain. */
        bool Transaction::Disconnect(const uint8_t nFlags)
        {
            /* Disconnect on block needs to manage ledger level indexes. */
            if(nFlags == FLAGS::BLOCK)
            {
                /* Erase last for genesis. */
                if(IsFirst())
                {
                    /* Erase our last hash now. */
                    if(!LLD::Ledger->EraseLast(hashGenesis))
                        return debug::error(FUNCTION, "failed to erase last hash");

                    /* Erase our genesis-id now. */
                    if(!LLD::Ledger->EraseFirst(hashGenesis))
                        return debug::error(FUNCTION, "failed to erase genesis");
                }

                /* Write proper last hash index. */
                else if(!LLD::Ledger->WriteLast(hashGenesis, hashPrevTx))
                    return debug::error(FUNCTION, "failed to write last hash");

                /* Revert last stake when disconnect a coinstake tx */
                if(IsCoinStake())
                {
                    /* Handle for trust keys. */
                    if(IsTrust())
                    {
                        /* Extract the last stake hash from the coinstake contract */
                        uint512_t hashLast = 0;
                        if(!vContracts[0].Previous(hashLast))
                            return debug::error(FUNCTION, "failed to extract last stake hash from contract");

                        /* Revert saved last stake to the prior stake transaction */
                        if(!LLD::Ledger->WriteStake(hashGenesis, hashLast))
                            return debug::error(FUNCTION, "failed to write last stake");

                        /* If local database has a stake change request, update it to not processed. */
                        StakeChange tRequest;
                        if(LLD::Local->ReadStakeChange(hashGenesis, tRequest))
                        {
                            /* Check for processed change that's also for this transaction. */
                            if(tRequest.fProcessed && tRequest.hashTx == GetHash()) //XXX: this is ugly, needs improvement
                            {
                                /* Set the stake request to not processed now. */
                                tRequest.fProcessed = false;
                                tRequest.hashTx = 0;

                                /* Update stake change request on disconnect. */
                                if(!LLD::Local->WriteStakeChange(hashGenesis, tRequest))
                                    debug::error(FUNCTION, "unable to reinstate disconnected stake change request"); //don't fail
                            }
                        }
                    }
                    else
                    {
                        /* Remove last stake indexing if disconnect a genesis transaction */
                        if(!LLD::Ledger->EraseStake(hashGenesis))
                            return debug::error(FUNCTION, "failed to erase last stake");
                    }
                }
            }

            /* Run through all the contracts in reverse order to disconnect. */
            std::set<uint256_t> setAddresses;
            for(auto contract = vContracts.rbegin(); contract != vContracts.rend(); ++contract)
            {
                contract->Bind(this);
                if(!TAO::Register::Rollback(*contract, nFlags))
                    return false;

                /* Erase our register index here now if not -client mode and setting enabled. */
                if(!config::fClient.load() && config::fIndexRegister.load())
                {
                    /* Unpack the address we will be working on. */
                    uint256_t hashAddress;
                    if(!TAO::Register::Unpack(*contract, hashAddress))
                        continue;

                    /* Check for duplicate entries. */
                    if(setAddresses.count(hashAddress))
                        continue;

                    /* Check fo register in database. */
                    if(!LLD::Logical->EraseRegisterTx(hashAddress))
                        debug::warning(FUNCTION, "failed to erase register tx ", TAO::Register::Address(hashAddress).ToString());

                    /* Push the address now. */
                    setAddresses.insert(hashAddress);
                }
            }

            return true;
        }


        /* Determines if the transaction is a coinbase transaction. */
        bool Transaction::IsCoinBase() const
        {
            /* Check all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for occurrence of coinbase operation. */
                if(contract.Primitive() == TAO::Operation::OP::COINBASE)
                    return true;
            }

            return false;
        }


        /* Determines if the transaction is a coinstake (trust or genesis) transaction. */
        bool Transaction::IsCoinStake() const
        {
            return (IsTrust() || IsGenesis());
        }


        /* Determines if the transaction is for a private block. */
        bool Transaction::IsHybrid() const
        {
            /* Check all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for occurrence of authorize operation. */
                if(contract.Primitive() == TAO::Operation::OP::AUTHORIZE)
                    return true;
            }

            return false;
        }


        /* Determines if the transaction is a solo staking trust transaction. */
        bool Transaction::IsTrust() const
        {
            /* Check all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for occurrence of trust operation. */
                if(contract.Primitive() == TAO::Operation::OP::TRUST)
                    return true;
            }

            return false;
        }


        /* Determines if the transaction is a solo staking genesis transaction */
        bool Transaction::IsGenesis() const
        {
            /* Check all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Check for occurrence of genesis operation. */
                if(contract.Primitive() == TAO::Operation::OP::GENESIS)
                    return true;
            }

            return false;
        }


        /* Determines if the transaction is a genesis transaction */
        bool Transaction::IsFirst() const
        {
            return nSequence == 0;
        }


        /* Determines if the transaction has been confirmed in the main chain. */
        bool Transaction::IsConfirmed() const
        {
            return LLD::Ledger->HasIndex(GetHash());
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
        uint512_t Transaction::GetHash(const bool fCacheOverride) const
        {
            /* Check if we have an active cache. */
            //if(hashCache != 0 && !fCacheOverride)
            //    return hashCache;

            /* Serialize the transaction data for hashing. */
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            /* Type of 0xff designates tritium tx. */
            hashCache = LLC::SK512(ss.begin(), ss.end());
            hashCache.SetType(TAO::Ledger::TRITIUM);

            return hashCache;
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
        void Transaction::NextHash(const uint512_t& hashSecret)
        {
            /* Set the next hash if void function. */
            hashNext = Transaction::NextHash(hashSecret, nNextType);
        }


        /* Calculates a next-hash from given secret key. */
        uint256_t Transaction::NextHash(const uint512_t& hashSecret, const uint8_t nType)
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nType)
            {
                /* Support for the FALCON signature scheme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret))
                        return 0;

                    /* Calculate the next hash. */
                    return LLC::SK256(key.GetPubKey());
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return 0;

                    /* Calculate the next hash. */
                    return LLC::SK256(key.GetPubKey());
                }
            }

            return 0;
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
                /* Support for the FALCON signature scheme. */
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
                "hashGenesis = ", hashGenesis.ToString(), ", ",
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

            /* The value of the contract */
            uint64_t nContractValue = 0;

            /* Iterate through all contracts. */
            for(const auto& contract : vContracts)
            {
                /* Bind the contract to this transaction. */
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
