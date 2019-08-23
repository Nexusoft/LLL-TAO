/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/include/legacy.h>
#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/types/txout.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/wallet/wallettx.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/include/args.h>

#include <exception>
#include <set>
#include <utility>


namespace Legacy
{

    /* Initialize static values */
    std::mutex WalletTx::cs_wallettx;


    /** Constructor **/
    WalletTx::WalletTx()
    : MerkleTx()
    , pWallet(nullptr)
    , fHaveWallet(false)
    , vtxPrev()
    , mapValue()
    , vOrderForm()
    , strFromAccount()
    , vfSpent(false, vout.size())
    , fTimeReceivedIsTxTime(false)
    , nTimeReceived(0)
    , fFromMe(false)
    , fDebitCached(false)
    , fCreditCached(false)
    , fAvailableCreditCached(false)
    , fChangeCached(false)
    , nDebitCached(0)
    , nCreditCached(0)
    , nAvailableCreditCached(0)
    , nChangeCached(0)
    {
    }


    /** Constructor **/
    WalletTx::WalletTx(Wallet& walletIn)
    : MerkleTx()
    , pWallet(&walletIn)
    , fHaveWallet(true)
    , vtxPrev()
    , mapValue()
    , vOrderForm()
    , strFromAccount()
    , vfSpent(false, vout.size())
    , fTimeReceivedIsTxTime(false)
    , nTimeReceived(0)
    , fFromMe(false)
    , fDebitCached(false)
    , fCreditCached(false)
    , fAvailableCreditCached(false)
    , fChangeCached(false)
    , nDebitCached(0)
    , nCreditCached(0)
    , nAvailableCreditCached(0)
    , nChangeCached(0)
    {
    }


    /** Constructor **/
    WalletTx::WalletTx(Wallet* pwalletIn)
    : MerkleTx()
    , pWallet(pwalletIn)
    , fHaveWallet(true)
    , vtxPrev()
    , mapValue()
    , vOrderForm()
    , strFromAccount()
    , vfSpent(false, vout.size())
    , fTimeReceivedIsTxTime(false)
    , nTimeReceived(0)
    , fFromMe(false)
    , fDebitCached(false)
    , fCreditCached(false)
    , fAvailableCreditCached(false)
    , fChangeCached(false)
    , nDebitCached(0)
    , nCreditCached(0)
    , nAvailableCreditCached(0)
    , nChangeCached(0)
    {
    }


    /** Constructor **/
    WalletTx::WalletTx(Wallet* pwalletIn, const MerkleTx& txIn)
    : MerkleTx(txIn)
    , pWallet(pwalletIn)
    , fHaveWallet(true)
    , vtxPrev()
    , mapValue()
    , vOrderForm()
    , strFromAccount()
    , vfSpent(false, vout.size())
    , fTimeReceivedIsTxTime(false)
    , nTimeReceived(0)
    , fFromMe(false)
    , fDebitCached(false)
    , fCreditCached(false)
    , fAvailableCreditCached(false)
    , fChangeCached(false)
    , nDebitCached(0)
    , nCreditCached(0)
    , nAvailableCreditCached(0)
    , nChangeCached(0)
    {
    }


    /** Constructor **/
    WalletTx::WalletTx(Wallet* pwalletIn, const Transaction& txIn)
    : MerkleTx(txIn)
    , pWallet(pwalletIn)
    , fHaveWallet(true)
    , vtxPrev()
    , mapValue()
    , vOrderForm()
    , strFromAccount()
    , vfSpent(false, vout.size())
    , fTimeReceivedIsTxTime(false)
    , nTimeReceived(0)
    , fFromMe(false)
    , fDebitCached(false)
    , fCreditCached(false)
    , fAvailableCreditCached(false)
    , fChangeCached(false)
    , nDebitCached(0)
    , nCreditCached(0)
    , nAvailableCreditCached(0)
    , nChangeCached(0)
    {
    }


    /** Destructor **/
    WalletTx::~WalletTx()
    {
    }


    /*  Initializes an empty wallet transaction */
    void WalletTx::InitWalletTx()
    {
        vtxPrev.clear();
        mapValue.clear();
        vOrderForm.clear();
        strFromAccount.clear();
        vfSpent.clear();

        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        fFromMe = false;
        fDebitCached = false;
        fCreditCached = false;
        fAvailableCreditCached = false;
        fChangeCached = false;
        nDebitCached = 0;
        nCreditCached = 0;
        nAvailableCreditCached = 0;
        nChangeCached = 0;
    }


    /* Assigns the wallet for this wallet transaction. */
    void WalletTx::BindWallet(Wallet& walletIn)
    {
        BindWallet(&walletIn);
    }


    /* Assigns the wallet for this wallet transaction. */
    void WalletTx::BindWallet(Wallet* pwalletIn)
    {
        pWallet = pwalletIn;
        fHaveWallet = true;
        MarkDirty();
    }


    /*  Test whether transaction is bound to a wallet. */
    bool WalletTx::IsBound() const
    {
        return fHaveWallet;
    }


    /* Calculates the total value for all inputs sent by this transaction that belong to the bound wallet. */
    int64_t WalletTx::GetDebit() const
    {
        /* Return 0 if no wallet bound or inputs not loaded */
        if (!IsBound() || vin.empty())
            return 0;

        /* When calculation previously cached, use cached value */
        if (fDebitCached)
            return nDebitCached;

        /* Call corresponding method in wallet that will check which txin entries belong to it */
        nDebitCached = pWallet->GetDebit(*this);

        /* Set cached flag */
        fDebitCached = true;

        return nDebitCached;
    }


    /* Calculates the total value for all outputs received by this transaction that belong to the bound wallet. */
    int64_t WalletTx::GetCredit() const
    {
        /* Return 0 if no wallet bound or outputs not loaded */
        if (!IsBound() || vout.empty())
            return 0;

        /* Immature transactions give zero credit */
        if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
            return 0;

        /* When calculation previously cached, use cached value */
        if (fCreditCached)
            return nCreditCached;

        /* Call corresponding method in wallet that will check which txout entries belong to it */
        nCreditCached = pWallet->GetCredit(*this);

        /* Set cached flag */
        fCreditCached = true;

        return nCreditCached;
    }


    /* Calculates the total value for all unspent outputs received by this transaction that belong to the bound wallet. */
    int64_t WalletTx::GetAvailableCredit() const
    {
        /* Return 0 if no wallet bound or outputs not loaded */
        if (!IsBound() || vout.empty())
            return 0;

        /* Immature transactions give zero credit */
        if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
            return 0;

        /* When calculation previously cached, use cached value */
        /* Disable caching -- this was commented out in legacy system code, left here for reference
        if (fUseCache && fAvailableCreditCached)
            return nAvailableCreditCached;
        */

        int64_t nCredit = 0;
        for (uint32_t i = 0; i < vout.size(); i++)
        {
            const TxOut &txout = vout[i];

            /* Calculate credit value only including unspent outputs */
            if (!IsSpent(i) && vout[i].nValue > 0)
            {
                /* Call corresponding method in wallet that will check which txout entries belong to it */
                nCredit += pWallet->GetCredit(txout);

                if (!Legacy::MoneyRange(nCredit))
                    throw std::runtime_error("WalletTx::GetAvailableCredit() : value out of range");
            }
        }

        /* Store cached value */
        nAvailableCreditCached = nCredit;

        /* Set cached flag */
        fAvailableCreditCached = true;

        return nCredit;
    }


    /* Retrieves any change amount for this transaction that belongs to the bound wallet. */
    int64_t WalletTx::GetChange() const
    {
        /* Return 0 if no wallet bound or outputs not loaded */
        if (!IsBound() || vout.empty())
            return 0;

        /* When calculation previously cached, use cached value */
        if (fChangeCached)
            return nChangeCached;

        /* Call corresponding method in wallet that will find the change transaction, if any */
        nChangeCached = pWallet->GetChange(*this);

        /* Set cached flag */
        fChangeCached = true;

        return nChangeCached;
    }


    /*  Retrieve the time when this transaction was received. */
    uint64_t WalletTx::GetTxTime() const
    {
        return nTimeReceived;
    }


    /*  Checks whether this transaction contains any inputs belonging
     *  to the bound wallet. */
    bool  WalletTx::IsFromMe() const
    {
        return (GetDebit() > 0);
    }


    /* Checks whether or not this transaction is confirmed. */
    bool WalletTx::IsConfirmed() const
    {
        /* Not final = not confirmed */
        if (!IsFinal())
            return false;

        /* If has depth, it is confirmed */
        if (GetDepthInMainChain() >= 1)
            return true;

        /* If transaction not from bound wallet (not ours) and depth is zero, it is unconfirmed */
        if (!IsFromMe())
            return false;

        /* If no confirmations but it is a transaction we sent (vtxPrev populated by AddSupportingTransactions()),
         * we can still consider it confirmed if all supporting transactions are confirmed.*/
        std::map<uint512_t, const MerkleTx*> mapPrev;
        for(const auto& prevTx : vtxPrev)
            mapPrev[prevTx.GetHash()] = &prevTx;

        /* Work queue to process all inputs recursively. */
        std::vector<const MerkleTx*> vWorkQueue;
        vWorkQueue.reserve(vtxPrev.size() + 1);

        /* Push back this tx to start with. */
        vWorkQueue.push_back(this);

        /* Loop through work queue. */
        for(uint32_t i = 0; i < vWorkQueue.size(); ++i)
        {
            /* Check for finality. */
            const MerkleTx* ptx = vWorkQueue[i];
            if(!ptx->IsFinal())
                return false;

            /* This code is only for edge case change transactions, so skip confiremd ones. */
            if(ptx->GetDepthInMainChain() >= 1)
                continue;

            /* This is for change transactions, so skip if previous isn't your change. */
            if(!pWallet->IsFromMe(*ptx))
                return false;

            /* Loop through inputs of transaction. */
            for(const auto& txin : ptx->vin)
            {
                /* Filter out inputs not included in dependant transactions. */
                if(!mapPrev.count(txin.prevout.hash))
                    return false;

                vWorkQueue.push_back(mapPrev[txin.prevout.hash]);
            }
        }

        return true;
    }


    /* Checks whether a particular output for this transaction is marked as spent */
    bool WalletTx::IsSpent(const uint32_t nOut) const
    {
        if(nOut >= vout.size())
            throw std::runtime_error("WalletTx::IsSpent() : nOut out of range");

        /* Any valid nOut value >= vfSpent.size() (not tracked) is considered unspent */
        if(nOut >= vfSpent.size())
            return false;

        return vfSpent[nOut];
    }


    /* Clears cache, assuring balances are recalculated. */
    void WalletTx::MarkDirty()
    {
        fCreditCached = false;
        fAvailableCreditCached = false;
        fDebitCached = false;
        fChangeCached = false;
    }


    /* Flags a given output for this transaction as spent */
    void WalletTx::MarkSpent(const uint32_t nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("WalletTx::MarkSpent() : nOut out of range");

        vfSpent.resize(vout.size());
        if (!vfSpent[nOut])
        {
            vfSpent[nOut] = true;

            /* Changing spent flags requires recalculating available credit */
            fAvailableCreditCached = false;
        }
    }


    /* Flags a given output for this transaction as unspent */
    void WalletTx::MarkUnspent(const uint32_t nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("WalletTx::MarkUnspent() : nOut out of range");

        vfSpent.resize(vout.size());
        if (vfSpent[nOut])
        {
            vfSpent[nOut] = false;

            /* Changing spent flags requires recalculating available credit */
            fAvailableCreditCached = false;
        }
    }


    /* Marks one or more outputs for this transaction as spent. */
    bool WalletTx::UpdateSpent(const std::vector<bool>& vfNewSpent)
    {
        bool fReturn = false;
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
    bool WalletTx::WriteToDisk()
    {
        LOCK(WalletTx::cs_wallettx);

        if (IsBound() && pWallet->IsFileBacked())
        {
            WalletDB walletDB(pWallet->GetWalletFile());
            bool ret = walletDB.WriteTx(GetHash(), *this);

            return ret;
        }

        return true;
    }


    /* Retrieve information about the current transaction. */
    void WalletTx::GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, std::list<std::pair<NexusAddress, int64_t> >& listReceived,
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
                nGeneratedImmature = pWallet->GetCredit(*this); //WalletTx::GetCredit() returns zero for immature
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
        for(const TxOut& txout : vout)
        {
            NexusAddress address;
            std::vector<uint8_t> vchPubKey;

            /* Get the Nexus address from the txout public key */
            if (!ExtractAddress(txout.scriptPubKey, address))
            {
                debug::log(0, FUNCTION, "Unknown transaction type found, txid ", this->GetHash().ToString());

                address = " unknown ";
            }

            /* For tx from us, txouts represent the sent value, add to the sent list */
            if (nDebit > 0)
                listSent.push_back(std::make_pair(address, txout.nValue));

            /* For txout received (sent to address in bound wallet), add to the received list */
            if (pWallet->IsMine(txout))
                listReceived.push_back(std::make_pair(address, txout.nValue));
        }
    }


    void WalletTx::GetAccountAmounts(const std::string& strAccount, int64_t& nGenerated, int64_t& nReceived,
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

        if (IsBound())
        {
            for(const auto& r : listReceived)
            {
                if (pWallet->GetAddressBook().HasAddress(r.first))
                {
                    /* When received Nexus Address (r.first) is in wallet address book,
                     * include it in nReceived amount if its label matches requested account label
                     */
                    if (pWallet->GetAddressBook().GetAddressBookName(r.first) == strAccount)
                        nReceived += r.second;
                }
                else if (strAccount == "" || strAccount == "*")
                {
                    /* Received address is not in wallet address book, included in nReceived when no account requested */
                    nReceived += r.second;
                }
            }
        }
    }


    /* Populates transaction data for previous transactions into vtxPrev */
    void WalletTx::AddSupportingTransactions()
    {
        /* pWallet->cs_wallet should already be locked before calling this method
         * Locking removed from within the method itself
         */
        vtxPrev.clear();

        const uint32_t COPY_DEPTH = 3;

        /* Calling this for new transaction will return main chain depth of 0 */
        if (IsBound() && GetDepthInMainChain() < COPY_DEPTH)
        {
            /* Create list of tx hashes for previous transactions referenced by this transaction's inputs */
            std::vector<uint512_t> vWorkQueue;
            for(const TxIn& txin : vin)
                vWorkQueue.push_back(txin.prevout.hash);

            { // Begin lock scope
                LOCK(WalletTx::cs_wallettx);

                /* Map keeps track of tx previously loaded, while set contains hash values already processed */
                std::map<uint512_t, const MerkleTx*> mapWalletPrev;
                std::set<uint512_t> setAlreadyDone;

                for (uint32_t i = 0; i < vWorkQueue.size(); i++) //Cannot use iterator because loop adds to vWorkQueue
                {
                    uint512_t prevoutTxHash = vWorkQueue[i];

                    /* Only need to process each hash once even if referenced multiple times */
                    if (setAlreadyDone.count(prevoutTxHash))
                        continue;

                    setAlreadyDone.insert(prevoutTxHash);

                    MerkleTx prevMerkleTx;
                    Legacy::Transaction parentTransaction;

                    /* Find returns iterator to equivalent of pair<uint512_t, WalletTx> */
                    auto mi = pWallet->mapWallet.find(prevoutTxHash);

                    if (mi != pWallet->mapWallet.end())
                    {
                        /* Found previous transaction (input to this one) in wallet */
                        const WalletTx& prevTx = (*mi).second; // Need WalletTx for access to vtxPrev
                        prevMerkleTx = prevTx;

                        /* Copy vtxPrev (inputs) from previous transaction into mapWalletPrev.
                         * This saves them so we can get MerkleTx from mapWalletPrev if it isn't in mapWallet
                         * and need to process deeper because tx depth is less than copy depth (unlikely, see below)
                         */
                        for(const MerkleTx& txWalletPrev : prevTx.vtxPrev)
                            mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;

                    }
                    else if (mapWalletPrev.count(prevoutTxHash))
                    {
                        /* Previous transaction not in wallet, but already in mapWalletPrev */
                        prevMerkleTx = *mapWalletPrev[prevoutTxHash];

                    }
                    else if (!config::fClient && LLD::legacyDB->ReadTx(prevoutTxHash, parentTransaction))
                    {
                        /* Found transaction in database, but it isn't in wallet. Create a new WalletTx from it to use as prevMerkleTx */
                        prevMerkleTx = WalletTx(pWallet, parentTransaction);
                    }
                    else
                    {
                        /* Transaction not found */
                        debug::log(0, FUNCTION, "Error: Unsupported transaction");
                        continue;
                    }

                    uint32_t nDepth = prevMerkleTx.GetDepthInMainChain();
                    vtxPrev.push_back(prevMerkleTx);

                    if (nDepth < COPY_DEPTH)
                    {
                        /* vtxPrev gets loaded with inputs to this transaction, but when one of these inputs
                         * is recent (depth < copy depth) we go one deeper and also load its inputs (inputs of inputs).
                         * This helps assure, when transactions are relayed, that we transmit anything not yet added
                         * to a block and included in legacyDB. Obviously, it is unikely that inputs of inputs are
                         * within the copy depth because we'd be spending balance that probably is not confirmed,
                         * so this really should never be processed. Code is from legacy and left here intact just in case.
                         */
                        for(const TxIn& txin : prevMerkleTx.vin)
                            vWorkQueue.push_back(txin.prevout.hash);
                    }
                }
            } // End lock scope
        }

        reverse(vtxPrev.begin(), vtxPrev.end());
    }


    /* Send this transaction to the network if not in our database, yet. */
    void WalletTx::RelayWalletTransaction() const
    {
        for(const MerkleTx& tx : vtxPrev)
        {
            /* Also relay any tx in vtxPrev that we don't have in our database, yet */
            if (!(tx.IsCoinBase() || tx.IsCoinStake()))
            {
                uint512_t hash = tx.GetHash();
                if (!LLD::legacyDB->HasTx(hash))
                {
                    std::vector<LLP::CInv> vInv = { LLP::CInv(hash, LLP::MSG_TX) };
                    LLP::LEGACY_SERVER->Relay("inv", vInv);

                    //Add to the memory pool
                    TAO::Ledger::mempool.Accept((Transaction)tx);
                }
            }
        }

        if (!(IsCoinBase() || IsCoinStake()))
        {
            uint512_t hash = GetHash();

            /* Relay this tx if we don't have it in our database, yet */
            if (!LLD::legacyDB->HasTx(hash))
            {
                debug::log(0, FUNCTION, "Relaying wtx ", hash.ToString().substr(0,10));

                std::vector<LLP::CInv> vInv = { LLP::CInv(hash, LLP::MSG_TX) };
                LLP::LEGACY_SERVER->Relay("inv", vInv);

                //Add to the memory pool
                TAO::Ledger::mempool.Accept((Transaction)*this);
            }
        }
    }

}
