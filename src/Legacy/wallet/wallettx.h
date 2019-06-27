/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_WALLETTX_H
#define NEXUS_LEGACY_WALLET_WALLETTX_H

#include <Legacy/types/merkle.h>
#include <Legacy/wallet/wallet.h>

#include <Util/include/debug.h>
#include <Util/templates/serialize.h>

#include <list>
#include <map>
#include <string>
#include <vector>


/* forward declaration */
namespace LLD
{
    class LegacyDB;
}

namespace Legacy
{

    /* forward declarations */
    class NexusAddress;
    class TxIn;
    class TxOut;

    /** @class WalletTx
     *
     * A transaction with additional information relevant to the owner and the owner's wallet which owns it.
     * It includes any unrecorded previoius transactions needed to link it back to the block chain.
     *
     *  Database key is tx<hash> where hash is the transaction hash
     **/
    class WalletTx : public MerkleTx
    {
    private:
        /* This has to be static or copy constructor/copy assignment are deleted.
         * As we normally process transactions iteratively (not simultaneously),
         * having it static should have little if any effect.
         *
         * Should this prove a problem later, then it can be changed to mutable
         * and copy operations defined
         */
        /** Mutex for thread concurrency across transaction operations. **/
        static std::mutex cs_wallettx;

        /** Pointer to the wallet to which this transaction is bound **/
        Wallet* ptransactionWallet;

        /** Flag indicating whether or not transaction bound to wallet **/
        bool fHaveWallet;


        /** Init
         *
         *  Initializes an empty wallet transaction
         *
         **/
        void InitWalletTx();


    public:
        /** Previous transactions that contain outputs spent by inputs to this transaction **/
        std::vector<MerkleTx> vtxPrev;


        /** Used by serialization to store/retrieve vfSpent and other settings.
         **/
        std::map<std::string, std::string> mapValue;


        /** @deprecated This is not used. Kept for backward compatabililty of serialzation/deserialization **/
        std::vector<std::pair<std::string, std::string> > vOrderForm;


        /** The sending account label for this tranasction **/
        std::string strFromAccount;


        /** char vector with true/false values indicating spent outputs.
         *  Vector index matches output index value. Outputs with index
         *  value > vfSpent.size() are considered unspent (not marked as spent).
         *
         *  Not serialized directly. Values are copied into an entry in mapValue
         *  using the "spent" key for serialization/deserialization.
         **/
        std::vector<bool> vfSpent;


        /** Flag indicating that the received time is also the transaction time.
         *  Should be set for transactions created and sent by bound wallet.
         *
         *  This is actually a bool value, set to true or false. Type
         *  declaration is uint32_t to support unserialization from legacy wallets.
         **/
        uint32_t fTimeReceivedIsTxTime;


        /** Timestamp when this transaction was received by this node.
         *
         *  Timestamps are generally uint64_t but this one must remain uint32_t
         *  to support unserialization of unsigned int values in legacy wallets.
         *
         **/
        uint32_t nTimeReceived;


        /** Flag to indicate transaction is from bound wallet.
         *  This is set by transaction creation, but generally not used.
         *  The IsFromMe() method should be used instead.
         **/
        bool fFromMe;


        /* Memory only data (not serialized) */

        /** Use nDebitCached value when true, recalculate when false. **/
        mutable bool fDebitCached;

        /** Use nCreditCached value when true, recalculate when false. **/
        mutable bool fCreditCached;

        /** Use nAvailableCreditCached value when true, recalculate when false. **/
        mutable bool fAvailableCreditCached;

        /** Use nChangeCached value when true, recalculate when false. **/
        mutable bool fChangeCached;

        /** Use nDebitCached value when true, recalculate when false. **/
        mutable int64_t nDebitCached;

        mutable int64_t nCreditCached;

        mutable int64_t nAvailableCreditCached;

        mutable int64_t nChangeCached;


        /** Constructor
         *
         *  Initializes an empty wallet transaction
         *
         **/
        WalletTx();


        /** Constructor
         *
         *  Initializes an empty wallet transaction bound to a given wallet.
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] walletIn The wallet for this wallet transaction
         *
         *  @see Wallet::AddToWallet()
         *
         **/
        WalletTx(Wallet& walletIn);


        /** Constructor
         *
         *  Initializes an empty wallet transaction bound to a given wallet.
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] pwalletIn Pointer to the wallet for this wallet transaction
         *
         *  @see Wallet::AddToWallet()
         *
         **/
        WalletTx(Wallet* pwalletIn);


        /** Constructor
         *
         *  Initializes a wallet transaction bound to a given wallet with data copied from a MerkleTx.
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] pwalletIn The wallet for this wallet transaction
         *
         *  @param[in] txIn Transaction data to copy into this wallet transaction
         *
         *  @see Wallet::AddToWallet()
         *
         **/
        WalletTx(Wallet* pwalletIn, const MerkleTx& txIn);


        /** Constructor
         *
         *  Initializes a wallet transaction bound to a given wallet with data copied from a Transaction.
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] pwalletIn The wallet for this wallet transaction
         *
         *  @param[in] txIn Transaction data to copy into this wallet transaction
         *
         *  @see Wallet::AddToWallet()
         *
         **/
        WalletTx(Wallet* pwalletIn, const Transaction& txIn);


        /** Destructor **/
        virtual ~WalletTx();


        /* Wallet transaction serialize/unserialize needs some customization to set up data*/
        IMPLEMENT_SERIALIZE
        (
            WalletTx* pthis = const_cast<WalletTx*>(this);
            if(fRead)
            {
                pthis->fHaveWallet = false;
                pthis->ptransactionWallet = nullptr;
                pthis->InitWalletTx();
            }
            bool fSpent = false;

            if(!fRead)
            {
                pthis->mapValue["fromaccount"] = pthis->strFromAccount;

                std::string str;
                for(bool f : vfSpent)
                {
                    str += (f ? '1' : '0');
                    if(f)
                        fSpent = true;
                }
                pthis->mapValue["spent"] = str;
            }

            nSerSize += SerReadWrite(s, *(MerkleTx *)this, nSerType, nSerVersion, ser_action);

            READWRITE(vtxPrev);
            READWRITE(mapValue);
            READWRITE(vOrderForm);
            READWRITE(fTimeReceivedIsTxTime);
            READWRITE(nTimeReceived);
            READWRITE(fFromMe);
            READWRITE(fSpent);

            if(fRead)
            {
                pthis->strFromAccount = pthis->mapValue["fromaccount"];

                if(mapValue.count("spent"))
                    for(char c : pthis->mapValue["spent"])
                        pthis->vfSpent.push_back(c != '0');
                else
                    pthis->vfSpent.assign(vout.size(), fSpent);
            }

            pthis->mapValue.erase("fromaccount");
            pthis->mapValue.erase("version");
            pthis->mapValue.erase("spent");
      )


        /** BindWallet
         *
         *  Assigns the wallet for this wallet transaction.
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] walletIn The wallet to be bound
         *
         *  @see Wallet::AddToWallet()
         *
         **/
        void BindWallet(Wallet& walletIn);


        /** BindWallet
         *
         *  Assigns the wallet for this wallet transaction.
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] pwalletIn Pointer to the wallet to be bound
         *
         *  @see Wallet::AddToWallet()
         *
         **/
        void BindWallet(Wallet* pwalletIn);


        /** IsBound
         *
         *  Test whether transaction is bound to a wallet.
         *
         *  @return true if this transaction is bound to a wallet, false otherwise
         *
         **/
        bool IsBound() const;


        /** GetDebit
         *
         *  Calculates the total value for all inputs sent by this transaction that belong to the bound wallet.
         *
         *  @return total transaction debit amount, 0 if no wallet bound
         *
         **/
        int64_t GetDebit() const;


        /** GetCredit
         *
         *  Calculates the total value for all outputs received by this transaction that belong to the bound wallet.
         *
         *  If this is a coinbase or coinstake transaction, credit is zero until it reaches maturity.
         *
         *  @return total transaction credit amount, 0 if no wallet bound
         *
         **/
        int64_t GetCredit() const;


        /** GetAvailableCredit
         *
         *  Calculates the total value for all unspent outputs received by this transaction that belong to the bound wallet.
         *
         *  If this is a coinbase or coinstake transaction, credit is zero until it reaches maturity.
         *
         *  @return total unspent transaction credit amount, 0 if no wallet bound
         *
         **/
        int64_t GetAvailableCredit() const;


        /** GetChange
         *
         *  Retrieves any change amount for this transaction that belongs to the bound wallet.
         *
         *  @return change transaction amount, 0 if no wallet bound
         *
         **/
        int64_t GetChange() const;


        /** GetTxTime
         *
         *  Retrieve the time when this transaction was received.
         *  If fTimeReceivedIsTxTime is set (non-zero), this is also
         *  the time when the transaction was created (transaction
         *  created and sent from bound wallet).
         *
         *  @return The transaction timestamp
         *
         **/
        uint64_t GetTxTime() const;



        /** GetRequestCount
         *
         *  Get the number of remote requests recorded for this transaction.
         *
         *  Coinbase and Coinstake transactions are tracked at the block level,
         *  so count records requests for the block containing them.
         *
         *  @return The request count as recorded by request tracking, -1 if not tracked, 0 if no wallet bound
         *
         **/
        int32_t GetRequestCount() const;


        /** IsFromMe
         *
         *  Checks whether this transaction contains any inputs belonging to the bound wallet.
         *
         *  @return true if the bound wallet sends balance via this transaction
         *
         **/
        bool IsFromMe() const;


        /** IsConfirmed
         *
         *  Checks whether or not this transaction is confirmed.
         *
         *  @return true if this transaction is confirmed
         *
         **/
        bool IsConfirmed() const;


        /** IsSpent
         *
         *  Checks whether a particular output for this transaction is marked as spent
         *
         *  @param[in] nOut The index value of the output to check
         *
         *  @return true if this transaction output is marked as spent
         *
         **/
        bool IsSpent(const uint32_t nOut) const;


        /** MarkDirty
         *
         *  Clears cache, assuring balances are recalculated.
         *
         **/
        void MarkDirty();


        /** MarkSpent
         *
         *  Flags a given output for this transaction as spent
         *
         *  @param[in] nOut The index value of the output to mark
         *
         **/
        void MarkSpent(const uint32_t nOut);


        /** MarkUnspent
         *
         *  Flags a given output for this transaction as unspent
         *
         *  @param[in] nOut The index value of the output to mark
         *
         **/
        void MarkUnspent(const uint32_t nOut);


        /** UpdateSpent
         *
         *  Marks one or more outputs for this transaction as spent.
         *
         *  Any entries in the input vector evaluating true will
         *  mark the output with the corresponding index as spent.
         *  Entries not marked as true are ignored.
         *
         *  @param[in] vfNewSpent Vector of spent flags to load
         *
         *  @return true if spent status of any output is updated
         *
         **/
        bool UpdateSpent(const std::vector<bool>& vfNewSpent);


        /** WriteToDisk
         *
         *  Store this transaction in the database for the bound wallet
         *  when wallet is file backed.
         *
         *  @param[in] nOut The index value of the output to mark
         *
         **/
        bool WriteToDisk();


        /** GetAmounts
         *
         *  Retrieve information about the current transaction.
         *
         *  Coinbase/Coinstake transactions return the two nGeneratedxxx values and all else is empty.
         *
         *  Other transactions return 0 for the two nGeneratedxxx and populate the remaining values
         *
         *  @param[out] nGeneratedImmature Credit amount if current transaction is immature Coinbase/Coinstake, 0 otherwise
         *
         *  @param[out] nGeneratedMature Credit amount if current transaction is mature Coinbase/Coinstake, 0 otherwise
         *
         *  @param[out] listReceived List of amounts received by this transaction, as to-address/amount pairs.
         *
         *  @param[out] listSent List of amounts sent by this transaction, as from-address/amount pairs.
         *
         *  @param[out] nFee The amount of fee paid if transaction sent from the bound wallet, 0 otherwise
         *
         *  @param[out] strSentAccount The sent from account assigned to this transaction, if any
         *
         **/
        void GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, std::list<std::pair<NexusAddress, int64_t> >& listReceived,
                        std::list<std::pair<NexusAddress, int64_t> >& listSent, int64_t& nFee, std::string& strSentAccount) const;


        /** GetAmounts
         *
         *  Retrieve information about the current transaction for a specified account Nexus address.
         *
         *  @param[in] strAccount The account label to get amounts for
         *
         *  @param[out] nGenerated When strAccount empty, returns any mature Coinbase/Coinstake generated
         *
         *  @param[out] nReceived Received amount for requested account.
         *                        If account empty, totals amounts for accounts not in wallet's address book
         *
         *  @param[out] nSent If transaction from requested account, the total amount sent
         *
         *  @param[out] nFee Fee amount for the amount sent
         *
         **/
        void GetAccountAmounts(const std::string& strAccount, int64_t& nGenerated, int64_t& nReceived,
                               int64_t& nSent, int64_t& nFee) const;


        /** AddSupportingTransactions
         *
         *  Populates transaction data for previous transactions into vtxPrev.
         *  Adds no transaction of no wallet bound.
         *
         **/
        void AddSupportingTransactions();


        /** RelayWalletTransaction
         *
         *  Send this transaction to the network if not in our database, yet.
         *
         **/
        void RelayWalletTransaction() const;

    };

}

#endif
