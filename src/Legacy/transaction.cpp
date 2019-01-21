/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <Legacy/types/transaction.h>
#include <Legacy/include/money.h>
#include <Legacy/types/script.h>
#include <Legacy/include/signature.h>
#include <Legacy/include/evaluate.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/runtime.h>

namespace Legacy
{

    //TODO: this may be needed elsewhere.
    //keep here for now.
    const int64_t LOCKTIME_THRESHOLD = 500000000;


	/* Sets the transaciton object to a null state. */
	void Transaction::SetNull()
	{
		nVersion = 1;
		nTime = runtime::unifiedtimestamp();
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
	    return LLC::SK512(ss.begin(), ss.end());
	}


	/* Determine if a transaction is within LOCKTIME_THRESHOLD */
	bool Transaction::IsFinal(int32_t nBlockHeight, int64_t nBlockTime) const
	{
		// Time based nLockTime implemented in 0.1.6
		if (nLockTime == 0)
			return true;

		if (nBlockHeight == 0)
			nBlockHeight = TAO::Ledger::ChainState::nBestHeight;

		if (nBlockTime == 0)
			nBlockTime = runtime::unifiedtimestamp();

		if ((int64_t)nLockTime < ((int64_t)nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
			return true;

		for(auto txin : vin)
			if (!txin.IsFinal())
				return false;

		return true;
	}


	/* Determine if a transaction is newer than supplied argument. */
	bool Transaction::IsNewerThan(const Transaction& old) const
	{
		if (vin.size() != old.vin.size())
			return false;
		for (uint32_t i = 0; i < vin.size(); i++)
			if (vin[i].prevout != old.vin[i].prevout)
				return false;

		bool fNewer = false;
		uint32_t nLowest = std::numeric_limits<uint32_t>::max();
		for (uint32_t i = 0; i < vin.size(); i++)
		{
			if (vin[i].nSequence != old.vin[i].nSequence)
			{
				if (vin[i].nSequence <= nLowest)
				{
					fNewer = false;
					nLowest = vin[i].nSequence;
				}
				if (old.vin[i].nSequence < nLowest)
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
		/* Must have more than one Input. */
		if(vin.size() <= 1)
			return false;

		/* First Input Script Signature must Contain Fibanacci Byte Series. */
		if(!vin[0].IsStakeSig())
			return false;

		/* All Remaining Previous Inputs must not be Empty. */
		for(int nIndex = 1; nIndex < vin.size(); nIndex++)
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
        for(auto txin : vin)
        {
            // Biggest 'standard' txin is a 3-signature 3-of-3 CHECKMULTISIG
            // pay-to-script-hash, which is 3 ~80-byte signatures, 3
            // ~65-byte public keys, plus a few script ops.
            if (txin.scriptSig.size() > 500)
                return false;

            if (!txin.scriptSig.IsPushOnly())
                return false;
        }

        for(auto txout : vout)
            if (!Legacy::IsStandard(txout.scriptPubKey))
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
        if (!Solver(vout[0].scriptPubKey, whichType, vSolutions))
            return debug::error(FUNCTION, "couldn't find trust key in script");

        /* Enforce public key rules. */
        if (whichType != TX_PUBKEY)
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

        /* Check the coin age of each Input. */
        for(int nIndex = 1; nIndex < vin.size(); nIndex++)
        {
            /** Calculate the Age and Value of given output. **/
            int64_t nCoinAge = (nTime - 0);//block.GetBlockTime());

            /** Compound the Total Figures. **/
            nAge += nCoinAge;
        }

        nAge /= (vin.size() - 1);

        return true;
    }


	/* Check for standard transaction types */
	bool Transaction::AreInputsStandard(const std::map<uint512_t, Transaction>& mapInputs) const
    {
        if (IsCoinBase())
            return true; // Coinbases don't use vin normally

        for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
        {
            const CTxOut& prev = GetOutputFor(vin[i], mapInputs);

            std::vector< std::vector<uint8_t> > vSolutions;
            TransactionType whichType;
            // get the scriptPubKey corresponding to this input:
            const CScript& prevScript = prev.scriptPubKey;
            if (!Solver(prevScript, whichType, vSolutions))
                return false;

            int nArgsExpected = ScriptSigArgsExpected(whichType, vSolutions);
            if (nArgsExpected < 0)
                return false;

            // Transactions with extra stuff in their scriptSigs are
            // non-standard. Note that this EvalScript() call will
            // be quick, because if there are any operations
            // beside "push data" in the scriptSig the
            // IsStandard() call returns false
            std::vector< std::vector<uint8_t> > stack;
            if (!EvalScript(stack, vin[i].scriptSig, *this, i, 0))
                return false;

            if (whichType == TX_SCRIPTHASH)
            {
                if (stack.empty())
                    return false;

                CScript subscript(stack.back().begin(), stack.back().end());
                std::vector< std::vector<uint8_t> > vSolutions2;
                TransactionType whichType2;
                if (!Solver(subscript, whichType2, vSolutions2))
                    return false;
                if (whichType2 == TX_SCRIPTHASH)
                    return false;

                int tmpExpected;
                tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
                if (tmpExpected < 0)
                    return false;
                nArgsExpected += tmpExpected;
            }

            if (stack.size() != (uint32_t)nArgsExpected)
                return false;
        }

        return true;
    }


	/* Count ECDSA signature operations the old-fashioned (pre-0.6) way */
	uint32_t Transaction::GetLegacySigOpCount() const
    {
        unsigned int nSigOps = 0;
        for(auto txin : vin)
        {
            if(txin.IsStakeSig())
                continue;

            nSigOps += txin.scriptSig.GetSigOpCount(false);
        }

        for(auto txout : vout)
        {
            nSigOps += txout.scriptPubKey.GetSigOpCount(false);
        }

        return nSigOps;
    }


	/* Count ECDSA signature operations in pay-to-script-hash inputs. */
	uint32_t Transaction::TotalSigOps(const std::map<uint512_t, Transaction>& mapInputs) const
    {
        if (IsCoinBase())
            return 0;

        uint32_t nSigOps = 0;
        for (uint32_t i = (uint32_t) IsCoinStake(); i < vin.size(); i++)
        {
            const CTxOut& prevout = GetOutputFor(vin[i], mapInputs);
            nSigOps += prevout.scriptPubKey.GetSigOpCount(vin[i].scriptSig);
        }

        return nSigOps;
    }


	/* Amount of Coins spent by this transaction. */
	uint64_t Transaction::GetValueOut() const
	{
		int64_t nValueOut = 0;
		for(auto txout : vout)
		{
			nValueOut += txout.nValue;
			if (!MoneyRange(txout.nValue) || !MoneyRange(nValueOut))
				throw std::runtime_error("Transaction::GetValueOut() : value out of range");
		}

		return nValueOut;
	}


	/* Amount of Coins coming in to this transaction */
	uint64_t Transaction::GetValueIn(const std::map<uint512_t, Transaction>& mapInputs) const
    {
        if (IsCoinBase())
            return 0;

        int64_t nResult = 0;
        for (uint32_t i = (uint32_t) IsCoinStake(); i < vin.size(); ++i)
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
        if (fAllowFree)
        {
            if (nBlockSize == 1)
            {
                // Transactions under 10K are free
                // (about 4500bc if made of 50bc inputs)
                if (nBytes < 10000)
                    nMinFee = 0;
            }
            else
            {
                // Free transaction area
                if (nNewBlockSize < 27000)
                    nMinFee = 0;
            }
        }

        /* To limit dust spam, require MIN_TX_FEE/MIN_RELAY_TX_FEE if any output is less than 0.01 */
        if (nMinFee < nBaseFee)
        {
            for(auto txout : vout)
                if (txout.nValue < CENT)
                    nMinFee = nBaseFee;
        }

        /* Raise the price as the block approaches full */
        if (nBlockSize != 1 && nNewBlockSize >= TAO::Ledger::MAX_BLOCK_SIZE_GEN / 2)
        {
            if (nNewBlockSize >= TAO::Ledger::MAX_BLOCK_SIZE_GEN)
                return MaxTxOut();

            nMinFee *= TAO::Ledger::MAX_BLOCK_SIZE_GEN / (TAO::Ledger::MAX_BLOCK_SIZE_GEN - nNewBlockSize);
        }

        if (!MoneyRange(nMinFee))
            nMinFee = MaxTxOut();

        return nMinFee;
    }


	/* Short form of the debug output. */
	std::string Transaction::ToStringShort() const
    {
        std::string str;
        str += debug::strprintf("%s %s", GetHash().ToString().c_str(), IsCoinBase()? "base" : (IsCoinStake()? "stake" : "user"));
        return str;
    }


	/* Long form of the debug output. */
	std::string Transaction::ToString() const
    {
        std::string str;
        str += IsCoinBase() ? "Coinbase" : (IsGenesis() ? "Genesis" : (IsTrust() ? "Trust" : "Transaction"));
        str += debug::strprintf("(hash=%s, nTime=%d, ver=%d, vin.size=%d, vout.size=%d, nLockTime=%d)",
            GetHash().ToString().substr(0,10).c_str(),
            nTime,
            nVersion,
            vin.size(),
            vout.size(),
            nLockTime);

        for (auto txin : vin)
            str += "    " + txin.ToString() + "";
        for (auto txout : vout)
            str += "    " + txout.ToString() + "";
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
        if (vin.empty())
            return debug::error(FUNCTION, "vin empty");

        /* Check for empty outputs. */
        if (vout.empty())
            return debug::error(FUNCTION, "vout empty");

        /* Check for size limits. */
        if (::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > TAO::Ledger::MAX_BLOCK_SIZE)
            return debug::error(FUNCTION, "size limits failed");

        /* Check for negative or overflow output values */
        int64_t nValueOut = 0;
        for(auto txout : vout)
        {
            /* Checkout for empty outputs. */
            if (txout.IsEmpty() && (!IsCoinBase()) && (!IsCoinStake()))
                return debug::error(FUNCTION, "txout empty for user transaction");

            /* Enforce minimum output amount. */
            if ((!txout.IsEmpty()) && txout.nValue < MIN_TXOUT_AMOUNT)
                return debug::error(FUNCTION, "txout.nValue below minimum");

            /* Enforce maximum output amount. */
            if (txout.nValue > MaxTxOut())
                return debug::error(FUNCTION, "txout.nValue too high");

            /* Enforce maximum total output value. */
            nValueOut += txout.nValue;
            if (!MoneyRange(nValueOut))
                return debug::error(FUNCTION, "txout total out of range");
        }

        /* Check for duplicate inputs */
        std::set<COutPoint> vInOutPoints;
        for(auto txin : vin)
        {
            if (vInOutPoints.count(txin.prevout))
                return false;

            vInOutPoints.insert(txin.prevout);
        }

        /* Check for null previous outputs. */
        if (!IsCoinBase() && !IsCoinStake())
        {
            for(auto txin : vin)
                if (txin.prevout.IsNull())
                    return debug::error(FUNCTION, "prevout is null");
        }

        return true;
    }


    /* Get the inputs for a transaction. */
    bool Transaction::FetchInputs(std::map<uint512_t, Transaction>& inputs) const
    {
        /* Coinbase has no inputs. */
        if (IsCoinBase())
            return true;

        /* Read all of the inputs. */
        for (uint32_t i = IsCoinStake() ? 1 : 0; i < vin.size(); i++)
        {
            /* Skip inputs that are already found. */
            COutPoint prevout = vin[i].prevout;
            if (inputs.count(prevout.hash))
                continue;

            /* Read the previous transaction. */
            Transaction txPrev;
            if(!LLD::legacyDB->ReadTx(prevout.hash, txPrev))
            {
                //TODO: check the memory pool for previous
                return debug::error(FUNCTION, "previous transaction not found");
            }

            /* Check that it is valid. */
            if(prevout.n >= txPrev.vout.size())
                return debug::error(FUNCTION, "prevout is out of range");

            /* Add to the inputs. */
            inputs[prevout.hash] = txPrev;
        }

        return true;
    }


    /* Mark the inputs in a transaction as spent. */
    bool Transaction::Connect(const std::map<uint512_t, Transaction>& inputs, const TAO::Ledger::BlockState* state, uint8_t nFlags) const
    {
        /* Coinbase has no inputs. */
        if (IsCoinBase())
            return true;

        /* Read all of the inputs. */
        uint64_t nValueIn = 0;
        for (uint32_t i = IsCoinStake() ? 1 : 0; i < vin.size(); i++)
        {
            /* Check the inputs map to tx inputs. */
            COutPoint prevout = vin[i].prevout;
            assert(inputs.count(prevout.hash) > 0);

            /* Get the previous transaction. */
            Transaction txPrev = inputs.at(prevout.hash);

            /* Check the inputs range. */
            if (prevout.n >= txPrev.vout.size())
                return debug::error(FUNCTION, "prevout is out of range");

            /* Check maturity before spend. */
            if (txPrev.IsCoinBase() || txPrev.IsCoinStake())
            {
                //TODO: read state vs txPrev state hashblock state.
            }

            /* Check the transaction timestamp. */
            if (txPrev.nTime > nTime)
                return debug::error(FUNCTION, "transaction timestamp earlier than input transaction");

            /* Check for overflow input values. */
            nValueIn += txPrev.vout[prevout.n].nValue;
            if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                return debug::error(FUNCTION, "txin values out of range");

            /* Check for double spends. */
            if(LLD::legacyDB->IsSpent(prevout.hash, prevout.n))
                return debug::error(FUNCTION, "prev tx is already spent");

            /* Check the ECDSA signatures. */
            if(!VerifySignature(txPrev, *this, i, 0))
                return debug::error(FUNCTION, "signature is invalid");

            /* Commit to disk if flagged. */
            if(nFlags & FLAGS::BLOCK && !LLD::legacyDB->WriteSpend(prevout.hash, prevout.n))
                return debug::error(FUNCTION, "failed to write spend");

        }

        /* Check the coinstake transaction. */
        if (IsCoinStake())
        {
            uint64_t nInterest = 0;

            //TODO: Check the coinstake inputs.
            if (vout[0].nValue > nInterest + nValueIn)
                return debug::error(FUNCTION, GetHash().ToString().substr(0,10), " stake reward mismatch");

        }
        else if (nValueIn < GetValueOut())
            return debug::error(FUNCTION, GetHash().ToString().substr(0,10), "value in < value out");

        return true;
    }

    /* Mark the inputs in a transaction as unspent. */
    bool Transaction::Disconnect() const
    {
        /* Coinbase has no inputs. */
        if (!IsCoinBase())
        {
            /* Read all of the inputs. */
            for (uint32_t i = IsCoinStake() ? 1 : 0; i < vin.size(); i++)
            {
                /* Erase the spends. */
                if(!LLD::legacyDB->EraseSpend(vin[i].prevout.hash, vin[i].prevout.n))
                    return debug::error(FUNCTION, "failed to erase spends.");
            }
        }

        /* Erase the transaction object. */
        LLD::legacyDB->EraseTx(GetHash());

        return true;
    }


    /* Get the corresponding output from input. */
    const CTxOut& Transaction::GetOutputFor(const CTxIn& input, const std::map<uint512_t, Transaction>& inputs) const
    {
        auto mi = inputs.find(input.prevout.hash);

        if (mi == inputs.end())
            throw std::runtime_error("Legacy::Transaction::GetOutputFor() : prevout.hash not found");

        const Transaction& txPrev = (*mi).second;

        if (input.prevout.n >= txPrev.vout.size())
            throw std::runtime_error("Legacy::Transaction::GetOutputFor() : prevout.n out of range");

        return txPrev.vout[input.prevout.n];

    }
}
