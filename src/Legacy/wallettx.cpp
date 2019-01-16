/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <exception>
#include <set>
#include <utility>

#include <LLC/types/uint1024.h>

#include <LLD/include/legacy.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/types/txout.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/wallet/wallettx.h>

#include <Util/include/args.h>

namespace Legacy
{

    /* Assigns the wallet for this wallet transaction. */
    void CWalletTx::BindWallet(CWallet *pwalletIn)
    {
        ptransactionWallet = pwalletIn;
        fHaveWallet = true;
        MarkDirty();
    }


    /* Calculates the total value for all inputs sent by this transaction that belong to the bound wallet. */
    int64_t CWalletTx::GetDebit() const
    {
        /* Return 0 if no wallet bound or inputs not loaded */
        if (!fHaveWallet || vin.empty())
            return 0;

        /* When calculation previously cached, use cached value */
        if (fDebitCached)
            return nDebitCached;

        /* Call corresponding method in wallet that will check which txin entries belong to it */
        nDebitCached = ptransactionWallet->GetDebit(*this);

        /* Set cached flag */
        fDebitCached = true;

        return nDebitCached;
    }


    /* Calculates the total value for all outputs received by this transaction that belong to the bound wallet. */
    int64_t CWalletTx::GetCredit() const
    {
        /* Return 0 if no wallet bound or outputs not loaded */
        if (!fHaveWallet || vout.empty())
            return 0;

        /* Immature transactions give zero credit */
        if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
            return 0;

        /* When calculation previously cached, use cached value */
        if (fCreditCached)
            return nCreditCached;

        /* Call corresponding method in wallet that will check which txout entries belong to it */
        nCreditCached = ptransactionWallet->GetCredit(*this);

        /* Set cached flag */
        fCreditCached = true;

        return nCreditCached;
    }


    /* Calculates the total value for all unspent outputs received by this transaction that belong to the bound wallet. */
    int64_t CWalletTx::GetAvailableCredit() const
    {
        /* Return 0 if no wallet bound or outputs not loaded */
        if (!fHaveWallet || vout.empty())
            return 0;

        /* Immature transactions give zero credit */
        if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
            return 0;

        /* When calculation previously cached, use cached value */
        /* Disable caching -- this was commented out in old QT code, left here for reference
        if (fUseCache && fAvailableCreditCached)
            return nAvailableCreditCached;
        */

        int64_t nCredit = 0;
        for (uint32_t i = 0; i < vout.size(); i++)
        {
            const CTxOut &txout = vout[i];

            /* Calculate credit value only including unspent outputs */
            if (!IsSpent(i) && vout[i].nValue > 0)
            {
                /* Call corresponding method in wallet that will check which txout entries belong to it */
                nCredit += ptransactionWallet->GetCredit(txout);

                if (!Legacy::MoneyRange(nCredit))
                    throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
            }
        }

        /* Store cached value */
        nAvailableCreditCached = nCredit;

        /* Set cached flag */
        fAvailableCreditCached = true;

        return nCredit;
    }


    /* Retrieves any change amount for this transaction that belongs to the bound wallet. */
    int64_t CWalletTx::GetChange() const
    {
        /* Return 0 if no wallet bound or outputs not loaded */
        if (!fHaveWallet || vout.empty())
            return 0;

        /* When calculation previously cached, use cached value */
        if (fChangeCached)
            return nChangeCached;

        /* Call corresponding method in wallet that will find the change transaction, if any */
        nChangeCached = ptransactionWallet->GetChange(*this);

        /* Set cached flag */
        fChangeCached = true;

        return nChangeCached;
    }


    int32_t CWalletTx::GetRequestCount() const
    {
        /* Return 0 if no wallet bound */
        if (!fHaveWallet)
            return 0;

        /* Returns -1 if it wasn't being tracked */
        int32_t nRequests = -1;

        {
            LOCK(ptransactionWallet->cs_wallet);

            if (IsCoinBase() || IsCoinStake())
            {
                /* If this is a block we generated via minting, use request count entry for the block */
                /* Blocks created via staking/mining have block hash added to request tracking */
                if (hashBlock != 0)
                {
                    auto mi = ptransactionWallet->mapRequestCount.find(hashBlock);

                    if (mi != ptransactionWallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                }
            }
            else
            {
                /* Did anyone request this transaction? */
               auto mi = ptransactionWallet->mapRequestCount.find(GetHash());
                if (mi != ptransactionWallet->mapRequestCount.end())
                {
                    nRequests = (*mi).second;

                    /* If no request specifically for the transaction hash, check the count for the block containing it */
                    if (nRequests == 0 && hashBlock != 0)
                    {
                        auto mi = ptransactionWallet->mapRequestCount.find(hashBlock);
                        if (mi != ptransactionWallet->mapRequestCount.end())
                            nRequests = (*mi).second;
                        else
                            nRequests = 1; // If it's in someone else's block it must have got out
                    }
                }
            }
        }

        return nRequests;
    }


    /* Checks whether or not this transaction is confirmed. */
    bool CWalletTx::IsConfirmed() const
    {
        /* Not final = not confirmed */
        if (!IsFinal())
            return false;

        /* If has depth, it is confirmed */
        if (GetDepthInMainChain() >= 1)
            return true;

        /* If transaction not from bound wallet (not ours), treat as unconfirmed */
        if (!IsFromMe())
            return false;

        /* If no confirmations but it's from us, we can still consider it confirmed if all dependencies are confirmed */
        std::map<uint512_t, const CMerkleTx*> mapPrev;

        std::vector<const CMerkleTx*> vWorkQueue;

        vWorkQueue.reserve(vtxPrev.size() + 1);

        /* Seed the for loop with this transaction for the first iteration */
        vWorkQueue.push_back(this);

        /* We only get here if this transaction has depth 0 and it is from us.
         *
         * This is a rather convoluted way to figure this out, but
         * rather than attempt to simplify it am leaving the legacy code as-is
         * in case there is some nuance I'm missing.
         *
         * It is basically saying (like the comment above) that, if this transaction's
         * prevouts are all final and have depth > 0, then this transaction is
         * treated as confirmed even though it has depth 0 itself.
         *
         * When every prevout for this transaction is final and has depth > 0
         * it will continue each iteration until for loop ends and method returns true.
         */
        for (uint32_t i = 0; i < vWorkQueue.size(); i++)
        {
            const CMerkleTx* ptx = vWorkQueue[i];

            if (!ptx->IsFinal())
                return false;

            if (ptx->GetDepthInMainChain() >= 1)
                continue;

            if (!ptransactionWallet->IsFromMe(*ptx))
                return false;

            /* On first iteration, loads mapPrev with vtxPrev from this transaction (this one is first one processed) */
            if (mapPrev.empty())
            {
                for(const auto& prevTx : vtxPrev)
                    mapPrev[prevTx.GetHash()] = &prevTx;
            }

            /* This is repeated each time through the loop for the current transaction
             * It adds all the prevouts to the work queue as long as they are in the mapPrev
             * for this transaction. In reality, it will return false after the initial iteration
             * because previous of previous will not be in the map for this transaction.
             *
             * The only way for the outer for loop to keep going without returning false
             * after initial iteration is for everything loaded in vWorkQueue here to have
             * depth > 0
             */
            for(const auto& txin : ptx->vin)
            {
                if (!mapPrev.count(txin.prevout.hash))
                    return false;

                vWorkQueue.push_back(mapPrev[txin.prevout.hash]);
            }
        }

        return true;
    }


    /* Checks whether a particular output for this transaction is marked as spent */
    bool CWalletTx::IsSpent(const uint32_t nOut) const
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::IsSpent() : nOut out of range");

        /* An valid nOut value not tracked by vfSpent is considered unspent */
        if (nOut >= vfSpent.size())
            return false;

        return vfSpent[nOut];
    }


    /* Clears cache, assuring balances are recalculated. */
    void CWalletTx::MarkDirty()
    {
        fCreditCached = false;
        fAvailableCreditCached = false;
        fDebitCached = false;
        fChangeCached = false;
    }


    /* Flags a given output for this transaction as spent */
    void CWalletTx::MarkSpent(const uint32_t nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::MarkSpent() : nOut out of range");

        if (vfSpent.size() != vout.size())
            vfSpent.resize(vout.size());

        if (!vfSpent[nOut])
        {
            vfSpent[nOut] = true;

            /* Changing spent flags requires recalculating available credit */
            fAvailableCreditCached = false;
        }
    }


    /* Flags a given output for this transaction as unspent */
    void CWalletTx::MarkUnspent(const uint32_t nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::MarkUnspent() : nOut out of range");

        if (vfSpent.size() != vout.size())
            vfSpent.resize(vout.size());

        if (vfSpent[nOut])
        {
            vfSpent[nOut] = false;

            /* Changing spent flags requires recalculating available credit */
            fAvailableCreditCached = false;
        }
    }


    /* Marks one or more outputs for this transaction as spent. */
    bool CWalletTx::UpdateSpent(const std::vector<bool>& vfNewSpent)
    {
        bool fReturn = false;

        if (vfSpent.size() != vout.size())
            vfSpent.resize(vout.size());

        for (uint32_t i = 0; i < vfNewSpent.size(); i++)
        {
            if (i == vfSpent.size())
                break;

            /* Only update if new flag is spent and old is unspent */
            if (vfNewSpent[i] && !vfSpent[i])
            {
                vfSpent[i] = true;
                fReturn = true;

                /* Changing spent flags requires recalculating available credit */
                fAvailableCreditCached = false;
            }
        }

        return fReturn;
    }


    /* Store this transaction in the database for the bound wallet */
    bool CWalletTx::WriteToDisk()
    {
        if (fHaveWallet && ptransactionWallet->IsFileBacked())
        {
            CWalletDB walletDB(ptransactionWallet->GetWalletFile());
            bool ret = walletDB.WriteTx(GetHash(), *this);
            walletDB.Close();

            return ret;
        }

        return true;
    }


    /* Retrieve information about the current transaction. */
   void CWalletTx::GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, std::list<std::pair<NexusAddress, int64_t> >& listReceived,
                              std::list<std::pair<NexusAddress, int64_t> >& listSent, int64_t& nFee, std::string& strSentAccount) const
    {
        nGeneratedImmature = nGeneratedMature = nFee = 0;
        listReceived.clear();
        listSent.clear();
        strSentAccount = strFromAccount;
        if(strSentAccount == "")
            strSentAccount = "default";

        if (IsCoinBase() || IsCoinStake())
        {
            if (GetBlocksToMaturity() > 0)
                nGeneratedImmature = ptransactionWallet->GetCredit(*this);
            else
                nGeneratedMature = GetCredit();

            return;
        }

        /* Compute fee only if transaction is from us */
        int64_t nDebit = GetDebit();
        if (nDebit > 0)
        {
            int64_t nValueOut = GetValueOut();

            /* Fee is total input - total output */
            nFee = nDebit - nValueOut;
        }

        /* Sent/received. */
        for(const CTxOut& txout : vout)
        {
            NexusAddress address;
            std::vector<uint8_t> vchPubKey;

            /* Get the Nexus address from the txout public key */
            if (!ExtractAddress(txout.scriptPubKey, address))
            {
                debug::log(0, "CWalletTx::GetAmounts: Unknown transaction type found, txid ",
                           this->GetHash().ToString());

                address = " unknown ";
            }

            /* For tx from us, txouts represent the sent value, add to the sent list */
            if (nDebit > 0)
                listSent.push_back(std::make_pair(address, txout.nValue));

            /* For txout received (sent to address in bound wallet), add to the received list */
            if (ptransactionWallet->IsMine(txout))
                listReceived.push_back(std::make_pair(address, txout.nValue));
        }
    }


    void CWalletTx::GetAccountAmounts(const std::string& strAccount, int64_t& nGenerated, int64_t& nReceived,
                                      int64_t& nSent, int64_t& nFee) const
    {
        nGenerated = nReceived = nSent = nFee = 0;

        int64_t allGeneratedImmature, allGeneratedMature, allFee;
        allGeneratedImmature = allGeneratedMature = allFee = 0;

        /* GetAmounts will return the strFromAccount for this transaction */
        std::string strSentAccount;

        std::list<std::pair<NexusAddress, int64_t> > listReceived;
        std::list<std::pair<NexusAddress, int64_t> > listSent;

        GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);

        /* If no requested account, set nGenerated to mature Coinbase/Coinstake amount */
        if (strAccount == "" || strAccount == "*")
            nGenerated = allGeneratedMature;

        /* If requested account is the account for this transaction, calculate nSent and nFee for it */
        if (strAccount == strSentAccount)
        {
            for(const auto& s : listSent)
                nSent += s.second;

            nFee = allFee;
        }

        if (fHaveWallet)
        {
            LOCK(ptransactionWallet->cs_wallet);

            for(const auto& r : listReceived)
            {
                if (ptransactionWallet->GetAddressBook().HasAddress(r.first))
                {
                    /* When received Nexus Address (r.first) is in wallet address book,
                     * include it in nReceived amount if its label matches requested account label
                     */
                    if (ptransactionWallet->GetAddressBook().GetAddressBookName(r.first) == strAccount)
                        nReceived += r.second;
                }
                else if (strAccount.empty())
                {
                    /* Received address is not in wallet address book, included in nReceived when no account requested */
                    nReceived += r.second;
                }
            }
        }
    }


    /* Populates transaction data for previous transactions into vtxPrev */
    void CWalletTx::AddSupportingTransactions(LLD::LegacyDB& legacydb)
    {
        vtxPrev.clear();

        const uint32_t COPY_DEPTH = 3;

        /* Calling this for new transaction will return main chain depth of 0 */
        if (fHaveWallet && GetDepthInMainChain() < COPY_DEPTH)
        {
            /* Create list of tx hashes for previous transactions referenced by this transaction's inputs */
            std::vector<uint512_t> vWorkQueue;
            for(const CTxIn& txin : vin)
                vWorkQueue.push_back(txin.prevout.hash);

            { // Begin lock scope
                LOCK(ptransactionWallet->cs_wallet);

                /* Map keeps track of tx previously loaded, while set contains hash values already processed */
                std::map<uint512_t, const CWalletTx*> mapWalletPrev;
                std::set<uint512_t> setAlreadyDone;

                for (uint32_t i = 0; i < vWorkQueue.size(); i++)
                {
                    uint512_t prevoutTxHash = vWorkQueue[i];

                    /* Only need to process each hash once even if referenced multiple times */
                    if (setAlreadyDone.count(prevoutTxHash))
                        continue;

                    setAlreadyDone.insert(prevoutTxHash);

                    CWalletTx tx;
                    Legacy::Transaction parentTransaction;

                    /* Find returns iterator to equivalent of pair<uint512_t, CWalletTx> */
                    auto mi = ptransactionWallet->mapWallet.find(prevoutTxHash);

                    if (mi != ptransactionWallet->mapWallet.end())
                    {
                        /* Found previous transaction (input to this one) in wallet */
                        tx = (*mi).second;

                        /* Copy vtxPrev (inputs) from previous transaction into mapWalletPrev.
                         * This saves them so we don't process them twice if we end up processing
                         * deeper because tx depth is less than copy depth (unlikely, see below)
                         */
                        for(const CWalletTx& txWalletPrev : tx.vtxPrev)
                            mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;

                    }
                    else if (mapWalletPrev.count(prevoutTxHash))
                    {
                        /* Previous transaction not in wallet, but already in mapWalletPrev */
                        tx = *mapWalletPrev[prevoutTxHash];

                    }
                    else if (!config::fClient && legacydb.ReadTx(prevoutTxHash, parentTransaction))
                    {
                        /* Found transaction in database, but it isn't in wallet so don't save */
                    }
                    else
                    {
                        /* Transaction not found */
                        debug::log(0, "CWalletTx::AddSupportingTransactions: Error: AddSupportingTransactions() : unsupported transaction");
                        continue;
                    }

                    uint32_t nDepth = tx.GetDepthInMainChain();
                    vtxPrev.push_back(tx);

                    if (nDepth < COPY_DEPTH)
                    {
                        /* vtxPrev gets loaded with inputs to this transaction, but when one of these inputs
                         * is recent (depth < copy depth) we go one deeper and also load its inputs (inputs of inputs).
                         * This helps assure, when transactions are relayed, that we transmit anything not yet added
                         * to a block and included in legacydb. Obviously, it is unikely that inputs of inputs are
                         * within the copy depth because we'd be spending balance that probably is not completely confirmed,
                         * so this really should never be processed. Code is from legacy and left here intact just in case.
                         */
                        for(const CTxIn& txin : tx.vin)
                            vWorkQueue.push_back(txin.prevout.hash);
                    }
                }
            } // End lock scope
        }

        reverse(vtxPrev.begin(), vtxPrev.end());
    }


    /* Send this transaction to the network if not in our database, yet. */
    void CWalletTx::RelayWalletTransaction(LLD::LegacyDB& legacydb)
    {
        for(const CMerkleTx& tx : vtxPrev)
        {
            /* Also relay any tx in vtxPrev that we don't have in our database, yet */
            if (!(tx.IsCoinBase() || tx.IsCoinStake()))
            {
                uint512_t hash = tx.GetHash();

// TODO: Need implementation to support RelayMessage()
                if (!legacydb.HasTx(hash))
                {
                    //RelayMessage(LLP::CInv(LLP::MSG_TX, hash), (Transaction)tx);
                }
            }
        }

        if (!(IsCoinBase() || IsCoinStake()))
        {
            uint512_t hash = GetHash();

            /* Relay this tx if we don't have it in our database, yet */
            if (!legacydb.HasTx(hash))
            {
                debug::log(0, "Relaying wtx ", hash.ToString().substr(0,10));
// TODO: Need implementation to support RelayMessage()
                //RelayMessage(LLP::CInv(LLP::MSG_TX, hash), (Transaction)*this);
            }
        }
    }


    /* Send this transaction to the network if not in our database, yet. */
    void CWalletTx::RelayWalletTransaction()
    {
        LLD::LegacyDB legacydb(LLD::FLAGS::READONLY);
        RelayWalletTransaction(legacydb);
    }

}
