/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "../main.h"

#include "../wallet/db.h"
#include "../util/ui_interface.h"

#include "../LLD/index.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

namespace Core
{

    void CTxMemPool::queryHashes(std::vector<uint512_t>& vtxid)
    {
        vtxid.clear();

        LOCK(cs);
        vtxid.reserve(mapTx.size());
        for (map<uint512_t, CTransaction>::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
            vtxid.push_back((*mi).first);
    }


    bool AddOrphanTx(const CDataStream& vMsg)
    {
        CTransaction tx;
        CDataStream(vMsg) >> tx;
        uint512_t hash = tx.GetHash();
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
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
            mapOrphanTransactionsByPrev[txin.prevout.hash].insert(make_pair(hash, pvMsg));

        printf("stored orphan tx %s (mapsz %u)\n", hash.ToString().substr(0,10).c_str(),
            mapOrphanTransactions.size());
        return true;
    }

    void EraseOrphanTx(uint512_t hash)
    {
        if (!mapOrphanTransactions.count(hash))
            return;
        const CDataStream* pvMsg = mapOrphanTransactions[hash];
        CTransaction tx;
        CDataStream(*pvMsg) >> tx;
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            mapOrphanTransactionsByPrev[txin.prevout.hash].erase(hash);
            if (mapOrphanTransactionsByPrev[txin.prevout.hash].empty())
                mapOrphanTransactionsByPrev.erase(txin.prevout.hash);
        }
        delete pvMsg;
        mapOrphanTransactions.erase(hash);
    }

    uint32_t LimitOrphanTxSize(uint32_t nMaxOrphans)
    {
        uint32_t nEvicted = 0;
        while (mapOrphanTransactions.size() > nMaxOrphans)
        {
            // Evict a random orphan:
            uint512_t randomhash = GetRand512();
            map<uint512_t, CDataStream*>::iterator it = mapOrphanTransactions.lower_bound(randomhash);
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
        BOOST_FOREACH(const CTxIn& txin, vin)
        {
            // Biggest 'standard' txin is a 3-signature 3-of-3 CHECKMULTISIG
            // pay-to-script-hash, which is 3 ~80-byte signatures, 3
            // ~65-byte public keys, plus a few script ops.
            if (txin.scriptSig.size() > 500)
                return false;
            if (!txin.scriptSig.IsPushOnly())
                return false;
        }
        BOOST_FOREACH(const CTxOut& txout, vout)
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

        for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
        {
            const CTxOut& prev = GetOutputFor(vin[i], mapInputs);

            vector<vector<uint8_t> > vSolutions;
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
            vector<vector<uint8_t> > stack;
            if (!EvalScript(stack, vin[i].scriptSig, *this, i, 0))
                return false;

            if (whichType == Wallet::TX_SCRIPTHASH)
            {
                if (stack.empty())
                    return false;
                Wallet::CScript subscript(stack.back().begin(), stack.back().end());
                vector<vector<uint8_t> > vSolutions2;
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

            if (stack.size() != (uint32_t)nArgsExpected)
                return false;
        }

        return true;
    }

    uint32_t
    CTransaction::GetLegacySigOpCount() const
    {
        uint32_t nSigOps = 0;
        BOOST_FOREACH(const CTxIn& txin, vin)
        {
            /** Don't count stake signature for operations. **/
            if(txin.IsStakeSig())
                continue;

            nSigOps += txin.scriptSig.GetSigOpCount(false);
        }
        BOOST_FOREACH(const CTxOut& txout, vout)
        {
            nSigOps += txout.scriptPubKey.GetSigOpCount(false);
        }
        return nSigOps;
    }


    int CMerkleTx::SetMerkleBranch(const CBlock* pblock)
    {
        if (Net::fClient)
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
        map<uint1024_t, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
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
            return DoS(10, error("CTransaction::CheckTransaction() : vin empty"));
        if (vout.empty())
            return DoS(10, error("CTransaction::CheckTransaction() : vout empty"));

        // Size limits
        if (::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
            return DoS(100, error("CTransaction::CheckTransaction() : size limits failed"));

        // Check for negative or overflow output values
        int64_t nValueOut = 0;
        for (int i = 0; i < vout.size(); i++)
        {
            const CTxOut& txout = vout[i];
            if (txout.IsEmpty() && (!IsCoinBase()) && (!IsCoinStake()))
                return DoS(100, error("CTransaction::CheckTransaction() : txout empty for user transaction"));

            // Nexus: enforce minimum output amount
            if ((!txout.IsEmpty()) && txout.nValue < MIN_TXOUT_AMOUNT)
                return DoS(100, error("CTransaction::CheckTransaction() : txout.nValue below minimum"));

            if (txout.nValue > MaxTxOut())
                return DoS(100, error("CTransaction::CheckTransaction() : txout.nValue too high"));

            nValueOut += txout.nValue;
            if (!MoneyRange(nValueOut))
                return DoS(100, error("CTransaction::CheckTransaction() : txout total out of range"));
        }

        // Check for duplicate inputs
        set<COutPoint> vInOutPoints;
        BOOST_FOREACH(const CTxIn& txin, vin)
        {

            if (vInOutPoints.count(txin.prevout))
                return false;

            vInOutPoints.insert(txin.prevout);
        }

        if (IsCoinBase() || IsCoinStake())
        {
            if (vin[0].scriptSig.size() < 2 || vin[0].scriptSig.size() > 100)
                return DoS(100, error("CTransaction::CheckTransaction() : coinbase script size"));
        }
        else
        {
            BOOST_FOREACH(const CTxIn& txin, vin)
                if (txin.prevout.IsNull())
                    return DoS(10, error("CTransaction::CheckTransaction() : prevout is null"));
        }

        return true;
    }

    bool CTxMemPool::accept(LLD::CIndexDB& indexdb, CTransaction &tx, bool fCheckInputs,
                            bool* pfMissingInputs)
    {
        if (pfMissingInputs)
            *pfMissingInputs = false;

        if (!tx.CheckTransaction())
            return error("CTxMemPool::accept() : CheckTransaction failed");

        // Coinbase is only valid in a block, not as a loose transaction
        if (tx.IsCoinBase())
            return tx.DoS(100, error("CTxMemPool::accept() : coinbase as individual tx"));

        // Nexus: coinstake is also only valid in a block, not as a loose transaction
        if (tx.IsCoinStake())
            return tx.DoS(100, error("CTxMemPool::accept() : coinstake as individual tx"));

        // To help v0.1.5 clients who would see it as a negative number
        if ((int64_t)tx.nLockTime > std::numeric_limits<int>::max())
            return error("CTxMemPool::accept() : not accepting nLockTime beyond 2038 yet");

        // Rather not work on nonstandard transactions (unless -testnet)
        if (!fTestNet && !tx.IsStandard())
            return error("CTxMemPool::accept() : nonstandard transaction type");

        // Do we already have it?
        uint512_t hash = tx.GetHash();
        {
            LOCK(cs);
            if (mapTx.count(hash))
                return false;
        }
        if (fCheckInputs)
            if (indexdb.ContainsTx(hash))
                return false;

        // Check for conflicts with in-memory transactions
        CTransaction* ptxOld = NULL;
        for (uint32_t i = 0; i < tx.vin.size(); i++)
        {
            COutPoint outpoint = tx.vin[i].prevout;
            if (mapNextTx.count(outpoint))
            {
                // Disable replacement feature for now
                return false;

                // Allow replacing with a newer version of the same transaction
                if (i != 0)
                    return false;
                ptxOld = mapNextTx[outpoint].ptx;
                if (ptxOld->IsFinal())
                    return false;
                if (!tx.IsNewerThan(*ptxOld))
                    return false;
                for (uint32_t i = 0; i < tx.vin.size(); i++)
                {
                    COutPoint outpoint = tx.vin[i].prevout;
                    if (!mapNextTx.count(outpoint) || mapNextTx[outpoint].ptx != ptxOld)
                        return false;
                }
                break;
            }
        }

        if (fCheckInputs)
        {
            MapPrevTx mapInputs;
            map<uint512_t, CTxIndex> mapUnused;
            bool fInvalid = false;
            if (!tx.FetchInputs(indexdb, mapUnused, false, false, mapInputs, fInvalid))
            {
                if (fInvalid)
                    return error("CTxMemPool::accept() : FetchInputs found invalid tx %s", hash.ToString().substr(0,10).c_str());
                if (pfMissingInputs)
                    *pfMissingInputs = true;
                return error("CTxMemPool::accept() : FetchInputs failed %s", hash.ToString().substr(0,10).c_str());
            }

            // Check for non-standard pay-to-script-hash in inputs
            if (!tx.AreInputsStandard(mapInputs) && !fTestNet)
                return error("CTxMemPool::accept() : nonstandard transaction input");

            // Note: if you modify this code to accept non-standard transactions, then
            // you should add code here to check that the transaction does a
            // reasonable number of ECDSA signature verifications.

            int64_t nFees = tx.GetValueIn(mapInputs)-tx.GetValueOut();
            uint32_t nSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

            // Don't accept it if it can't get into a block
            if (nFees < tx.GetMinFee(1000, false, GMF_RELAY))
                return error("CTxMemPool::accept() : not enough fees");

            // Continuously rate-limit free transactions
            // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
            // be annoying or make other's transactions take longer to confirm.
            if (nFees < MIN_RELAY_TX_FEE)
            {
                static CCriticalSection cs;
                static double dFreeCount;
                static int64_t nLastTime;
                int64_t nNow = GetUnifiedTimestamp();

                {
                    LOCK(cs);
                    // Use an exponentially decaying ~10-minute window:
                    dFreeCount *= pow(1.0 - 1.0/600.0, (double)(nNow - nLastTime));
                    nLastTime = nNow;
                    // -limitfreerelay unit is thousand-bytes-per-minute
                    // At default rate it would take over a month to fill 1GB
                    if (dFreeCount > GetArg("-limitfreerelay", 15)*10*1000 && !IsFromMe(tx))
                        return error("CTxMemPool::accept() : free transaction rejected by rate limiter");
                    if (fDebug)
                        printf("Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount+nSize);
                    dFreeCount += nSize;
                }
            }

            // Check against previous transactions
            // This is done last to help prevent CPU exhaustion denial-of-service attacks.
            if (!tx.ConnectInputs(indexdb, mapInputs, mapUnused, CDiskTxPos(1,1,1), pindexBest, false, false))
            {
                return error("CTxMemPool::accept() : ConnectInputs failed %s", hash.ToString().substr(0,10).c_str());
            }
        }

        // Store transaction in memory
        {
            LOCK(cs);
            if (ptxOld)
            {
                printf("CTxMemPool::accept() : replacing tx %s with new version\n", ptxOld->GetHash().ToString().c_str());
                remove(*ptxOld);
            }
            addUnchecked(tx);
        }

        ///// are we sure this is ok when loading transactions or restoring block txes
        // If updated, erase old tx from wallet
        if (ptxOld)
            EraseFromWallets(ptxOld->GetHash());

        printf("CTxMemPool::accept() : accepted %s\n", hash.ToString().substr(0,10).c_str());
        return true;
    }

    bool CTransaction::AcceptToMemoryPool(LLD::CIndexDB& indexdb, bool fCheckInputs, bool* pfMissingInputs)
    {
        return mempool.accept(indexdb, *this, fCheckInputs, pfMissingInputs);
    }

    bool CTxMemPool::addUnchecked(CTransaction &tx)
    {
        printf("addUnchecked(): size %lu\n",  mapTx.size());
        // Add to memory pool without checking anything.  Don't call this directly,
        // call CTxMemPool::accept to properly check the transaction first.
        {
            LOCK(cs);
            uint512_t hash = tx.GetHash();
            mapTx[hash] = tx;
            for (uint32_t i = 0; i < tx.vin.size(); i++)
                mapNextTx[tx.vin[i].prevout] = CInPoint(&mapTx[hash], i);

        }
        return true;
    }


    bool CTxMemPool::remove(CTransaction &tx)
    {
        // Remove transaction from memory pool
        {
            LOCK(cs);
            uint512_t hash = tx.GetHash();
            if (mapTx.count(hash))
            {
                BOOST_FOREACH(const CTxIn& txin, tx.vin)
                    mapNextTx.erase(txin.prevout);
                mapTx.erase(hash);

            }
        }
        return true;
    }






    int CMerkleTx::GetDepthInMainChain(CBlockIndex* &pindexRet) const
    {
        if (hashBlock == 0 || nIndex == -1)
            return 0;

        // Find the block it claims to be in
        map<uint1024_t, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
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


    bool CMerkleTx::AcceptToMemoryPool(LLD::CIndexDB& indexdb, bool fCheckInputs)
    {
        if (Net::fClient)
        {
            if (!IsInMainChain() && !ClientConnectInputs())
                return false;
            return CTransaction::AcceptToMemoryPool(indexdb, false);
        }
        else
        {
            return CTransaction::AcceptToMemoryPool(indexdb, fCheckInputs);
        }
    }

    bool CMerkleTx::AcceptToMemoryPool()
    {
        LLD::CIndexDB indexdb("r");
        return AcceptToMemoryPool(indexdb);
    }

    int CTxIndex::GetDepthInMainChain() const
    {
        // Read block header
        CBlock block;
        if (!block.ReadFromDisk(pos.nFile, pos.nBlockPos, false))
            return 0;
        // Find the block in the index
        map<uint1024_t, CBlockIndex*>::iterator mi = mapBlockIndex.find(block.GetHash());
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


    bool CTransaction::FetchInputs(LLD::CIndexDB& indexdb, const map<uint512_t, CTxIndex>& mapTestPool,
                                   bool fBlock, bool fMiner, MapPrevTx& inputsRet, bool& fInvalid)
    {
        // FetchInputs can return false either because we just haven't seen some inputs
        // (in which case the transaction should be stored as an orphan)
        // or because the transaction is malformed (in which case the transaction should
        // be dropped).  If tx is definitely invalid, fInvalid will be set to true.
        fInvalid = false;

        if (IsCoinBase())
            return true; // Coinbase transactions have no inputs to fetch.

        for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
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
                {
                    LOCK(mempool.cs);
                    if (!mempool.exists(prevout.hash))
                        return error("FetchInputs() : %s mempool Tx prev not found %s", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());
                    txPrev = mempool.lookup(prevout.hash);
                }
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
        for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
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
                return DoS(100, error("FetchInputs() : %s prevout.n out of range %d %d %d prev tx %s\n%s", GetHash().ToString().substr(0,10).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,10).c_str(), txPrev.ToString().c_str()));
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

    int64_t CTransaction::GetValueIn(const MapPrevTx& inputs) const
    {
        if (IsCoinBase())
            return 0;

        int64_t nResult = 0;
        for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
        {
            nResult += GetOutputFor(vin[i], inputs).nValue;
        }
        return nResult;

    }

    uint32_t CTransaction::TotalSigOps(const MapPrevTx& inputs) const
    {
        if (IsCoinBase())
            return 0;

        uint32_t nSigOps = 0;
        for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
        {
            const CTxOut& prevout = GetOutputFor(vin[i], inputs);
            nSigOps += prevout.scriptPubKey.GetSigOpCount(vin[i].scriptSig);
        }
        return nSigOps;
    }

    bool CTransaction::ConnectInputs(LLD::CIndexDB& indexdb, MapPrevTx inputs,
                                     map<uint512_t, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx,
                                     const CBlockIndex* pindexBlock, bool fBlock, bool fMiner)
    {
        // Take over previous transactions' spent pointers
        // fBlock is true when this is called from AcceptBlock when a new best-block is added to the blockchain
        // ... both are false when called from CTransaction::AcceptToMemoryPool
        if (!IsCoinBase())
        {
            int64_t nValueIn = 0;
            for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
            {
                COutPoint prevout = vin[i].prevout;
                assert(inputs.count(prevout.hash) > 0);
                CTxIndex& txindex = inputs[prevout.hash].first;
                CTransaction& txPrev = inputs[prevout.hash].second;

                if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
                    return DoS(100, error("ConnectInputs() : %s prevout.n out of range %d %d %d prev tx %s\n%s", GetHash().ToString().substr(0,10).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,10).c_str(), txPrev.ToString().c_str()));

                // If prev is coinbase/coinstake, check that it's matured
                if (txPrev.IsCoinBase() || txPrev.IsCoinStake())
                    for (const CBlockIndex* pindex = pindexBlock; pindex && pindexBlock->nHeight - pindex->nHeight < nCoinbaseMaturity; pindex = pindex->pprev)
                        if (pindex->nBlockPos == txindex.pos.nBlockPos && pindex->nFile == txindex.pos.nFile)
                            return error("ConnectInputs() : tried to spend coinbase/coinstake at depth %d", pindexBlock->nHeight - pindex->nHeight);

                // Nexus: check transaction timestamp
                if (txPrev.nTime > nTime)
                    return DoS(100, error("ConnectInputs() : transaction timestamp earlier than input transaction"));

                // Check for negative or overflow input values
                nValueIn += txPrev.vout[prevout.n].nValue;
                if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                    return DoS(100, error("ConnectInputs() : txin values out of range"));

            }
            // The first loop above does all the inexpensive checks.
            // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
            // Helps prevent CPU exhaustion attacks.
            for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
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

                // Skip ECDSA signature verification when mining blocks (fMiner=true) since they are already checked on memory pool
                if (!IsInitialBlockDownload() && !fMiner && !Wallet::VerifySignature(txPrev, *this, i, 0))
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
                int64_t nInterest;
                GetCoinstakeInterest(indexdb, nInterest);

                printf("ConnectInputs() : %f Value Out, %f Expected\n", (double)round_coin_digits(vout[0].nValue, 3) / COIN, (double)(round_coin_digits(nInterest + nValueIn, 3)) / COIN);

                if (round_coin_digits(vout[0].nValue, 3) > round_coin_digits((nInterest + nValueIn), 3))
                    return DoS(100, error("ConnectInputs() : %s stake reward mismatch", GetHash().ToString().substr(0,10).c_str()));

            }
            else if (nValueIn < GetValueOut())
                return DoS(100, error("ConnectInputs() : %s value in < value out", GetHash().ToString().substr(0,10).c_str()));

        }

        return true;
    }


    bool CTransaction::ClientConnectInputs()
    {
        if (IsCoinBase())
            return false;

        // Take over previous transactions' spent pointers
        {
            LOCK(mempool.cs);
            int64_t nValueIn = 0;
            for (uint32_t i = (int) IsCoinStake(); i < vin.size(); i++)
            {
                // Get prev tx from single transactions in memory
                COutPoint prevout = vin[i].prevout;
                if (!mempool.exists(prevout.hash))
                    return false;
                CTransaction& txPrev = mempool.lookup(prevout.hash);

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
bool GetTransaction(const uint512_t &hash, CTransaction &tx, uint1024_t &hashBlock)
{
    {
        LOCK(cs_main);
        {
            LOCK(mempool.cs);
            if (mempool.exists(hash))
            {
                tx = mempool.lookup(hash);
                return true;
            }
        }
        LLD::CIndexDB txdb("r");
        CTxIndex txindex;
        if (tx.ReadFromDisk(txdb, COutPoint(hash, 0), txindex))
        {
            CBlock block;
            if (block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
                hashBlock = block.GetHash();
            return true;
        }
    }
    return false;
}

}

bool Wallet::CWalletTx::AcceptWalletTransaction(LLD::CIndexDB& indexdb, bool fCheckInputs)
{

    {
        LOCK(Core::mempool.cs);
        // Add previous supporting transactions first
        BOOST_FOREACH(Core::CMerkleTx& tx, vtxPrev)
        {
            if (!(tx.IsCoinBase() || tx.IsCoinStake()))
            {
                uint512_t hash = tx.GetHash();
                if (!Core::mempool.exists(hash) && !indexdb.ContainsTx(hash))
                    tx.AcceptToMemoryPool(indexdb, fCheckInputs);
            }
        }
        return AcceptToMemoryPool(indexdb, fCheckInputs);
    }
    return false;
}

bool Wallet::CWalletTx::AcceptWalletTransaction()
{
    LLD::CIndexDB indexdb("r");
    return AcceptWalletTransaction(indexdb);
}
