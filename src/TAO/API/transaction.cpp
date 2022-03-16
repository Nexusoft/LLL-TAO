/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    
    /* The default constructor. */
    Transaction::Transaction()
    : TAO::Ledger::Transaction ( )
    , hashNextTx               ( )
    {
    }


    /* Copy constructor. */
    Transaction::Transaction(const Transaction& tx)
    : TAO::Ledger::Transaction (tx)
    , hashNextTx               (tx.hashNextTx)
    {
    }


    /* Move constructor. */
    Transaction::Transaction(Transaction&& tx) noexcept
    : TAO::Ledger::Transaction (std::move(tx))
    , hashNextTx               (std::move(tx.hashNextTx))
    {
    }


    /* Copy constructor. */
    Transaction::Transaction(const TAO::Ledger::Transaction& tx)
    : TAO::Ledger::Transaction (tx)
    , hashNextTx               (0)
    {
    }


    /* Move constructor. */
    Transaction::Transaction(TAO::Ledger::Transaction&& tx) noexcept
    : TAO::Ledger::Transaction (std::move(tx))
    , hashNextTx               (0)
    {
    }


    /* Copy assignment. */
    Transaction& Transaction::operator=(const Transaction& tx)
    {
        vContracts    = tx.vContracts;
        nVersion      = tx.nVersion;
        nSequence     = tx.nSequence;
        nTimestamp    = tx.nTimestamp;
        hashNext      = tx.hashNext;
        hashRecovery  = tx.hashRecovery;
        hashGenesis   = tx.hashGenesis;
        hashPrevTx    = tx.hashPrevTx;
        nKeyType      = tx.nKeyType;
        nNextType     = tx.nNextType;
        vchPubKey     = tx.vchPubKey;
        vchSig        = tx.vchSig;

        hashNextTx    = tx.hashNextTx;

        return *this;
    }


    /* Move assignment. */
    Transaction& Transaction::operator=(Transaction&& tx) noexcept
    {
        vContracts    = std::move(tx.vContracts);
        nVersion      = std::move(tx.nVersion);
        nSequence     = std::move(tx.nSequence);
        nTimestamp    = std::move(tx.nTimestamp);
        hashNext      = std::move(tx.hashNext);
        hashRecovery  = std::move(tx.hashRecovery);
        hashGenesis   = std::move(tx.hashGenesis);
        hashPrevTx    = std::move(tx.hashPrevTx);
        nKeyType      = std::move(tx.nKeyType);
        nNextType     = std::move(tx.nNextType);
        vchPubKey     = std::move(tx.vchPubKey);
        vchSig        = std::move(tx.vchSig);

        hashNextTx    = std::move(tx.hashNextTx);

        return *this;
    }


    /* Copy assignment. */
    Transaction& Transaction::operator=(const TAO::Ledger::Transaction& tx)
    {
        vContracts    = tx.vContracts;
        nVersion      = tx.nVersion;
        nSequence     = tx.nSequence;
        nTimestamp    = tx.nTimestamp;
        hashNext      = tx.hashNext;
        hashRecovery  = tx.hashRecovery;
        hashGenesis   = tx.hashGenesis;
        hashPrevTx    = tx.hashPrevTx;
        nKeyType      = tx.nKeyType;
        nNextType     = tx.nNextType;
        vchPubKey     = tx.vchPubKey;
        vchSig        = tx.vchSig;

        return *this;
    }


    /* Move assignment. */
    Transaction& Transaction::operator=(TAO::Ledger::Transaction&& tx) noexcept
    {
        vContracts    = std::move(tx.vContracts);
        nVersion      = std::move(tx.nVersion);
        nSequence     = std::move(tx.nSequence);
        nTimestamp    = std::move(tx.nTimestamp);
        hashNext      = std::move(tx.hashNext);
        hashRecovery  = std::move(tx.hashRecovery);
        hashGenesis   = std::move(tx.hashGenesis);
        hashPrevTx    = std::move(tx.hashPrevTx);
        nKeyType      = std::move(tx.nKeyType);
        nNextType     = std::move(tx.nNextType);
        vchPubKey     = std::move(tx.vchPubKey);
        vchSig        = std::move(tx.vchSig);

        return *this;
    }


    /* Default Destructor */
    Transaction::~Transaction()
    {
    }
}
