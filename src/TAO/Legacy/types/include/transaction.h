/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_TRANSACTION_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_TRANSACTION_H


namespace LLD
{
	class CIndexDB;
}

namespace Legacy
{

	namespace Types
	{

		/** Transaction Fee Based Relay Codes. **/
		enum GetMinFee_mode
		{
			GMF_BLOCK,
			GMF_RELAY,
			GMF_SEND,
		};

		/** Typedef for reading previous transaction inputs. TODO: deprecate txindex **/
		typedef std::map<uint512_t, std::pair<CTxIndex, CTransaction> > MapPrevTx;


		/** The basic transaction that is broadcasted on the network and contained in
		 * blocks.  A transaction can contain multiple inputs and outputs.
		 */
		class CTransaction
		{
		public:
			int nVersion;
			uint32_t nTime;
			std::vector<CTxIn> vin;
			std::vector<CTxOut> vout;
			uint32_t nLockTime;

	        //TODO: CTransactionState (hash) - get from prevout on ctxIn
	        //{ std::vector<bool> vSpent; { out1, out2, out3 } }
	        //To hold the spend data on disk transactions.
	        //If it is unspent the vout index will false

			CTransaction()
			{
				SetNull();
			}

			IMPLEMENT_SERIALIZE
			(
				READWRITE(this->nVersion);
				nVersion = this->nVersion;
				READWRITE(nTime);
				READWRITE(vin);
				READWRITE(vout);
				READWRITE(nLockTime);
			)

			void SetNull()
			{
				nVersion = 1;
				nTime = UnifiedTimestamp();
				vin.clear();
				vout.clear();
				nLockTime = 0;
			}

			bool IsNull() const
			{
				return (vin.empty() && vout.empty());
			}

			uint512_t GetHash() const
			{
				return SerializeHash(*this);
			}

			bool IsFinal(int nBlockHeight=0, int64_t nBlockTime=0) const
			{
				// Time based nLockTime implemented in 0.1.6
				if (nLockTime == 0)
					return true;
				if (nBlockHeight == 0)
					nBlockHeight = nBestHeight;
				if (nBlockTime == 0)
					nBlockTime = Timestamp();
				if ((int64_t)nLockTime < ((int64_t)nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
					return true;
				for(auto txin : vin)
					if (!txin.IsFinal())
						return false;
				return true;
			}

			bool IsNewerThan(const CTransaction& old) const
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

			/** Coinbase Transaction Rules: **/
			bool IsCoinBase() const
			{
				/** A] Input Size must be 1. **/
				if(vin.size() != 1)
					return false;

				/** B] First Input must be Empty. **/
				if(!vin[0].prevout.IsNull())
					return false;

				/** C] Outputs Count must Exceed 1. **/
				if(vout.size() < 1)
					return false;

				return true;
			}

			/** Coinstake Transaction Rules: **/
			bool IsCoinStake() const
			{
				/* A] Must have more than one Input. */
				if(vin.size() <= 1)
					return false;

				/* B] First Input Script Signature must be 8 Bytes. */
				if(vin[0].scriptSig.size() != 8)
					return false;

				/* C] First Input Script Signature must Contain Fibanacci Byte Series. */
				if(!vin[0].IsStakeSig())
					return false;

				/* D] All Remaining Previous Inputs must not be Empty. */
				for(int nIndex = 1; nIndex < vin.size(); nIndex++)
					if(vin[nIndex].prevout.IsNull())
						return false;

				/* E] Must Contain only 1 Outputs. */
				if(vout.size() != 1)
					return false;

				return true;
			}

			/** Flag to determine if the transaction is a genesis transaction. **/
			bool IsGenesis() const
			{
				/* A] Genesis Transaction must be Coin Stake. */
				if(!IsCoinStake())
					return false;

				/* B] First Input Previous Transaction must be Empty. */
				if(!vin[0].prevout.IsNull())
					return false;

				return true;
			}


			/** Flag to determine if the transaction is a Trust Transaction. **/
			bool IsTrust() const
			{
				/** A] Genesis Transaction must be Coin Stake. **/
				if(!IsCoinStake())
					return false;

				/** B] First Input Previous Transaction must not be Empty. **/
				if(vin[0].prevout.IsNull())
					return false;

				/** C] First Input Previous Transaction Hash must not be 0. **/
				if(vin[0].prevout.hash == 0)
					return false;

				/** D] First Input Previous Transaction Index must be 0. **/
				if(vin[0].prevout.n != 0)
					return false;

				return true;
			}

			/** Check for standard transaction types
				@return True if all outputs (scriptPubKeys) use only standard transaction forms
			*/
			bool IsStandard() const;

			/** Check for standard transaction types
				@param[in] mapInputs	Map of previous transactions that have outputs we're spending
				@return True if all inputs (scriptSigs) use only standard transaction forms
				@see CTransaction::FetchInputs
			*/
			bool AreInputsStandard(const MapPrevTx& mapInputs) const;

			/** Count ECDSA signature operations the old-fashioned (pre-0.6) way
				@return number of sigops this transaction's outputs will produce when spent
				@see CTransaction::FetchInputs
			*/
			uint32_t GetLegacySigOpCount() const;

			/** Count ECDSA signature operations in pay-to-script-hash inputs.

				@param[in] mapInputs	Map of previous transactions that have outputs we're spending
				@return maximum number of sigops required to validate this transaction's inputs
				@see CTransaction::FetchInputs
			 */
			uint32_t TotalSigOps(const MapPrevTx& mapInputs) const;

			/** Amount of Coins spent by this transaction.
				@return sum of all outputs (note: does not include fees)
			 */
			int64_t GetValueOut() const
			{
				int64_t nValueOut = 0;
				BOOST_FOREACH(const CTxOut& txout, vout)
				{
					nValueOut += txout.nValue;
					if (!MoneyRange(txout.nValue) || !MoneyRange(nValueOut))
						throw std::runtime_error("CTransaction::GetValueOut() : value out of range");
				}
				return nValueOut;
			}

			/** Amount of Coins coming in to this transaction

				@param[in] mapInputs	Map of previous transactions that have outputs we're spending
				@return	Sum of value of all inputs (scriptSigs)
				@see CTransaction::FetchInputs
			 */
			int64_t GetValueIn(const MapPrevTx& mapInputs) const;

			static bool AllowFree(double dPriority)
			{
				// Large (in bytes) low-priority (new, small-coin) transactions
				// need a fee.
				return dPriority > COIN * 144 / 250;
			}

			int64_t GetMinFee(uint32_t nBlockSize=1, bool fAllowFree=false, enum GetMinFee_mode mode=GMF_BLOCK) const;


			bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet=NULL);

			friend bool operator==(const CTransaction& a, const CTransaction& b)
			{
				return (a.nVersion  == b.nVersion &&
						a.nTime     == b.nTime &&
						a.vin       == b.vin &&
						a.vout      == b.vout &&
						a.nLockTime == b.nLockTime);
			}

			friend bool operator!=(const CTransaction& a, const CTransaction& b)
			{
				return !(a == b);
			}


			std::string ToStringShort() const;

			std::string ToString() const;

			void print() const;


			bool GetCoinstakeInterest(LLD::CIndexDB& txdb, int64_t& nInterest) const;
			bool GetCoinstakeAge(LLD::CIndexDB& txdb, uint64_t& nAge) const;


			bool ReadFromDisk(LLD::CIndexDB& indexdb, COutPoint prevout, CTxIndex& txindexRet);
			bool ReadFromDisk(LLD::CIndexDB& indexdb, COutPoint prevout);
			bool ReadFromDisk(COutPoint prevout);
			bool DisconnectInputs(LLD::CIndexDB& indexdb);

			/** Fetch from memory and/or disk. inputsRet keys are transaction hashes.

			 @param[in] txdb	Transaction database
			 @param[in] mapTestPool	List of pending changes to the transaction index database
			 @param[in] fBlock	True if being called to add a new best-block to the chain
			 @param[in] fMiner	True if being called by CreateNewBlock
			 @param[out] inputsRet	Pointers to this transaction's inputs
			 @param[out] fInvalid	returns true if transaction is invalid
			 @return	Returns true if all inputs are in txdb or mapTestPool
			 */
			bool FetchInputs(LLD::CIndexDB& indexdb, const std::map<uint512_t, CTxIndex>& mapTestPool,
							 bool fBlock, bool fMiner, MapPrevTx& inputsRet, bool& fInvalid);

			/** Sanity check previous transactions, then, if all checks succeed,
				mark them as spent by this transaction.

				@param[in] inputs	Previous transactions (from FetchInputs)
				@param[out] mapTestPool	Keeps track of inputs that need to be updated on disk
				@param[in] posThisTx	Position of this transaction on disk
				@param[in] pindexBlock
				@param[in] fBlock	true if called from ConnectBlock
				@param[in] fMiner	true if called from CreateNewBlock
				@return Returns true if all checks succeed
			 */
			bool ConnectInputs(LLD::CIndexDB& indexdb, MapPrevTx inputs,
							   std::map<uint512_t, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx,
							   const CBlockIndex* pindexBlock, bool fBlock, bool fMiner);

			bool ClientConnectInputs();
			bool CheckTransaction() const;


		protected:
			const CTxOut& GetOutputFor(const CTxIn& input, const MapPrevTx& inputs) const;
		};
	}
}








	/* __________________________________________________ (Transaction Methods) __________________________________________________  */




	/* Add an oprhan transaction to the Orphans Memory Map. */
	bool AddOrphanTx(const CDataStream& vMsg);


	/* Remove an Orphaned Transaction from the Memory Mao. */
	void EraseOrphanTx(uint512_t hash);


	/* Set the Limit of the Orphan Transaction Sizes Dynamically. */
	uint32_t LimitOrphanTxSize(uint32_t nMaxOrphans);


	/* Get a specific transaction from the Database or from a block's binary position. */
	bool GetTransaction(const uint512_t &hash, CTransaction &tx, uint1024_t &hashBlock);

}


#endif
