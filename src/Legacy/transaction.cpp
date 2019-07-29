/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <Legacy/types/transaction.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/include/signature.h>
#include <Legacy/include/trust.h>

#include <Legacy/types/legacy.h>
#include <Legacy/types/script.h>
#include <Legacy/types/trustkey.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>

namespace Legacy
{

    /* Old legacy outdated threshold, currently a placeholder. */
    const int64_t LOCKTIME_THRESHOLD = 500000000;


    /** Copy Constructor (From Tritium). **/
    Transaction::Transaction(const TAO::Ledger::Transaction& tx)
    : nVersion(tx.nVersion)
    , nTime(tx.nTimestamp)
    , vin()
    , vout()
    , nLockTime(0)
    {
        SetNull();

        /* Loop through the contracts. */
        for(uint32_t n = 0; n < tx.Size(); ++n)
        {
            /* Get legacy converted output.*/
            TxOut txout;
            tx[n].Legacy(txout); //a failure here will result in a null output

            /* Add the output. */
            vout.push_back(txout);
        }
    }


	/* Sets the transaciton object to a null state. */
	void Transaction::SetNull()
	{
		nVersion = 1;
		nTime = (uint32_t)runtime::unifiedtimestamp();
		vin.clear();
		vout.clear();
		nLockTime = 0;
	}


	/* Determines if object is in a null state. */
	bool Transaction::IsNull() const
	{
		return (vin.empty() && vout.empty());
	}


	/* Returns the hash of this object. */
	uint512_t Transaction::GetHash() const
	{
        // Most of the time is spent allocating and deallocating DataStream's
	    // buffer.  If this ever needs to be optimized further, make a CStaticStream
	    // class with its buffer on the stack.
	    DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
	    ss.reserve(10000);
	    ss << *this;

        /* Get the hash. */
	    uint512_t hash = LLC::SK512(ss.begin(), ss.end());

        /* Type of 0xfe designates legacy tx. */
        if(TAO::Ledger::VersionActive(nTime, 7))
            hash.SetType(TAO::Ledger::LEGACY);

        return hash;
	}


	/* Determine if a transaction is within LOCKTIME_THRESHOLD */
	bool Transaction::IsFinal(int32_t nBlockHeight, int64_t nBlockTime) const
	{
		// Time based nLockTime implemented in 0.1.6
		if(nLockTime == 0)
			return true;

		if(nBlockHeight == 0)
			nBlockHeight = TAO::Ledger::ChainState::nBestHeight.load();

		if(nBlockTime == 0)
			nBlockTime = runtime::unifiedtimestamp();

		if((int64_t)nLockTime < ((int64_t)nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
			return true;

		for(const auto& txin : vin)
			if(!txin.IsFinal())
				return false;

		return true;
	}


	/* Determine if a transaction is newer than supplied argument. */
	bool Transaction::IsNewerThan(const Transaction& old) const
	{
        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());

		if(nSize != old.vin.size())
			return false;

		for(uint32_t i = 0; i < nSize; ++i)
			if(vin[i].prevout != old.vin[i].prevout)
				return false;

		bool fNewer = false;
		uint32_t nLowest = std::numeric_limits<uint32_t>::max();
		for(uint32_t i = 0; i < nSize; ++i)
		{
			if(vin[i].nSequence != old.vin[i].nSequence)
			{
				if(vin[i].nSequence <= nLowest)
				{
					fNewer = false;
					nLowest = vin[i].nSequence;
				}
				if(old.vin[i].nSequence < nLowest)
				{
					fNewer = true;
					nLowest = old.vin[i].nSequence;
				}
			}
		}

		return fNewer;
	}


	/* Check the flags that denote a coinbase transaction. */
	bool Transaction::IsCoinBase() const
	{
		/* Input Size must be 1. */
		if(vin.size() != 1)
			return false;

		/* First Input must be Empty. */
		if(!vin[0].prevout.IsNull())
			return false;

		/* Outputs Count must Exceed 1. */
		if(vout.size() < 1)
			return false;

		return true;
	}


	/* Check the flags that denote a coinstake transaction. */
	bool Transaction::IsCoinStake() const
	{
        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());

		/* Must have more than one Input. */
		if(nSize <= 1)
			return false;

		/* First Input Script Signature must Contain Fibanacci Byte Series. */
		if(!vin[0].IsStakeSig())
			return false;

		/* All Remaining Previous Inputs must not be Empty. */
		for(uint32_t nIndex = 1; nIndex < nSize; ++nIndex)
			if(vin[nIndex].prevout.IsNull())
				return false;

		/* Must Contain only 1 Outputs. */
		if(vout.size() != 1)
			return false;

		return true;
	}


	/* Check the flags that denote a genesis transaction. */
	bool Transaction::IsGenesis() const
	{
		/* Genesis Transaction must be Coin Stake. */
		if(!IsCoinStake())
			return false;

		/* First Input Previous Transaction must be Empty. */
		if(!vin[0].prevout.IsNull())
			return false;

		return true;
	}


	/* Check the flags that denote a trust transaction. */
	bool Transaction::IsTrust() const
	{
		/* Genesis Transaction must be Coin Stake. */
		if(!IsCoinStake())
			return false;

		/* First Input Previous Transaction must not be Empty. */
		if(vin[0].prevout.IsNull())
			return false;

		/* First Input Previous Transaction Hash must not be 0. */
		if(vin[0].prevout.hash == 0)
			return false;

		/* First Input Previous Transaction Index must be 0. */
		if(vin[0].prevout.n != 0)
			return false;

		return true;
	}


	/* Check for standard transaction types */
	bool Transaction::IsStandard() const
    {
        for(const auto& txin : vin)
        {
            // Biggest 'standard' txin is a 3-signature 3-of-3 CHECKMULTISIG
            // pay-to-script-hash, which is 3 ~80-byte signatures, 3
            // ~65-byte public keys, plus a few script ops.
            if(txin.scriptSig.size() > 500)
                return false;

            if(!txin.scriptSig.IsPushOnly())
                return false;
        }

        //TODO: we need to add UTXO to Tritium script as Standard here

        for(const auto& txout : vout)
            if(!Legacy::IsStandard(txout.scriptPubKey))
                return false;

        return true;
    }

    /* Extract the trust key out of the coinstake transaction. */
    bool Transaction::TrustKey(std::vector<uint8_t>& vchTrustKey) const
    {
        /* Extract the Key from the Script Signature. */
        std::vector<std::vector<uint8_t> > vSolutions;
        TransactionType whichType;

        /* Extract the key from script sig. */
        if(!Solver(vout[0].scriptPubKey, whichType, vSolutions))
            return debug::error(FUNCTION, "couldn't find trust key in script");

        /* Enforce public key rules. */
        if(whichType != TX_PUBKEY)
            return debug::error(FUNCTION, "key not of public key type");

        /* Set the Public Key Integer Key from Bytes. */
        vchTrustKey = vSolutions[0];

        return true;
    }


    /* Extract the trust key out of the coinstake transaction. */
    bool Transaction::TrustKey(uint576_t& cKey) const
    {
        /* Extract the trust key. */
        std::vector<uint8_t> vchTrustKey;
        if(!TrustKey(vchTrustKey))
            return debug::error(FUNCTION, "trust key failed to extract");

        /* Set the bytes for the key object. */
        cKey.SetBytes(vchTrustKey);
        return true;

    }


    /* Extract the trust score out of the coinstake transaction. */
    bool Transaction::TrustScore(uint32_t& nScore) const
    {
        /* Extract the trust key. */
        uint1024_t hashBlock;

        /* Get the sequence. */
        uint32_t nSequence;

        /* Extract the trust score from vin. */
        if(!ExtractTrust(hashBlock, nSequence, nScore))
            return debug::error(FUNCTION, "failed to get trust score");

        return true;
    }


    /* Extract the trust data from the input script. */
    bool Transaction::ExtractTrust(uint1024_t& hashLastBlock, uint32_t& nSequence, uint32_t& nTrustScore) const
    {
        /* Don't extract trust if not coinstake. */
        if(!IsCoinStake())
            return debug::error(FUNCTION, "not proof of stake");

        /* Check the script size matches expected length. */
        if(vin[0].scriptSig.size() != 144)
            return debug::error(FUNCTION, "script not 144 bytes");

        /* Put script in deserializing stream. */
        DataStream scriptPub(vin[0].scriptSig.begin() + 8, vin[0].scriptSig.end(), SER_NETWORK, LLP::PROTOCOL_VERSION);

        /* Erase the first 8 bytes of the fib byte series flag. */
        //scriptPub.erase(scriptPub.begin(), scriptPub.begin() + 8);

        /* Deserialize the values from stream. */
        scriptPub >> hashLastBlock >> nSequence >> nTrustScore;

        return true;
    }


    /* Age is determined by average time from previous transactions. */
    bool Transaction::CoinstakeAge(uint64_t& nAge) const
    {
        /* Output figure to show the amount of coins being staked at their interest rates. */
        nAge = 0;

        /* Check that the transaction is Coinstake. */
        if(!IsCoinStake())
            return false;

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());

        /* Check the coin age of each Input. */
        for(uint32_t nIndex = 1; nIndex < nSize; ++nIndex)
        {
            /* Calculate the Age and Value of given output. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::Ledger->ReadBlock(vin[nIndex].prevout.hash, statePrev))
                return debug::error(FUNCTION, "no block for previous transaction");

            /* Time is from current transaction to previous block time. */
            uint64_t nCoinAge = (nTime - statePrev.GetBlockTime());

            /* Compound the Total Figures. */
            nAge += nCoinAge;
        }

        nAge /= (nSize - 1);

        return true;
    }


    /* Get the total calculated coinstake reward for a Proof of Stake block */
    bool Transaction::CoinstakeReward(const TAO::Ledger::BlockState& block, uint64_t& nStakeReward) const
    {
        /* Check that the transaction is Coinstake. */
        if(!IsCoinStake())
            return debug::error(FUNCTION, "not coinstake transaction");

        /* Extract the Key from the Script Signature. */
        uint576_t cKey;
        if(!TrustKey(cKey))
            return debug::error(FUNCTION, "trust key couldn't be extracted");

        /* Output figure to show the amount of coins being staked at their interest rates. */
        uint64_t nTotalCoins = 0, nAverageAge = 0;
        nStakeReward = 0;

        /* Calculate the Variable Interest Rate for Given Coin Age Input. */
        double nStakeRate = 0.05; //genesis interest rate
        if(block.nVersion >= 6)
            nStakeRate = 0.005;

        /* Get the trust key from index database. */
        if(!IsGenesis() || block.nVersion >= 6)
        {
            /* Read the trust key from the disk. */
            Legacy::TrustKey trustKey;
            if(LLD::Trust->ReadTrustKey(cKey, trustKey))
            {
                /* This method can be called on a candidate block to calculate reward for it,
                 * so don't use the StakeRate version that takes the BlockState. This would
                 * fail because coinstake tx not in database. But this tx is the coinstake tx
                 * so can call the version that takes tx directly and pass *this
                 */
                nStakeRate = trustKey.StakeRate(block, nTime);
            }

            /* Check if it failed to read and this is genesis. */
            else if(!IsGenesis())
                return debug::error(FUNCTION, "unable to read trust key");
        }

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());

        /** Check the coin age of each Input. **/
        for(uint32_t nIndex = 1; nIndex < nSize; ++nIndex)
        {
            /* Calculate the Age and Value of given output. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::Ledger->ReadBlock(vin[nIndex].prevout.hash, statePrev))
            {
                if (block.hashPrevBlock == 0)
                {
                    /* Current state is not connected (eg, this is a candidate block)
                     * so begin repair from best state
                     */
                    statePrev = TAO::Ledger::ChainState::stateBest.load();
                }
                else
                    statePrev = block;

                if(!LLD::Ledger->RepairIndex(vin[nIndex].prevout.hash, statePrev))
                    return debug::error(FUNCTION, "failed to read previous tx block");
            }

            /* Read the previous transaction. */
            Legacy::Transaction txPrev;
            if(!LLD::Legacy->ReadTx(vin[nIndex].prevout.hash, txPrev))
                return debug::error(FUNCTION, "failed to read previous tx");

            /* Calculate the Age and Value of given output. */
            uint64_t nCoinAge = (nTime - statePrev.GetBlockTime());
            uint64_t nValue = txPrev.vout[vin[nIndex].prevout.n].nValue;

            /* Compound the Total Figures. */
            nTotalCoins += nValue;
            nAverageAge += nCoinAge;

            /* Reward rate for time period is annual rate * (time period / annual time) = nStakeRate * (nCoinAge / ONE_YEAR)
             * Then, nStakeReward = nValue * reward rate.
             *
             * But, while easier to understand if write as nValue * nStakeRate * (nCoinAge / ONE_YEAR)
             * you can't do that because this is integer math. It will do division first, which will be zero
             *
             * Instead, write as (nValue * nStakeRate * nCoinAge) / ONE_YEAR
             * thus saving any truncation from integer division until the final result.
             *
             * Formerly, this calculation used TAO::Ledger::TRUST_SCORE_MAX which was set to same value
             * as ONE_YEAR. However, rate is ALWAYS annual, regardless of the possible max trust score which
             * could at some point be changed. Thus, ONE_YEAR was defined and used here instead.
             */
            nStakeReward += ((nValue * nStakeRate * nCoinAge) / TAO::Ledger::ONE_YEAR);
        }

        nAverageAge /= (nSize - 1);

        return true;
    }


	/* Check for standard transaction types */
	bool Transaction::AreInputsStandard(const std::map<uint512_t, std::pair<uint8_t, DataStream> >& mapInputs) const
    {
        if(IsCoinBase())
            return true; // Coinbases don't use vin

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());
        for(uint32_t i = (uint32_t) IsCoinStake(); i < nSize; ++i)
        {
            const TxOut& prev = GetOutputFor(vin[i], mapInputs);

            std::vector< std::vector<uint8_t> > vSolutions;
            TransactionType whichType;

            // get the scriptPubKey corresponding to this input:
            const Script& prevScript = prev.scriptPubKey;
            if(!Solver(prevScript, whichType, vSolutions))
                return false;

            int nArgsExpected = ScriptSigArgsExpected(whichType, vSolutions);
            if(nArgsExpected < 0)
                return false;

            // Transactions with extra stuff in their scriptSigs are
            // non-standard. Note that this EvalScript() call will
            // be quick, because if there are any operations
            // beside "push data" in the scriptSig the
            // IsStandard() call returns false
            std::vector< std::vector<uint8_t> > stack;
            if(!EvalScript(stack, vin[i].scriptSig, *this, i, 0))
                return false;

            if(whichType == TX_SCRIPTHASH)
            {
                if(stack.empty())
                    return false;

                Script subscript(stack.back().begin(), stack.back().end());
                std::vector< std::vector<uint8_t> > vSolutions2;
                TransactionType whichType2;
                if(!Solver(subscript, whichType2, vSolutions2))
                    return false;
                if(whichType2 == TX_SCRIPTHASH)
                    return false;

                int tmpExpected;
                tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
                if(tmpExpected < 0)
                    return false;
                nArgsExpected += tmpExpected;
            }

            if(stack.size() != (uint32_t)nArgsExpected)
                return false;
        }

        return true;
    }


	/* Count ECDSA signature operations the old-fashioned (pre-0.6) way */
	uint32_t Transaction::GetLegacySigOpCount() const
    {
        uint32_t nSigOps = 0;
        for(const auto& txin : vin)
        {
            if(txin.IsStakeSig())
                continue;

            nSigOps += txin.scriptSig.GetSigOpCount(false);
        }

        for(const auto& txout : vout)
        {
            nSigOps += txout.scriptPubKey.GetSigOpCount(false);
        }

        return nSigOps;
    }


	/* Count ECDSA signature operations in pay-to-script-hash inputs. */
	uint32_t Transaction::TotalSigOps(const std::map<uint512_t, std::pair<uint8_t, DataStream> >& mapInputs) const
    {
        if(IsCoinBase())
            return 0;

        uint32_t nSigOps = 0;

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());

        for(uint32_t i = (uint32_t)IsCoinStake(); i < nSize; ++i)
        {
            const TxOut& prevout = GetOutputFor(vin[i], mapInputs);
            nSigOps += prevout.scriptPubKey.GetSigOpCount(vin[i].scriptSig);
        }

        return nSigOps;
    }


	/* Amount of Coins spent by this transaction. */
	uint64_t Transaction::GetValueOut() const
	{
		int64_t nValueOut = 0;
		for(const auto& txout : vout)
		{
			nValueOut += txout.nValue;
			if(!MoneyRange(txout.nValue) || !MoneyRange(nValueOut))
				throw std::runtime_error("Transaction::GetValueOut() : value out of range");
		}

		return nValueOut;
	}


	/* Amount of Coins coming in to this transaction */
	uint64_t Transaction::GetValueIn(const std::map<uint512_t, std::pair<uint8_t, DataStream> >& mapInputs) const
    {
        if(IsCoinBase())
            return 0;

        uint64_t nResult = 0;

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());

        for(uint32_t i = (uint32_t) IsCoinStake(); i < nSize; ++i)
        {
            nResult += GetOutputFor(vin[i], mapInputs).nValue;
        }

        return nResult;
    }


	/* Determine if transaction qualifies to be free. */
	bool Transaction::AllowFree(double dPriority)
	{
		// Large (in bytes) low-priority (new, small-coin) transactions
		// need a fee.
		return dPriority > COIN * 144 / 250;
	}


	/* Get the minimum fee to pay for broadcast. */
	uint64_t Transaction::GetMinFee(uint32_t nBlockSize, bool fAllowFree, enum GetMinFee_mode mode) const
    {

        /* Base fee is either MIN_TX_FEE or MIN_RELAY_TX_FEE */
        int64_t  nBaseFee = (mode == GMF_RELAY) ? MIN_RELAY_TX_FEE : MIN_TX_FEE;
        uint32_t nBytes = ::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION);
        uint32_t nNewBlockSize = nBlockSize + nBytes;
        int64_t  nMinFee = (1 + (int64_t)nBytes / 1000) * nBaseFee;

        /* Check if within bounds of free transactions. */
        if(fAllowFree)
        {
            if(nBlockSize == 1)
            {
                // Transactions under 10K are free
                // (about 4500bc if made of 50bc inputs)
                if(nBytes < 10000)
                    nMinFee = 0;
            }
            else
            {
                // Free transaction area
                if(nNewBlockSize < 27000)
                    nMinFee = 0;
            }
        }

        /* To limit dust spam, require MIN_TX_FEE/MIN_RELAY_TX_FEE if any output is less than 0.01 */
        if(nMinFee < nBaseFee)
        {
            for(const auto& txout : vout)
                if(txout.nValue < CENT)
                {
                    nMinFee = nBaseFee;
                    break;
                }
        }

        /* Raise the price as the block approaches full */
        if(nBlockSize != 1 && nNewBlockSize >= TAO::Ledger::MAX_BLOCK_SIZE_GEN / 2)
        {
            if(nNewBlockSize >= TAO::Ledger::MAX_BLOCK_SIZE_GEN)
                return MaxTxOut();

            nMinFee *= TAO::Ledger::MAX_BLOCK_SIZE_GEN / (TAO::Ledger::MAX_BLOCK_SIZE_GEN - nNewBlockSize);
        }

        if(!MoneyRange(nMinFee))
            nMinFee = MaxTxOut();

        return nMinFee;
    }


	/* Short form of the debug output. */
	std::string Transaction::ToStringShort() const
    {
        std::string str;
        std::string txtype = TypeString();

        str += debug::safe_printstr(GetHash().ToString(), " ", txtype);
        return str;
    }


    /*  User readable description of the transaction type. */
    std::string Transaction::TypeString() const
    {
        std::string txtype = "legacy ";
        if(IsCoinBase())
            txtype += "base";
        else if(IsTrust())
            txtype += "trust";
        else if(IsGenesis())
            txtype += "genesis";
        else
            txtype += "user";

        return txtype;
    }


	/* Long form of the debug output. */
	std::string Transaction::ToString() const
    {
        std::string str;
        str += IsCoinBase() ? "Coinbase" : (IsGenesis() ? "Genesis" : (IsTrust() ? "Trust" : "Transaction"));
        str += debug::safe_printstr(
            "(hash=", GetHash().SubString(10),
            ", nTime=", nTime,
            ", ver=", nVersion,
            ", vin.size=", vin.size(),
            ", vout.size=", vout.size(),
            ", nLockTime=", nLockTime, ")\n");

        for(const auto& txin : vin)
            str += "    " + txin.ToString() + "\n";
        for(const auto& txout : vout)
            str += "    " + txout.ToString() + "\n";
        return str;
    }


	/* Dump the transaction data to the console. */
	void Transaction::print() const
    {
        debug::log(0, ToString());
    }


	/* Check the transaction for validity. */
	bool Transaction::CheckTransaction() const
    {
        /* Check for empty inputs. */
        if(vin.empty())
            return debug::error(FUNCTION, "vin empty");

        /* Check for empty outputs. */
        if(vout.empty())
            return debug::error(FUNCTION, "vout empty");

        /* Check for size limits. */
        if(::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > TAO::Ledger::MAX_BLOCK_SIZE)
            return debug::error(FUNCTION, "size limits failed");

        /* Determine if Transaction is CoinStake or CoinBase. */
        bool fIsCoinBase  = IsCoinBase();
        bool fIsCoinStake = IsCoinStake();

        /* Check for negative or overflow output values */
        int64_t nValueOut = 0;
        for(const auto& txout : vout)
        {
            /* Determine if txout is empty. */
            bool fTxOutIsEmpty = txout.IsEmpty();

            /* Checkout for empty outputs. */
            if(fTxOutIsEmpty && (!fIsCoinBase && !fIsCoinStake))
                return debug::error(FUNCTION, "txout empty for user transaction");

            /* Enforce minimum output amount. */
            if(!fTxOutIsEmpty && txout.nValue < MIN_TXOUT_AMOUNT)
                return debug::error(FUNCTION, "txout.nValue below minimum");

            /* Enforce maximum output amount. */
            if(txout.nValue > MaxTxOut())
                return debug::error(FUNCTION, "txout.nValue too high");

            /* Enforce maximum total output value. */
            nValueOut += txout.nValue;
            if(!MoneyRange(nValueOut))
                return debug::error(FUNCTION, "txout total out of range");
        }

        /* Check for duplicate inputs */
        std::set<OutPoint> vInOutPoints;
        for(const auto& txin : vin)
        {
            if(vInOutPoints.count(txin.prevout))
                return false;

            vInOutPoints.insert(txin.prevout);
        }

        /* Check for null previous outputs. */
        if(!fIsCoinBase && !fIsCoinStake)
        {
            for(const auto& txin : vin)
                if(txin.prevout.IsNull())
                    return debug::error(FUNCTION, "prevout is null");
        }

        return true;
    }


    /* Get the inputs for a transaction. */
    bool Transaction::FetchInputs(std::map<uint512_t, std::pair<uint8_t, DataStream> >& inputs) const
    {
        /* Coinbase has no inputs. */
        if(IsCoinBase())
            return true;

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());
        for(uint32_t i = (uint32_t)IsCoinStake(); i < nSize; ++i)
        {
            /* Skip inputs that are already found. */
            OutPoint prevout = vin[i].prevout;
            if(inputs.count(prevout.hash))
                continue;

            /* Check for Tritium version transactions.
             * Becomes ACTIVE 2 hours after time-lock.
             */
            if(TAO::Ledger::VersionActive(nTime + 7200, 7))
            {
                /* Get the type of transaction. */
                if(prevout.hash.GetType() == TAO::Ledger::TRITIUM)
                {
                    /* Read the previous transaction. */
                    TAO::Ledger::Transaction txPrev;
                    if(LLD::Ledger->ReadTx(prevout.hash, txPrev))
                    {   //we can't rely soley on the type byte, so we must revert to legacy if not found in ledger.}

                        /* Check for existing indexes. */
                        if(!txPrev.IsConfirmed())
                            return debug::error(FUNCTION, "tritium tx ", prevout.hash.ToString().substr(0, 20), " not confirmed");

                        /* Check that it is valid. */
                        if(prevout.n >= txPrev.Size())
                            return debug::error(FUNCTION, "prevout ", prevout.n, " is out of range ", txPrev.Size());

                        /* Check for Legacy. */
                        if(txPrev[prevout.n].Primitive() != TAO::Operation::OP::LEGACY)
                            return debug::error(FUNCTION, "can't spend from UTXO with no OP::LEGACY");

                        /* Add to the inputs. */
                        inputs.emplace(prevout.hash, std::make_pair(uint8_t(TAO::Ledger::TRITIUM), DataStream(SER_LLD, LLD::DATABASE_VERSION)));
                        inputs.at(prevout.hash).second << txPrev;

                        continue;
                    }
                }
            }

            /* Read the previous transaction. */
            Transaction txPrev;
            if(!LLD::Legacy->ReadTx(prevout.hash, txPrev))
                return debug::error(FUNCTION, "legacy tx ", prevout.hash.ToString().substr(0, 20), " not found");

            /* Check for existing indexes. */
            if(!LLD::Ledger->HasIndex(prevout.hash))
                return debug::error(FUNCTION, "tx ", prevout.hash.ToString().substr(0, 20), " not connected");

            /* Check that it is valid. */
            if(prevout.n >= txPrev.vout.size())
                return debug::error(FUNCTION, "prevout ", prevout.n, " is out of range ", txPrev.vout.size());

            /* Add to the inputs. */
            inputs.emplace(prevout.hash, std::make_pair(uint8_t(TAO::Ledger::LEGACY), DataStream(SER_LLD, LLD::DATABASE_VERSION)));
            inputs.at(prevout.hash).second << txPrev;
        }

        return true;
    }


    /* Mark the inputs in a transaction as spent. */
    bool Transaction::Connect(const std::map<uint512_t, std::pair<uint8_t, DataStream> >& inputs, TAO::Ledger::BlockState& state, uint8_t nFlags) const
    {
        /* Determine if Transaction is CoinStake or CoinBase. */
        bool fIsCoinBase = IsCoinBase();
        bool fIsCoinStake = IsCoinStake();

        /* Special checks for coinbase and coinstake. */
        if(fIsCoinStake || fIsCoinBase)
        {
            /* Check the input script size. */
            if(vin[0].scriptSig.size() < 2 || vin[0].scriptSig.size() > (state.nVersion < 5 ? 100 : 144))
                return debug::error(FUNCTION, "coinbase/coinstake script invalid size ", vin[0].scriptSig.size());

            /* Coinbase has no inputs. */
            if(fIsCoinBase)
            {
                /* Calculate the mint when on a block. */
                if(nFlags == FLAGS::BLOCK)
                    state.nMint = GetValueOut();

                return true;
            }

            /* When reach here, tx is not a coinbase (just processed), so the rest handles coinstake */

            /* Get the trust key. */
            std::vector<uint8_t> vTrustKey;
            if(!TrustKey(vTrustKey))
                return debug::error(FUNCTION, "can't extract trust key.");

            /* Check for trust key. */
            uint576_t cKey;
            cKey.SetBytes(vTrustKey);

            /* Handle Genesis Transaction Rules. Genesis is checked after Trust Key Established. */
            Legacy::TrustKey trustKey;
            if(IsGenesis())
            {
                /* Create the Trust Key from Genesis Transaction Block. */
                trustKey = Legacy::TrustKey(vTrustKey, state.GetHash(), GetHash(), state.GetBlockTime());

                /* Check the genesis transaction. */
                if(!trustKey.CheckGenesis(state))
                    return debug::error(FUNCTION, "invalid genesis transaction");
            }

            /* Handle Adding Trust Transactions. */
            else if(IsTrust())
            {
                /* No Trust Transaction without a Genesis. */
                if(!LLD::Trust->ReadTrustKey(cKey, trustKey))
                {
                    /* FindGenesis will set hashPrevBlock to genesis block. Don't want to change that here, so use temp hash */
                    if(!FindGenesis(cKey, state.hashPrevBlock, trustKey))
                        return debug::error(FUNCTION, "no trust without genesis");
                }

                /* Check that the Trust Key and Current Block match. */
                if(trustKey.vchPubKey != vTrustKey)
                    return debug::error(FUNCTION, "trust key and block trust key mismatch");

                /* Trust Keys can only exist after the Genesis Transaction. */
                TAO::Ledger::BlockState stateGenesis;
                if(!LLD::Ledger->ReadBlock(trustKey.hashGenesisBlock, stateGenesis))
                    return debug::error(FUNCTION, "genesis block not found");

                /* Double Check the Genesis Transaction. */
                if(!trustKey.CheckGenesis(stateGenesis))
                    return debug::error(FUNCTION, "invalid genesis transaction");
            }

            /* Write trust key changes to disk. */
            trustKey.hashLastBlock = state.GetHash();

            /* Get the last trust key time. */
            uint64_t nBlockTime = state.GetBlockTime();
            trustKey.nLastBlockTime = nBlockTime;

            /* Get the stake rate. */
            trustKey.nStakeRate = trustKey.StakeRate(state, nBlockTime);

            /* Write the trust key. */
            LLD::Trust->WriteTrustKey(cKey, trustKey);

            /* Check that the trust score is accurate. */
            if(state.nVersion >= 5 && !CheckTrust(state))
                return debug::error(FUNCTION, "invalid trust score");
        }

        /* Read all of the inputs. */
        uint64_t nValueIn = 0;

        /* Get the number of inputs to the transaction. */
        uint32_t nSize = static_cast<uint32_t>(vin.size());
        for(uint32_t i = (uint32_t)fIsCoinStake; i < nSize; ++i)
        {
            /* Check the inputs map to tx inputs. */
            OutPoint prevout = vin[i].prevout;
            assert(inputs.count(prevout.hash) > 0);

            /* Get the type of transaction. */
            uint8_t nType = inputs.at(prevout.hash).first;

            /* Switch based on type. */
            switch(nType)
            {
                /* Handle for legacy transaction. */
                case TAO::Ledger::LEGACY:
                {
                    /* Get the previous transaction. */
                    Transaction txPrev;
                    inputs.at(prevout.hash).second.SetPos(0);
                    inputs.at(prevout.hash).second >> txPrev;

                    /* Check the inputs range. */
                    if(prevout.n >= txPrev.vout.size())
                        return debug::error(FUNCTION, "prevout is out of range");

                    /* Check maturity before spend. */
                    if(txPrev.IsCoinBase() || txPrev.IsCoinStake())
                    {
                        /* Read the previous block state. */
                        TAO::Ledger::BlockState statePrev;
                        if(!LLD::Ledger->ReadBlock(txPrev.GetHash(), statePrev))
                            return debug::error(FUNCTION, "failed to read previous tx block");

                        /* Check the maturity. */
                        if((state.nHeight - statePrev.nHeight) < (config::fTestNet.load() ? TAO::Ledger::TESTNET_MATURITY_BLOCKS : TAO::Ledger::NEXUS_MATURITY_BLOCKS))
                            return debug::error(FUNCTION, "tried to spend immature balance ", (state.nHeight - statePrev.nHeight));
                    }

                    /* Check the transaction timestamp. */
                    if(txPrev.nTime > nTime)
                        return debug::error(FUNCTION, "transaction timestamp earlier than input transaction");

                    /* Check for overflow input values. */
                    nValueIn += txPrev.vout[prevout.n].nValue;
                    if(!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                        return debug::error(FUNCTION, "txin values out of range");

                    /* Check for double spends. */
                    if(LLD::Legacy->IsSpent(prevout.hash, prevout.n))
                        return debug::error(FUNCTION, "prev tx ", prevout.hash.SubString(), " is already spent");

                    /* Check the ECDSA signatures. (...When not syncronizing) */
                    if(!TAO::Ledger::ChainState::Synchronizing() && !VerifySignature(txPrev, *this, i, 0))
                        return debug::error(FUNCTION, "signature is invalid");

                    /* Commit to disk if flagged. */
                    if((nFlags == FLAGS::BLOCK) && !LLD::Legacy->WriteSpend(prevout.hash, prevout.n))
                        return debug::error(FUNCTION, "failed to write spend");

                    break;
                }

                /* Handle for tritium transaction. */
                case TAO::Ledger::TRITIUM:
                {
                    /* Check for Tritium version transactions.
                     * Becomes ACTIVE 2 hours after time-lock.
                     */
                    if(!TAO::Ledger::VersionActive(nTime + 7200, 7))
                        return debug::error(FUNCTION, "tritium transactions not available until version 7");

                    /* Get the previous transaction. */
                    TAO::Ledger::Transaction txPrev;
                    inputs.at(prevout.hash).second.SetPos(0);
                    inputs.at(prevout.hash).second >> txPrev;

                    /* Check the inputs range. */
                    if(prevout.n >= txPrev.Size())
                        return debug::error(FUNCTION, "prevout is out of range");

                    /* Check maturity before spend. */
                    if(txPrev.IsCoinBase() || txPrev.IsCoinStake())
                        return debug::error(FUNCTION, "cannot spend producer from UTXO");

                    /* Check the transaction timestamp. */
                    if(txPrev.nTimestamp > nTime)
                        return debug::error(FUNCTION, "transaction timestamp earlier than input transaction");

                    /* Check for overflow input values. */
                    const TxOut txout = GetOutputFor(vin[i], inputs);
                    nValueIn += txout.nValue;
                    if(!MoneyRange(txout.nValue) || !MoneyRange(nValueIn))
                        return debug::error(FUNCTION, "txin values out of range");

                    /* Check for double spends. */
                    if(LLD::Legacy->IsSpent(prevout.hash, prevout.n))
                        return debug::error(FUNCTION, "prev tx ", prevout.hash.SubString(), " is already spent");

                    /* Check the ECDSA signatures. (...When not syncronizing) */
                    if(!TAO::Ledger::ChainState::Synchronizing())
                    {
                        /* Check that hashes match. */
                        if(prevout.hash != txPrev.GetHash())
                            return debug::error(FUNCTION, "prevout.hash mismatch");

                        /* Verify the scripts. */
                        if(!VerifyScript(vin[i].scriptSig, txout.scriptPubKey, *this, i, 0))
                            return debug::error(FUNCTION, "invalid script");
                    }

                    /* Commit to disk if flagged. */
                    if((nFlags == FLAGS::BLOCK) && !LLD::Legacy->WriteSpend(prevout.hash, prevout.n))
                        return debug::error(FUNCTION, "failed to write spend");

                    break;
                }

                default:
                    return debug::error(FUNCTION, "unknown transaction type");
            }
        }

        /* Check the coinstake transaction. */
        if(fIsCoinStake)
        {
            /* Get the coinstake interest. */
            uint64_t nStakeReward = 0;
            if(!CoinstakeReward(state, nStakeReward))
                return debug::error(FUNCTION, GetHash().SubString(10), " failed to get coinstake interest");

            /* Check that the interest is within range. */
            //add tolerance to stake reward of + 1 (viz.) for stake rewards
            if (vout[0].nValue / 1000 > (nStakeReward + nValueIn) / 1000)
                return debug::error(FUNCTION, GetHash().SubString(), " stake reward ", vout[0].nValue / 1000, " mismatch ", (nStakeReward + nValueIn) / 1000);
        }
        else if(nValueIn < GetValueOut())
            return debug::error(FUNCTION, GetHash().SubString(), " value in ", nValueIn, " < value out ", GetValueOut());

        /* Calculate the mint if connected with a block. */
        if(nFlags == FLAGS::BLOCK)
            state.nMint += (int32_t)(GetValueOut() - nValueIn);

        /* UTXO to Sig Chain support - If we are connected with a block then check the outputs to see if any of them
           are to a register address.  If they are then write an event for the account holder */
        if(nFlags == FLAGS::BLOCK)
        {
            for(const auto txout : vout )
            {
                uint256_t hashTo;
                if( ExtractRegister( txout.scriptPubKey, hashTo))
                {
                    /* Read the owner of register. */
                    TAO::Register::State state;
                    if(!LLD::Register->ReadState(hashTo, state, nFlags))
                        return debug::error(FUNCTION, "failed to read register to");

                    /* Commit an event for receiving sigchain in the legay DB. */
                    if(!LLD::Legacy->WriteEvent(state.hashOwner, GetHash()))
                        return debug::error(FUNCTION, "failed to write event for account ", state.hashOwner.SubString());
                }
            }
        }

        return true;
    }

    /* Mark the inputs in a transaction as unspent. */
    bool Transaction::Disconnect() const
    {
        /* Coinbase has no inputs. */
        if(!IsCoinBase())
        {
            /* Get the number of inputs to the transaction. */
            uint32_t nSize = static_cast<uint32_t>(vin.size());

            /* Read all of the inputs. */
            for(uint32_t i = (uint32_t)IsCoinStake(); i < nSize; ++i)
            {
                /* Erase the spends. */
                if(!LLD::Legacy->EraseSpend(vin[i].prevout.hash, vin[i].prevout.n))
                    return debug::error(FUNCTION, "failed to erase spends.");
            }

            /* Remove events for any UTXO to sig chain sends */
            for(const auto txout : vout )
            {
                uint256_t hashTo;
                if( ExtractRegister( txout.scriptPubKey, hashTo))
                {
                    /* Read the owner of register. */
                    TAO::Register::State state;
                    if(!LLD::Register->ReadState(hashTo, state))
                        return debug::error(FUNCTION, "failed to read register to");

                    /* Commit an event for receiving sigchain in the legay DB. */
                    if(!LLD::Legacy->EraseEvent(state.hashOwner))
                        return debug::error(FUNCTION, "failed to write event for account ", state.hashOwner.SubString());
                }
            }
        }

        return true;
    }


    /* Check the calculated trust score meets published one. */
    bool Transaction::CheckTrust(const TAO::Ledger::BlockState& state) const
    {
        /* No trust score for non proof of stake (for now). */
        if(!IsCoinStake())
            return debug::error(FUNCTION, "not proof of stake");

        /* Extract the trust key from the coinstake. */
        uint576_t cKey;
        if(!TrustKey(cKey))
            return debug::error(FUNCTION, "trust key not found in script");

        /* Genesis has a trust score of 0. */
        if(IsGenesis())
        {
            if(vin[0].scriptSig.size() != 8)
                return debug::error(FUNCTION, "genesis unexpected size ", vin[0].scriptSig.size());

            return true;
        }

        /* Version 5 - last trust block. */
        uint1024_t hashLastBlock;
        uint32_t   nSequence;
        uint32_t   nTrustScore;

        /* Extract values from coinstake vin. */
        if(!ExtractTrust(hashLastBlock, nSequence, nTrustScore))
            return debug::error(FUNCTION, "failed to extract values from script");

        /* Check that the last trust block is in the block database. */
        TAO::Ledger::BlockState stateLast;
        if(!LLD::Ledger->ReadBlock(hashLastBlock, stateLast))
            return debug::error(FUNCTION, "last block not in database");

        /* Check that the previous block is in the block database. */
        TAO::Ledger::BlockState statePrev;
        if(!LLD::Ledger->ReadBlock(state.hashPrevBlock, statePrev))
            return debug::error(FUNCTION, "prev block not in database");

        /* Get the last coinstake transaction. */
        Transaction txLast;
        if(!LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast))
            return debug::error(FUNCTION, "last state coinstake tx not found");

        /* Enforce the minimum trust key interval of 120 blocks. */
        const uint32_t nMinimumInterval = config::fTestNet.load()
                                            ? TAO::Ledger::TESTNET_MINIMUM_INTERVAL
                                            : TAO::Ledger::MAINNET_MINIMUM_INTERVAL_LEGACY;

        if(state.nHeight - stateLast.nHeight < nMinimumInterval)
            return debug::error(FUNCTION, "trust key interval below minimum interval ", state.nHeight - stateLast.nHeight);

        /* Extract the last trust key */
        uint576_t keyLast;
        if(!txLast.TrustKey(keyLast))
            return debug::error(FUNCTION, "couldn't extract trust key from previous tx");

        /* Ensure the last block being checked is the same trust key. */
        if(keyLast != cKey)
            return debug::error(FUNCTION,
                "trust key in previous block ", cKey.SubString(),
                " to this one ", keyLast.SubString());

        /* Placeholder in case previous block is a version 4 block. */
        uint32_t nScorePrev = 0;
        uint32_t nScore     = 0;

        /* If previous block is genesis, set previous score to 0. */
        if(txLast.IsGenesis())
        {
            /* Enforce sequence number 1 if previous block was genesis */
            if(nSequence != 1)
                return debug::error(FUNCTION, "first trust block and sequence is not 1: ", nSequence);

            /* Genesis results in a previous score of 0. */
            nScorePrev = 0;
        }

        /* Version 4 blocks need to get score from previous blocks calculated score from the trust pool. */
        else if(stateLast.nVersion < 5)
        {
            /* Check the trust pool - this should only execute once transitioning from version 4 to version 5 trust keys. */
            Legacy::TrustKey trustKey;
            if(!LLD::Trust->ReadTrustKey(cKey, trustKey))
                return debug::error(FUNCTION, "couldn't find the genesis");

            /* Enforce sequence number of 1 for anything made from version 4 blocks. */
            if(nSequence != 1)
                return debug::error(FUNCTION, "version 4 block sequence number is ", nSequence);

            /* Ensure that a version 4 trust key is not expired based on new timespan rules. */
            if(trustKey.Expired(TAO::Ledger::ChainState::stateBest.load()))
                return debug::error("version 4 key expired ", trustKey.BlockAge(TAO::Ledger::ChainState::stateBest.load()));

            /* Score is the total age of the trust key for version 4. */
            nScorePrev = trustKey.Age(TAO::Ledger::ChainState::stateBest.load().GetBlockTime());
        }

        /* Version 5 blocks that are trust must pass sequence checks. */
        else
        {
            /* The last block of previous. */
            uint1024_t hashBlockPrev = 0; //dummy variable unless we want to do recursive checking of scores all the way back to genesis

            /* Extract the value from the previous block. */
            uint32_t nSequencePrev;
            if(!txLast.ExtractTrust(hashBlockPrev, nSequencePrev, nScorePrev))
                return debug::error(FUNCTION, "failed to extract trust");

            /* Enforce Sequence numbering, must be +1 always. */
            if(nSequence != nSequencePrev + 1)
                return debug::error(FUNCTION, "previous sequence broken");
        }

        /* The time it has been since the last trust block for this trust key. */
        uint32_t nTimespan = (statePrev.GetBlockTime() - stateLast.GetBlockTime());

        /* Timespan less than required timespan is awarded the total seconds it took to find. */
        if(nTimespan < (config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN))
            nScore = nScorePrev + nTimespan;

        /* Timespan more than required timespan is penalized 3 times the time it took past the required timespan. */
        else
        {
            /* Calculate the penalty for score (3x the time). */
            uint32_t nPenalty = (nTimespan - (config::fTestNet.load() ?
                TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN)) * 3;

            /* Catch overflows and zero out if penalties are greater than previous score. */
            if(nPenalty > nScorePrev)
                nScore = 0;
            else
                nScore = (nScorePrev - nPenalty);
        }

        /* Set maximum trust score to seconds passed for interest rate. */
        if(nScore > TAO::Ledger::TRUST_SCORE_MAX)
            nScore = TAO::Ledger::TRUST_SCORE_MAX;

        /* Debug output. */
        debug::log(2, FUNCTION,
            "score=", nScore, ", ",
            "prev=", nScorePrev, ", ",
            "timespan=", nTimespan, ", ",
            "change=", (int32_t)(nScore - nScorePrev), ")" );

        /* Check that published score in this block is equivilent to calculated score. */
        if(nTrustScore != nScore)
            return debug::error(FUNCTION, "published trust score ", nTrustScore, " not meeting calculated score ", nScore);

        return true;
    }


    /* Get the corresponding output from input. */
    const TxOut Transaction::GetOutputFor(const TxIn& input, const std::map<uint512_t, std::pair<uint8_t, DataStream> >& inputs) const
    {
        /* Find the input in the map. */
        auto mi = inputs.find(input.prevout.hash);
        if(mi == inputs.end())
            throw debug::exception(FUNCTION, "prevout.hash not found");

        /* Get the type. */
        uint8_t nType = (*mi).second.first;

        /* Switch based on type. */
        switch(nType)
        {
            /* Handle for legacy transaction. */
            case TAO::Ledger::LEGACY:
            {
                /* Read the previous transaction. */
                Transaction txPrev;
                (*mi).second.second.SetPos(0);
                (*mi).second.second >> txPrev;

                /* Check ranges. */
                if(input.prevout.n >= txPrev.vout.size())
                    throw debug::exception(FUNCTION, "prevout.n out of range");

                return txPrev.vout[input.prevout.n];
            }

            /* Handle for tritium transaction. */
            case TAO::Ledger::TRITIUM:
            {
                /* Get the tritium transaction. */
                TAO::Ledger::Transaction txPrev;
                (*mi).second.second.SetPos(0);
                (*mi).second.second >> txPrev;

                /* Check ranges. */
                if(input.prevout.n >= txPrev.Size())
                    throw debug::exception(FUNCTION, "prevout.n out of range");

                /* Get legacy converted output.*/
                TxOut txout;
                if(!txPrev[input.prevout.n].Legacy(txout))
                    throw debug::exception(FUNCTION, "invalid tritium operation");

                return txout;
            }

            /* Fail for default. */
            default:
                throw debug::exception(FUNCTION, "unkown transaction type");
        }

        return TxOut();
    }
}
