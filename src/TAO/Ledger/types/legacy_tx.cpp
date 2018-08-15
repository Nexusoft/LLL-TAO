/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#include "include/transaction.h"

#include "../include/unifiedtime.h"
#include "../include/dispatch.h"
#include "../include/manager.h"

#include "../../LLC/include/random.h"

#include "../../Util/include/ui_interface.h"
#include "../../Util/include/args.h"

#include "../../LLD/include/index.h"

#include "../../LLP/include/legacy.h"

#include "../../Wallet/wallet.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

namespace Core
{
	
	std::string COutPoint::ToString() const
	{
		return strprintf("COutPoint(%s, %d)", hash.ToString().substr(0,10).c_str(), n);
	}

	
	void COutPoint::print() const
	{
		printf("%s\n", ToString().c_str());
	}
	
	
	std::string CTxIn::ToStringShort() const
	{
		return strprintf(" %s %d", prevout.hash.ToString().c_str(), prevout.n);
	}

	std::string CTxIn::ToString() const
	{
		std::string str;
		str += "CTxIn(";
		str += prevout.ToString();
		if (prevout.IsNull())
		{	
			if(IsStakeSig())
				str += strprintf(", genesis %s", HexStr(scriptSig).c_str());
			else
				str += strprintf(", coinbase %s", HexStr(scriptSig).c_str());
		}
		else if(IsStakeSig())
			str += strprintf(", trust %s", HexStr(scriptSig).c_str());
		else
			str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24).c_str());
		if (nSequence != std::numeric_limits<unsigned int>::max())
			str += strprintf(", nSequence=%u", nSequence);
		str += ")";
		return str;
	}

	void CTxIn::print() const
	{
		printf("%s\n", ToString().c_str());
	}
	
	
	std::string CTxOut::ToStringShort() const
	{
		return strprintf(" out %s %s", FormatMoney(nValue).c_str(), scriptPubKey.ToString(true).c_str());
	}

	std::string CTxOut::ToString() const
	{
		if (IsEmpty()) return "CTxOut(empty)";
		if (scriptPubKey.size() < 6)
			return "CTxOut(error)";
		return strprintf("CTxOut(nValue=%s, scriptPubKey=%s)", FormatMoney(nValue).c_str(), scriptPubKey.ToString().c_str());
	}

	void CTxOut::print() const
	{
		printf("%s\n", ToString().c_str());
	}

	
	bool AddOrphanTx(const CDataStream& vMsg)
	{
		CTransaction tx;
		CDataStream(vMsg) >> tx;
		uint512 hash = tx.GetHash();
		if (mapOrphanTransactions.count(hash))
			return false;

		CDataStream* pvMsg = new CDataStream(vMsg);

		// Ignore big transactions, to avoid a
		// send-big-orphans memory exhaustion attack. If a peer has a legitimate
		// large transaction with a missing parent then we assume
		// it will rebroadcast it later, after the parent transaction(s)
		// have been mined or received.
		// 10,000 orphans, each of which is at most 5,000 bytes big is
		// at most 500 megabytes of orphans:
		if (pvMsg->size() > 5000)
		{
			printf("ignoring large orphan tx (size: %u, hash: %s)\n", pvMsg->size(), hash.ToString().substr(0,10).c_str());
			delete pvMsg;
			return false;
		}

		mapOrphanTransactions[hash] = pvMsg;
		for(auto txin : tx.vin)
			mapOrphanTransactionsByPrev[txin.prevout.hash].insert(make_pair(hash, pvMsg));

		printf("stored orphan tx %s (mapsz %u)\n", hash.ToString().substr(0,10).c_str(),
			mapOrphanTransactions.size());
		return true;
	}

	void EraseOrphanTx(uint512 hash)
	{
		if (!mapOrphanTransactions.count(hash))
			return;
		const CDataStream* pvMsg = mapOrphanTransactions[hash];
		CTransaction tx;
		CDataStream(*pvMsg) >> tx;
		for(auto txin : tx.vin)
		{
			mapOrphanTransactionsByPrev[txin.prevout.hash].erase(hash);
			if (mapOrphanTransactionsByPrev[txin.prevout.hash].empty())
				mapOrphanTransactionsByPrev.erase(txin.prevout.hash);
		}
		delete pvMsg;
		mapOrphanTransactions.erase(hash);
	}

	unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans)
	{
		unsigned int nEvicted = 0;
		while (mapOrphanTransactions.size() > nMaxOrphans)
		{
			// Evict a random orphan:
			uint512 randomhash = GetRand512();
			map<uint512, CDataStream*>::iterator it = mapOrphanTransactions.lower_bound(randomhash);
			if (it == mapOrphanTransactions.end())
				it = mapOrphanTransactions.begin();
			EraseOrphanTx(it->first);
			++nEvicted;
		}
		return nEvicted;
	}







	//////////////////////////////////////////////////////////////////////////////
	//
	// CTransaction and CTxIndex
	//
	
	
	std::string CDiskTxPos::ToString() const
	{
		if (IsNull())
			return "null";
		else
			return strprintf("(nFile=%d, nBlockPos=%d, nTxPos=%d)", nFile, nBlockPos, nTxPos);
	}

	void CDiskTxPos::print() const
	{
		printf("%s", ToString().c_str());
	}
	
	
	
	

	int64 CTransaction::GetMinFee(unsigned int nBlockSize, bool fAllowFree, enum GetMinFee_mode mode) const
	{
		if(nVersion >= 2)
			return 0;
				
		// Base fee is either MIN_TX_FEE or MIN_RELAY_TX_FEE
		int64 nBaseFee = (mode == GMF_RELAY) ? MIN_RELAY_TX_FEE : MIN_TX_FEE;
		
		unsigned int nBytes = ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
		unsigned int nNewBlockSize = nBlockSize + nBytes;
		int64 nMinFee = (1 + (int64)nBytes / 1000) * nBaseFee;

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

		// To limit dust spam, require MIN_TX_FEE/MIN_RELAY_TX_FEE if any output is less than 0.01
		if (nMinFee < nBaseFee)
		{
			for(auto txout : vout)
				if (txout.nValue < CENT)
					nMinFee = nBaseFee;
		}

		// Raise the price as the block approaches full
		if (nBlockSize != 1 && nNewBlockSize >= MAX_BLOCK_SIZE_GEN/2)
		{
			if (nNewBlockSize >= MAX_BLOCK_SIZE_GEN)
				return MAX_TXOUT_AMOUNT;
			nMinFee *= MAX_BLOCK_SIZE_GEN / (MAX_BLOCK_SIZE_GEN - nNewBlockSize);
		}

		if (!MoneyRange(nMinFee))
			nMinFee = MAX_TXOUT_AMOUNT;
				
		return nMinFee;
	}


	std::string CTransaction::ToStringShort() const
	{
		std::string str;
		str += strprintf("%s %s", GetHash().ToString().c_str(), IsCoinBase()? "base" : (IsCoinStake()? "stake" : "user"));
		return str;
	}

	std::string CTransaction::ToString() const
	{
		std::string str;
		str += IsCoinBase() ? "Coinbase" : (IsGenesis() ? "Genesis" : (IsTrust() ? "Trust" : "Transaction"));
		str += strprintf("(hash=%s, nTime=%d, ver=%d, vin.size=%d, vout.size=%d, nLockTime=%d)\n",
			GetHash().ToString().substr(0,10).c_str(),
			nTime,
			nVersion,
			vin.size(),
			vout.size(),
			nLockTime);
		
		for (unsigned int i = 0; i < vin.size(); i++)
			str += "    " + vin[i].ToString() + "\n";
		for (unsigned int i = 0; i < vout.size(); i++)
			str += "    " + vout[i].ToString() + "\n";
		return str;
	}

	void CTransaction::print() const
	{
		printf("%s", ToString().c_str());
	}
	
	
	bool CTransaction::ReadFromDisk(CDiskTxPos pos, FILE** pfileRet)
	{
		CAutoFile filein = CAutoFile(OpenBlockFile(pos.nFile, 0, pfileRet ? "rb+" : "rb"), SER_DISK, DATABASE_VERSION);
		if (!filein)
			return error("CTransaction::ReadFromDisk() : OpenBlockFile failed");

		// Read transaction
		if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
			return error("CTransaction::ReadFromDisk() : fseek failed");

		try {
			filein >> *this;
		}
		catch (std::exception &e) {
			return error("%s() : deserialize or I/O error", __PRETTY_FUNCTION__);
		}

		// Return file pointer
		if (pfileRet)
		{
			if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
				return error("CTransaction::ReadFromDisk() : second fseek failed");
			*pfileRet = filein.release();
		}
		return true;
	}
		

	bool CTransaction::ReadFromDisk(LLD::CIndexDB& indexdb, COutPoint prevout, CTxIndex& txindexRet)
	{
		SetNull();
		if (!indexdb.ReadTxIndex(prevout.hash, txindexRet))
			return false;
		if (!ReadFromDisk(txindexRet.pos))
			return false;
		if (prevout.n >= vout.size())
		{
			SetNull();
			return false;
		}
		return true;
	}

	bool CTransaction::ReadFromDisk(LLD::CIndexDB& indexdb, COutPoint prevout)
	{
		CTxIndex txindex;
		return ReadFromDisk(indexdb, prevout, txindex);
	}

	bool CTransaction::ReadFromDisk(COutPoint prevout)
	{
		LLD::CIndexDB indexdb("r");
		CTxIndex txindex;
		return ReadFromDisk(indexdb, prevout, txindex);
	}

	bool CTransaction::IsStandard() const
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
			if (!Wallet::IsStandard(txout.scriptPubKey))
				return false;
		return true;
	}

	//
	// Check transaction inputs, and make sure any
	// pay-to-script-hash transactions are evaluating IsStandard scripts
	//
	// Why bother? To avoid denial-of-service attacks; an attacker
	// can submit a standard HASH... OP_EQUAL transaction,
	// which will get accepted into blocks. The redemption
	// script can be anything; an attacker could use a very
	// expensive-to-check-upon-redemption script like:
	//   DUP CHECKSIG DROP ... repeated 100 times... OP_1
	//
	bool CTransaction::AreInputsStandard(const MapPrevTx& mapInputs) const
	{
		if (IsCoinBase())
			return true; // Coinbases don't use vin normally

		for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
		{
			const CTxOut& prev = GetOutputFor(vin[i], mapInputs);

			vector<vector<unsigned char> > vSolutions;
			Wallet::TransactionType whichType;
			// get the scriptPubKey corresponding to this input:
			const Wallet::CScript& prevScript = prev.scriptPubKey;
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
			vector<vector<unsigned char> > stack;
			if (!EvalScript(stack, vin[i].scriptSig, *this, i, 0))
				return false;

			if (whichType == Wallet::TX_SCRIPTHASH)
			{
				if (stack.empty())
					return false;
				Wallet::CScript subscript(stack.back().begin(), stack.back().end());
				vector<vector<unsigned char> > vSolutions2;
				Wallet::TransactionType whichType2;
				if (!Solver(subscript, whichType2, vSolutions2))
					return false;
				if (whichType2 == Wallet::TX_SCRIPTHASH)
					return false;

				int tmpExpected;
				tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
				if (tmpExpected < 0)
					return false;
				nArgsExpected += tmpExpected;
			}

			if (stack.size() != (unsigned int)nArgsExpected)
				return false;
		}

		return true;
	}

	unsigned int
	CTransaction::GetLegacySigOpCount() const
	{
		unsigned int nSigOps = 0;
		for(auto txin : vin)
		{
			/** Don't count stake signature for operations. **/
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


	int CMerkleTx::SetMerkleBranch(const CBlock* pblock)
	{
		if (fClient)
		{
			if (hashBlock == 0)
				return 0;
		}
		else
		{
			CBlock blockTmp;
			if (pblock == NULL)
			{
				// Load the block this tx is in
				CTxIndex txindex;
				if (!LLD::CIndexDB("r").ReadTxIndex(GetHash(), txindex))
					return 0;
				if (!blockTmp.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos))
					return 0;
				pblock = &blockTmp;
			}

			// Update the tx's hashBlock
			hashBlock = pblock->GetHash();

			// Locate the transaction
			for (nIndex = 0; nIndex < (int)pblock->vtx.size(); nIndex++)
				if (pblock->vtx[nIndex] == *(CTransaction*)this)
					break;
			if (nIndex == (int)pblock->vtx.size())
			{
				vMerkleBranch.clear();
				nIndex = -1;
				printf("ERROR: SetMerkleBranch() : couldn't find tx in block\n");
				return 0;
			}

			// Fill in merkle branch
			vMerkleBranch = pblock->GetMerkleBranch(nIndex);
		}

		// Is the tx in a block that's in the main chain
		map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
		if (mi == mapBlockIndex.end())
			return 0;
		CBlockIndex* pindex = (*mi).second;
		if (!pindex || !pindex->IsInMainChain())
			return 0;

		return pindexBest->nHeight - pindex->nHeight + 1;
	}







	bool CTransaction::CheckTransaction() const
	{
		// Basic checks that don't depend on any context
		if (vin.empty())
			return error("CTransaction::CheckTransaction() : vin empty");
		if (vout.empty())
			return error("CTransaction::CheckTransaction() : vout empty");
			
		// Size limits
		if (::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
			return error("CTransaction::CheckTransaction() : size limits failed");

		// Check for negative or overflow output values
		int64 nValueOut = 0;
		for (int i = 0; i < vout.size(); i++)
		{
			const CTxOut& txout = vout[i];
			if (txout.IsEmpty() && (!IsCoinBase()) && (!IsCoinStake()))
				return error("CTransaction::CheckTransaction() : txout empty for user transaction");
				
			// Nexus: enforce minimum output amount
			if ((!txout.IsEmpty()) && txout.nValue < MIN_TXOUT_AMOUNT)
				return error("CTransaction::CheckTransaction() : txout.nValue below minimum");
				
			if (txout.nValue > MAX_TXOUT_AMOUNT)
				return error("CTransaction::CheckTransaction() : txout.nValue too high");
				
			nValueOut += txout.nValue;
			if (!MoneyRange(nValueOut))
				return error("CTransaction::CheckTransaction() : txout total out of range");
		}

		// Check for duplicate inputs
		set<COutPoint> vInOutPoints;
		for(auto txin : vin)
		{
				
			if (vInOutPoints.count(txin.prevout))
				return false;
				
			vInOutPoints.insert(txin.prevout);
		}

		if (IsCoinBase() || IsCoinStake())
		{
			if (vin[0].scriptSig.size() < 2 || vin[0].scriptSig.size() > 100)
				return error("CTransaction::CheckTransaction() : coinbase script size");
		}
		else
		{
			for(auto txin : vin)
				if (txin.prevout.IsNull())
					return error("CTransaction::CheckTransaction() : prevout is null");
		}

		return true;
	}


	int CMerkleTx::GetDepthInMainChain(CBlockIndex* &pindexRet) const
	{
		if (hashBlock == 0 || nIndex == -1)
			return 0;

		// Find the block it claims to be in
		map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
		if (mi == mapBlockIndex.end())
			return 0;
		CBlockIndex* pindex = (*mi).second;
		if (!pindex || !pindex->IsInMainChain())
			return 0;

		// Make sure the merkle branch connects to this block
		if (!fMerkleVerified)
		{
			if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
				return 0;
			fMerkleVerified = true;
		}

		pindexRet = pindex;
		return pindexBest->nHeight - pindex->nHeight + 1;
	}


	int CMerkleTx::GetBlocksToMaturity() const
	{
		if (!(IsCoinBase() || IsCoinStake()))
			return 0;
		return max(0, (nCoinbaseMaturity + 20) - GetDepthInMainChain());
	}

	int CTxIndex::GetDepthInMainChain() const
	{
		// Read block header
		CBlock block;
		if (!block.ReadFromDisk(pos.nFile, pos.nBlockPos, false))
			return 0;
		// Find the block in the index
		map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(block.GetHash());
		if (mi == mapBlockIndex.end())
			return 0;
		CBlockIndex* pindex = (*mi).second;
		if (!pindex || !pindex->IsInMainChain())
			return 0;
		return 1 + nBestHeight - pindex->nHeight;
	}



	bool CTransaction::DisconnectInputs(LLD::CIndexDB& indexdb)
	{
		// Relinquish previous transactions' spent pointers
		if (!IsCoinBase())
		{
			for(int nIndex = (int) IsCoinStake(); nIndex < vin.size(); nIndex++)
			{
				CTxIn txin = vin[nIndex];
				COutPoint prevout = txin.prevout;

				// Get prev txindex from disk
				CTxIndex txindex;
				if (!indexdb.ReadTxIndex(prevout.hash, txindex))
					return error("DisconnectInputs() : ReadTxIndex failed");

				if (prevout.n >= txindex.vSpent.size())
					return error("DisconnectInputs() : prevout.n out of range");

				// Mark outpoint as not spent
				txindex.vSpent[prevout.n].SetNull();

				// Write back
				if (!indexdb.UpdateTxIndex(prevout.hash, txindex))
					return error("DisconnectInputs() : UpdateTxIndex failed");
			}
		}

		// Remove transaction from index
		// This can fail if a duplicate of this transaction was in a chain that got
		// reorganized away. This is only possible if this transaction was completely
		// spent, so erasing it would be a no-op anway.
		indexdb.EraseTxIndex(*this);

		return true;
	}


	bool CTransaction::FetchInputs(LLD::CIndexDB& indexdb, const map<uint512, CTxIndex>& mapTestPool,
								   bool fBlock, bool fMiner, MapPrevTx& inputsRet, bool& fInvalid)
	{
		// FetchInputs can return false either because we just haven't seen some inputs
		// (in which case the transaction should be stored as an orphan)
		// or because the transaction is malformed (in which case the transaction should
		// be dropped).  If tx is definitely invalid, fInvalid will be set to true.
		fInvalid = false;

		if (IsCoinBase())
			return true; // Coinbase transactions have no inputs to fetch.

		for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
		{
			COutPoint prevout = vin[i].prevout;
			if (inputsRet.count(prevout.hash))
				continue; // Got it already

			// Read txindex
			CTxIndex& txindex = inputsRet[prevout.hash].first;
			bool fFound = true;
			if ((fBlock || fMiner) && mapTestPool.count(prevout.hash))
			{
				// Get txindex from current proposed changes
				txindex = mapTestPool.find(prevout.hash)->second;
			}
			else
			{
				// Read txindex from txdb
				fFound = indexdb.ReadTxIndex(prevout.hash, txindex);
			}
			if (!fFound && (fBlock || fMiner))
				return fMiner ? false : error("FetchInputs() : %s prev tx %s index entry not found", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());

			// Read txPrev
			CTransaction& txPrev = inputsRet[prevout.hash].second;
			if (!fFound || txindex.pos == CDiskTxPos(1,1,1))
			{
				// Get prev tx from single transactions in memory
				if(!pManager->txPool.Get(prevout.hash, txPrev))
					return error("FetchInputs() : %s mempool Tx prev not found %s", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());

				
				if (!fFound)
					txindex.vSpent.resize(txPrev.vout.size());
			}
			else
			{
				// Get prev tx from disk
				if (!txPrev.ReadFromDisk(txindex.pos))
					return error("FetchInputs() : %s ReadFromDisk prev tx %s failed", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());
			}
		}

		// Make sure all prevout.n's are valid:
		for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
		{
			const COutPoint prevout = vin[i].prevout;
			assert(inputsRet.count(prevout.hash) != 0);
			const CTxIndex& txindex = inputsRet[prevout.hash].first;
			const CTransaction& txPrev = inputsRet[prevout.hash].second;
			if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
			{
				// Revisit this if/when transaction replacement is implemented and allows
				// adding inputs:
				fInvalid = true;
				return error("FetchInputs() : %s prevout.n out of range %d %d %d prev tx %s\n%s", GetHash().ToString().substr(0,10).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,10).c_str(), txPrev.ToString().c_str());
			}
		}

		return true;
	}

	const CTxOut& CTransaction::GetOutputFor(const CTxIn& input, const MapPrevTx& inputs) const
	{
		MapPrevTx::const_iterator mi = inputs.find(input.prevout.hash);
		if (mi == inputs.end())
			throw std::runtime_error("CTransaction::GetOutputFor() : prevout.hash not found");

		const CTransaction& txPrev = (mi->second).second;
		if (input.prevout.n >= txPrev.vout.size())
			throw std::runtime_error("CTransaction::GetOutputFor() : prevout.n out of range");

		return txPrev.vout[input.prevout.n];
	}

	int64 CTransaction::GetValueIn(const MapPrevTx& inputs) const
	{
		if (IsCoinBase())
			return 0;

		int64 nResult = 0;
		for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
		{
			nResult += GetOutputFor(vin[i], inputs).nValue;
		}
		return nResult;

	}

	unsigned int CTransaction::TotalSigOps(const MapPrevTx& inputs) const
	{
		if (IsCoinBase())
			return 0;

		unsigned int nSigOps = 0;
		for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
		{
			const CTxOut& prevout = GetOutputFor(vin[i], inputs);
			nSigOps += prevout.scriptPubKey.GetSigOpCount(vin[i].scriptSig);
		}
		return nSigOps;
	}

	bool CTransaction::ConnectInputs(LLD::CIndexDB& indexdb, MapPrevTx inputs,
									 map<uint512, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx,
									 const CBlockIndex* pindexBlock, bool fBlock, bool fMiner)
	{
		// Take over previous transactions' spent pointers
		// fBlock is true when this is called from AcceptBlock when a new best-block is added to the blockchain
		// ... both are false when called from CTransaction::AcceptToMemoryPool
		if (!IsCoinBase())
		{
			int64 nValueIn = 0;
			for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
			{
				COutPoint prevout = vin[i].prevout;
				assert(inputs.count(prevout.hash) > 0);
				CTxIndex& txindex = inputs[prevout.hash].first;
				CTransaction& txPrev = inputs[prevout.hash].second;

				if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
					return error("ConnectInputs() : %s prevout.n out of range %d %d %d prev tx %s\n%s", GetHash().ToString().substr(0,10).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,10).c_str(), txPrev.ToString().c_str());

				// If prev is coinbase/coinstake, check that it's matured
				if (txPrev.IsCoinBase() || txPrev.IsCoinStake())
					for (const CBlockIndex* pindex = pindexBlock; pindex && pindexBlock->nHeight - pindex->nHeight < nCoinbaseMaturity; pindex = pindex->pprev)
						if (pindex->nBlockPos == txindex.pos.nBlockPos && pindex->nFile == txindex.pos.nFile)
							return error("ConnectInputs() : tried to spend coinbase/coinstake at depth %d", pindexBlock->nHeight - pindex->nHeight);

				// Nexus: check transaction timestamp
				if (txPrev.nTime > nTime)
					return error("ConnectInputs() : transaction timestamp earlier than input transaction");

				// Check for negative or overflow input values
				nValueIn += txPrev.vout[prevout.n].nValue;
				if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
					return error("ConnectInputs() : txin values out of range");

			}
			// The first loop above does all the inexpensive checks.
			// Only if ALL inputs pass do we perform expensive ECDSA signature checks.
			// Helps prevent CPU exhaustion attacks.
			for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
			{
				COutPoint prevout = vin[i].prevout;
				assert(inputs.count(prevout.hash) > 0);
				CTxIndex& txindex = inputs[prevout.hash].first;
				CTransaction& txPrev = inputs[prevout.hash].second;
				
				// Check for conflicts (double-spend)
				// This doesn't trigger the DoS code on purpose; if it did, it would make it easier
				// for an attacker to attempt to split the network.
				if (!txindex.vSpent[prevout.n].IsNull())
					return error("ConnectInputs() : %s prev tx already used at %s", GetHash().ToString().substr(0,10).c_str(), txindex.vSpent[prevout.n].ToString().c_str());
					
				// Skip ECDSA signature verification when connecting blocks (fBlock=true)
				// before the last blockchain checkpoint. This is safe because block merkle hashes are
				// still computed and checked, and any change will be caught at the next checkpoint.
				if (!fMiner && !Wallet::VerifySignature(txPrev, *this, i, 0))
					return error("ConnectInputs() : %s Wallet::VerifySignature failed prev %s", GetHash().ToString().substr(0,10).c_str(), txPrev.GetHash().ToString().substr(0, 10).c_str());

				// Mark outpoints as spent
				txindex.vSpent[prevout.n] = posThisTx;

				// Write back
				if (fBlock || fMiner)
				{
					mapTestPool[prevout.hash] = txindex;
				}
			}

			if (IsCoinStake())
			{
				int64 nInterest;
				GetCoinstakeInterest(indexdb, nInterest);
				
				if(GetArg("-verbose", 0) >= 2)
					printf("ConnectInputs() : %f Value Out, %f Interest, %f Expected\n", (double)vout[0].nValue / COIN, (double)nInterest / COIN, (double)(nInterest + nValueIn) / COIN);
				
				if (vout[0].nValue != (nInterest + nValueIn))
					return error("ConnectInputs() : %s stake reward mismatch", GetHash().ToString().substr(0,10).c_str());
					
			}
			else if (nValueIn < GetValueOut())
				return error("ConnectInputs() : %s value in < value out", GetHash().ToString().substr(0,10).c_str());
				
		}

		return true;
	}


	bool CTransaction::ClientConnectInputs()
	{
		if (IsCoinBase())
			return false;

		// Take over previous transactions' spent pointers
		{
			int64 nValueIn = 0;
			for (unsigned int i = (int) IsCoinStake(); i < vin.size(); i++)
			{
				// Get prev tx from single transactions in memory
				COutPoint prevout = vin[i].prevout;
				CTransaction txPrev;
				if(!pManager->txPool.Get(prevout.hash, txPrev))
					return false;

				if (prevout.n >= txPrev.vout.size())
					return false;

				// Verify signature
				if (!Wallet::VerifySignature(txPrev, *this, i, 0))
					return error("ConnectInputs() : Wallet::VerifySignature failed");

				nValueIn += txPrev.vout[prevout.n].nValue;

				if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
					return error("ClientConnectInputs() : txin values out of range");
			}
			if (GetValueOut() > nValueIn)
				return false;
		}

		return true;
	}

	// Return transaction in tx, and if it was found inside a block, its hash is placed in hashBlock
	bool GetTransaction(const uint512 &hash, CTransaction &tx, uint1024 &hashBlock)
	{
		if (pManager->txPool.Has(hash))
			return pManager->txPool.Get(hash, tx);
			
		LLD::CIndexDB txdb("r");
		CTxIndex txindex;
		if (tx.ReadFromDisk(txdb, COutPoint(hash, 0), txindex))
		{
			CBlock block;
			if (block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
				hashBlock = block.GetHash();
			return true;
		}
		
		return false;
	}

}


bool Wallet::CWalletTx::AcceptWalletTransaction(LLD::CIndexDB& indexdb, bool fCheckInputs)
{
	// Add previous supporting transactions first
	for(auto tx : vtxPrev)
	{
		if (!(tx.IsCoinBase() || tx.IsCoinStake()))
		{
			uint512 hash = tx.GetHash();
			if (!Core::pManager->txPool.Has(hash) && !indexdb.ContainsTx(hash))
				Core::pManager->txPool.Accept(indexdb, tx, fCheckInputs);
		}
	}
		
	return Core::pManager->txPool.Accept(indexdb, *this, fCheckInputs);
}

bool Wallet::CWalletTx::AcceptWalletTransaction()
{
	LLD::CIndexDB indexdb("r");
	return AcceptWalletTransaction(indexdb);
}


