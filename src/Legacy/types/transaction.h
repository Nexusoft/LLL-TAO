/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_TYPES_TRANSACTION_H
#define NEXUS_LEGACY_TYPES_TRANSACTION_H

#include <Util/templates/serialize.h>

#include <Legacy/types/txin.h>
#include <Legacy/types/txout.h>

namespace Legacy
{

	/** Transaction Class
	 *
	 * The basic transaction that is broadcasted on the network and contained in
	 * blocks.  A transaction can contain multiple inputs and outputs.
	 *
	 */
	class Transaction
	{
	public:

		/** The version specifier of transaction. **/
		int32_t nVersion;


		/** The timestamp of transaction.
         *
         *  Timestamps are generally uint64_t but this one must remain uint32_t
         *  to support unserialization of unsigned int values in legacy wallets.
         *
         **/
		uint32_t nTime;


		/** The inputs for the transaction. **/
		std::vector<CTxIn> vin;


		/** The outputs for the transaction. **/
		std::vector<CTxOut> vout;


		/** The time this transaction is locked to. **/
		uint32_t nLockTime;


		//serialization methods
		IMPLEMENT_SERIALIZE
		(
			READWRITE(nVersion);
			READWRITE(nTime);
			READWRITE(vin);
			READWRITE(vout);
			READWRITE(nLockTime);
		)


		/** Default Constructor. **/
		Transaction()
		: nVersion(1)
		, nTime(0)
		, vin()
		, vout()
		, nLockTime(0)
		{
			SetNull();
		}

		/** Default destructor. **/
		~Transaction() {}


		/** Comparison overload (equals). **/
		friend bool operator==(const Transaction& a, const Transaction& b)
		{
			return (a.nVersion  == b.nVersion &&
					a.nTime     == b.nTime &&
					a.vin       == b.vin &&
					a.vout      == b.vout &&
					a.nLockTime == b.nLockTime);
		}


		/** Comparision overload (not equals). **/
		friend bool operator!=(const Transaction& a, const Transaction& b)
		{
			return !(a == b);
		}


		/** Set Null
		 *
		 *  Sets the transaciton object to a null state.
		 *
		 **/
		void SetNull();


		/** Is Null
		 *
		 *  Determines if object is in a null state.
		 *
		 **/
		bool IsNull() const;


		/** Get Hash
		 *
		 *  Returns the hash of this object.
		 *
		 *  @return the 512-bit hash of this object.
		 *
		 **/
		uint512_t GetHash() const;


		/** Is Final
		 *
		 *  Determine if a transaction is within LOCKTIME_THRESHOLD
		 *
		 *  @param[in] nBlockHeight The block to check at
		 *  @param[in] nBlockTime The time of the block
		 *
		 *  @return true if the transaction is in a final state.
		 *
		 **/
		bool IsFinal(int32_t nBlockHeight=0, int64_t nBlockTime=0) const;


		/** Is Newer Than
		 *
		 *  Determine if a transaction is newer than supplied argument.
		 *
		 *  @param[in] old The transaction to compare to.
		 *
		 *  @return true if the transaction is newer.
		 *
		 **/
		bool IsNewerThan(const Transaction& old) const;


		/** Is Coinbase
		 *
		 *  Check the flags that denote a coinbase transaction.
		 *
		 *  @return true if this is a coinbase.
		 *
		 **/
		bool IsCoinBase() const;


		/** Is Coinstake
		 *
		 *  Check the flags that denote a coinstake transaction.
		 *
		 *  @return true if this is a coinstake.
		 *
		 **/
		bool IsCoinStake() const;


		/** Is Genesis
		 *
		 *  Check the flags that denote a genesis transaction.
		 *
		 *  @return true if this is a genesis.
		 *
		 **/
		bool IsGenesis() const;


		/** Is Trust
		 *
		 *  Check the flags that denote a trust transaction.
		 *
		 *  @return true if this is a trust transaction.
		 *
		 **/
		bool IsTrust() const;


		/** Is Standard
		 *
		 *  Check for standard transaction types
		 *
		 *  @return True if all outputs (scriptPubKeys) use only standard transaction forms
		 *
		 **/
		bool IsStandard() const;


		/** Are Inputs Standard
		 *
		 *  Check for standard transaction types
		 *
		 *  @param[in] mapInputs Map of previous transactions that have outputs we're spending
		 *
		 *  @return True if all inputs (scriptSigs) use only standard transaction forms
		 *  @see CTransaction::FetchInputs
		 *
		 **/
		bool AreInputsStandard(const std::map<uint512_t, Transaction>& mapInputs) const;


		/** Get Legacy SigOp Count
		 *
		 *  Count ECDSA signature operations the old-fashioned (pre-0.6) way
		 *
		 *  @return number of sigops this transaction's outputs will produce when spent
		 *  @see CTransaction::FetchInputs
		 *
		 **/
		uint32_t GetLegacySigOpCount() const;


		/** Total SigOps
		 *
		 *  Count ECDSA signature operations in pay-to-script-hash inputs.
		 *
		 *  @param[in] mapInputs Map of previous transactions that have outputs we're spending
		 *
		 *  @return maximum number of sigops required to validate this transaction's inputs
		 *  @see CTransaction::FetchInputs
		 *
		 **/
		uint32_t TotalSigOps(const std::map<uint512_t, Transaction>& mapInputs) const;


		/** Get Value Out
		 *
		 *  Amount of Coins spent by this transaction.
		 *
		 *  @return sum of all outputs (note: does not include fees)
		 *
		 **/
		int64_t GetValueOut() const;


		/** Get Value In
		 *
		 *  Amount of Coins coming in to this transaction
		 *
		 *  @param[in] mapInputs Map of previous transactions that have outputs we're spending
		 *
		 *  @return	Sum of value of all inputs (scriptSigs)
		 *  @see CTransaction::FetchInputs
		 *
		 **/
		int64_t GetValueIn(const std::map<uint512_t, Transaction>& mapInputs) const;


		/** Allow Free
		 *
		 *  Determine if transaction qualifies to be free.
		 *
		 *  @param[in] dPriority The priority of requested transaction.
		 *
		 *  @return true if can be free.
		 *
		 **/
		bool AllowFree(double dPriority);


		/** Get Min Fee
		 *
		 *  Get the minimum fee to pay for broadcast.
		 *
		 *  @param[in] nBlockSize The current block size
		 *  @param[in] fAllowFree Flag to tell if free is allowed.
		 *  @param[in] mode The mode in which this will be relayed.
		 *
		 *  @return The fee in satoshi's for transaction.
		 *
		 **/
		int64_t GetMinFee(uint32_t nBlockSize=1, bool fAllowFree=false, enum GetMinFee_mode mode=GMF_BLOCK) const;


		/** To String Short
		 *
		 *  Short form of the debug output.
		 *
		 *  @return The string value to return;
		 *
		 **/
		std::string ToStringShort() const;


		/** To String
		 *
		 *  Long form of the debug output.
		 *
		 *  @return The string value to return.
		 *
		 **/
		std::string ToString() const;


		/** Print
		 *
		 *  Dump the transaction data to the console.
		 *
		 **/
		void print() const;


		/** Check Transaction
		 *
		 *  Check the transaction for validity.
		 *
		 **/
		bool CheckTransaction() const;


	protected:

		/** Get Output For
		 *
		 *  Get the corresponding output from input.
		 *
		 *  @param[in] input The input to get output for
		 *  @param[in] inputs The inputs map that has prev transactions
		 *
		 *  @return the output that was found.
		 *
		 **/
		const CTxOut& GetOutputFor(const CTxIn& input, const std::map<uint512_t, Transaction>& inputs) const;
	};
}


#endif
