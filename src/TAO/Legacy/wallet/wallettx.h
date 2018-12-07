/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_WALLET_WALLETTX_H
#define NEXUS_TAO_LEGACY_WALLET_WALLETTX_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include <TAO/Legacy/types/merkle.h>
#include <TAO/Legacy/wallet/wallet.h>

#include <Util/include/debug.h>
#include <Util/templates/serialize.h>


/* forward declaration */
namespace LLD {
    class CIndexDB;
}

namespace Legacy
{
    
    /* forward declarations */    
    class NexusAddress;
    class CTxIn;
    class CTxOut;

    /** @class CWalletTx
     *
     * A transaction with a bunch of additional info that only the owner cares about.
     * It includes any unrecorded transactions needed to link it back to the block chain.
     *
     *  Database key is tx<hash> where hash is the transaction hash
     **/
    class CWalletTx : public CMerkleTx
    {
    private:
        CWallet* ptransactionWallet;

        bool fHaveWallet;


        /** Init
         *
         *  Initializes an empty wallet transaction
         *
         **/
        void InitWalletTx()
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


    public:
        /** Previous transactions that contain outputs spent by inputs to this transaction **/
        std::vector<CMerkleTx> vtxPrev;


        /** Map used by RPC server to record certain argument for send commands.
         *  Also used by serialization to store vfSpent settings.
         **/
        std::map<std::string, std::string> mapValue;


        /** @deprecated This is not used. Kept for backward compatabililty of serialzation/deserialization **/
        std::vector<std::pair<std::string, std::string> > vOrderForm;


        /** Used by RPC server to record and report "fromaccount" for send operations. **/
        std::string strFromAccount;


        /** char vector with true/false values indicating spent outputs.
         *  Vector index matches output index value. Outputs with index
         *  value > vfSpent.size() are considered unspent (not marked as spent).
         *
         *  Not serialized directly. Values are copied into an entry in mapValue 
         *  using the "spent" key for serialization/deserialization.
         **/
        std::vector<char> vfSpent; 


        /** Flag indicating that the received time is also the transaction time.
         *  Should be set for transactions created and sent by bound wallet.
         **/
        uint64_t fTimeReceivedIsTxTime;


        /** Timestamp when this transaction was received by this node. **/
        uint64_t nTimeReceived; 


        /** Flag to indicate transaction is from bound wallet.
         *  This is set by transaction creation, but generally not used.
         *  The IsFromMe() method should be used instead. 
         **/
        char fFromMe;


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
        /* Have to initialize ptransactionWallet reference, but set fHaveWallet to false so it isn't used, need BindWallet first */
        CWalletTx() : ptransactionWallet(nullptr), fHaveWallet(false)
        {                
            InitWalletTx();
        }


        /** Constructor
         *
         *  Initializes an empty wallet transaction bound to a given wallet. 
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] walletIn The wallet for this wallet transaction
         *
         *  @see CWallet::AddToWallet()
         *
         **/
        CWalletTx(CWallet& walletIn) : ptransactionWallet(&walletIn), fHaveWallet(true)
        {
            InitWalletTx();
        }


        /** Constructor
         *
         *  Initializes an empty wallet transaction bound to a given wallet. 
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] pwalletIn Pointer to the wallet for this wallet transaction
         *
         *  @see CWallet::AddToWallet()
         *
         *  @deprecated Included for backward compatability
         *
         **/
        CWalletTx(CWallet* pwalletIn) : ptransactionWallet(pwalletIn), fHaveWallet(true)
        {
            InitWalletTx();
        }


        /** Constructor
         *
         *  Initializes a wallet transaction bound to a given wallet with data copied from a CMerkleTx. 
         *
         *  This does not add the transaction to the wallet.
         *
         *  @param[in] pwalletIn The wallet for this wallet transaction
         *
         *  @param[in] txIn Transaction data to copy into this wallet transaction
         *
         *  @see CWallet::AddToWallet()
         *
         **/
        CWalletTx(CWallet* pwalletIn, const CMerkleTx& txIn) : CMerkleTx(txIn), ptransactionWallet(pwalletIn), fHaveWallet(true)
        {
            InitWalletTx();
        }


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
         *  @see CWallet::AddToWallet()
         *
         **/
        CWalletTx(CWallet* pwalletIn, const Transaction& txIn) : CMerkleTx(txIn), ptransactionWallet(pwalletIn), fHaveWallet(true)
        {
            InitWalletTx();
        }


        /* Wallet transaction serialize/unserialize needs some customization to set up data*/
        IMPLEMENT_SERIALIZE
        (
            CWalletTx* pthis = const_cast<CWalletTx*>(this);
            if (fRead)
            {
                pthis->fHaveWallet = false;
                pthis->ptransactionWallet = nullptr;
                pthis->InitWalletTx();
            }
            char fSpent = false;

            if (!fRead)
            {
                pthis->mapValue["fromaccount"] = pthis->strFromAccount;

                std::string str;
                for(char f : vfSpent)
                {
                    str += (f ? '1' : '0');
                    if (f)
                        fSpent = true;
                }
                pthis->mapValue["spent"] = str;
            }

            nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nSerType, nVersion, ser_action);
            READWRITE(vtxPrev);
            READWRITE(mapValue);
            READWRITE(vOrderForm);
            READWRITE(fTimeReceivedIsTxTime);
            READWRITE(nTimeReceived);
            READWRITE(fFromMe);
            READWRITE(fSpent);

            if (fRead)
            {
                pthis->strFromAccount = pthis->mapValue["fromaccount"];

                if (mapValue.count("spent"))
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
         *  @param[in] pwalletIn Pointer to the wallet to be bound
         *
         *  @see CWallet::AddToWallet()
         *
         **/
        void BindWallet(CWallet *pwalletIn);


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
        inline int64_t GetTxTime() const { return nTimeReceived; }



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
        int GetRequestCount() const;


        /** IsFromMe
         *
         *  Checks whether this transaction contains any inputs belonging to the bound wallet. 
         * 
         *  @return true if the bound wallet sends balance via this transaction 
         *
         **/
        inline bool IsFromMe() const { return (GetDebit() > 0); }


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
        bool UpdateSpent(const std::vector<char>& vfNewSpent);


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
         *  @param[in] strSentAccount The Nexus address to get amounts for
         *
         *  @param[out] nGenerated When strSentAccount empty, returns any mature Coinbase/Coinstake generated
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
         *  @param[in] indexdb Local index database containing previous transaction data
         *
         **/
        void AddSupportingTransactions(LLD::CIndexDB& indexdb);


        /** RelayWalletTransaction
         *
         *  Send this transaction to the network if not in our database, yet.
         *
         *  @param[in] indexdb Local index database to check
         *
         **/
        void RelayWalletTransaction(LLD::CIndexDB& indexdb);


        /** RelayWalletTransaction
         *
         *  Send this transaction to the network if not in our database, yet.
         *
         *  This method open the lobal index database to check it.
         *
         **/
        void RelayWalletTransaction();

    };

}

#endif
