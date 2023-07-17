/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /* The default constructor. */
    Transaction::Transaction()
    : TAO::Ledger::Transaction ( )
    , nModified                (nTimestamp)
    , nStatus                  (PENDING)
    , hashNextTx               ( )
    {
    }


    /* Copy constructor. */
    Transaction::Transaction(const Transaction& tx)
    : TAO::Ledger::Transaction (tx)
    , nModified                (tx.nModified)
    , nStatus                  (tx.nStatus)
    , hashNextTx               (tx.hashNextTx)
    {
    }


    /* Move constructor. */
    Transaction::Transaction(Transaction&& tx) noexcept
    : TAO::Ledger::Transaction (std::move(tx))
    , nModified                (std::move(tx.nModified))
    , nStatus                  (std::move(tx.nStatus))
    , hashNextTx               (std::move(tx.hashNextTx))
    {
    }


    /* Copy constructor. */
    Transaction::Transaction(const TAO::Ledger::Transaction& tx)
    : TAO::Ledger::Transaction (tx)
    , nModified                (nTimestamp)
    , nStatus                  (PENDING)
    , hashNextTx               (0)
    {
    }


    /* Move constructor. */
    Transaction::Transaction(TAO::Ledger::Transaction&& tx) noexcept
    : TAO::Ledger::Transaction (std::move(tx))
    , nModified                (nTimestamp)
    , nStatus                  (PENDING)
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
        hashCache     = tx.hashCache;

        nModified     = tx.nModified;
        nStatus       = tx.nStatus;
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
        hashCache     = std::move(tx.hashCache);

        nModified     = std::move(tx.nModified);
        nStatus       = std::move(tx.nStatus);
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
        hashCache     = tx.hashCache;

        //private values
        nModified     = nTimestamp;
        nStatus       = PENDING;
        hashNextTx    = 0;

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
        hashCache     = std::move(tx.hashCache);

        //private values
        nModified     = nTimestamp;
        nStatus       = PENDING;
        hashNextTx    = 0;

        return *this;
    }


    /* Default Destructor */
    Transaction::~Transaction()
    {
    }


    /* Set the transaction to a confirmed status. */
    bool Transaction::Confirmed() const
    {
        return (nStatus == ACCEPTED);
    }


    /* Get if transaction is in a matured status.*/
    bool Transaction::Mature(const uint512_t& hash) const
    {
        /* Check for coinbase/coinstake for maturity. */
        if(!IsCoinBase() && !IsCoinStake())
            return true; //non-producer transactions have no maturity requirement

        /* Read our confirmations from our ledger database. */
        uint32_t nConfirmations = 0;
        if(!LLD::Ledger->ReadConfirmations(hash, nConfirmations))
            return false;

        /* Switch for coinbase. */
        if(IsCoinBase())
            return (nConfirmations >= TAO::Ledger::MaturityCoinBase(TAO::Ledger::ChainState::tStateBest.load()));

        /* Switch for coinstake. */
        if(IsCoinStake())
            return (nConfirmations >= TAO::Ledger::MaturityCoinStake(TAO::Ledger::ChainState::tStateBest.load()));

        return true; //we shouldn't get here, but if we do return true
    }

    /* Check if a specific contract can be spent. */
    bool Transaction::Burned(const uint512_t& hash, const uint32_t nContract) const
    {
        /* Build a static contract to check with. */
        static TAO::Operation::Stream ssCheck;
        if(ssCheck.size() == 0)
        {
            ssCheck << uint8_t(TAO::Operation::OP::TYPES::UINT16_T) << uint16_t(4);
            ssCheck << uint8_t(TAO::Operation::OP::EQUALS);
            ssCheck << uint8_t(TAO::Operation::OP::TYPES::UINT16_T) << uint16_t(2);
        }

        /* Check our expected contract ranges. */
        if(nContract >= vContracts.size())
            return false; //we use this method to skip contracts so if out of range we need to know.

        /* Check for conditions. */
        const TAO::Operation::Stream& ssContract =
            vContracts[nContract].Conditions();

        /* Check for empty contracts. */
        if(ssContract.size() == 0)
            return false;

        /* Check our values byte for byte. */
        for(uint32_t nIndex = 0; nIndex < std::min(ssContract.size(), ssCheck.size()); nIndex++)
        {
            /* Check for out of range and return as spendable if so. */
            if(nIndex >= ssContract.size() || nIndex >= ssCheck.size())
                return false;

            /* Check for matching bytes. */
            if(ssContract.get(nIndex) != ssCheck.get(nIndex))
                return false;
        }

        return true;
    }


    /* Check if a specific contract has been spent already. */
    bool Transaction::Spent(const uint512_t& hash, const uint32_t nContract) const
    {
        /* Check our expected contract ranges. */
        if(nContract >= vContracts.size())
            return true; //we use this method to skip contracts so if out of range we need to know.

        /* Get a reference of our internal contract. */
        return vContracts[nContract].Spent(nContract);
    }


    /* Broadcast the transaction to all available nodes and update status. */
    void Transaction::Broadcast()
    {
        /* We don't need to re-broadcast confirmed transactions. */
        if(Confirmed())
            return;

        /* Check our re-broadcast time. */
        if(nModified + 60 < runtime::unifiedtimestamp())
        {
            /* Get a copy of our hash. */
            const uint512_t hashTx = GetHash();

            /* Adjust our modified timestamp. */
            nModified = runtime::unifiedtimestamp();

            /* Relay tx if creating ourselves. */
            if(LLP::TRITIUM_SERVER)
            {
                /* Relay the transaction notification. */
                LLP::TRITIUM_SERVER->Relay
                (
                    LLP::TritiumNode::ACTION::NOTIFY,
                    uint8_t(LLP::TritiumNode::TYPES::TRANSACTION),
                    hashTx
                );

                /* Log that tx was rebroadcast. */
                debug::log(1, FUNCTION, "Re-Broadcasted ", hashTx.SubString(), " to network");
            }

            /* Write our transaction update to disk. */
            if(!LLD::Logical->WriteTx(hashTx, *this))
                debug::error(FUNCTION, "failed to write ", VARIABLE(hashTx.SubString()));
        }
    }

    /* Index a transaction into the ledger database. */
    bool Transaction::Index(const uint512_t& hash)
    {
        /* Set our status to accepted if transaction has been connected to a block. */
        if(LLD::Ledger->HasIndex(hash))
        {
            /* Refresh this transaction from disk if it exists. */
            LLD::Logical->ReadTx(hash, *this); //only after it has been accepted to disk

            /* Set status to accepted. */
            nStatus = ACCEPTED;
        }

        /* Read our previous transaction. */
        if(!IsFirst())
        {
            /* Read our previous transaction to build indexes for it. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(hashPrevTx, tx))
                return debug::error(FUNCTION, "failed to read previous ", VARIABLE(hashPrevTx.SubString()));

            /* Set our forward iteration hash. */
            tx.hashNextTx = hash;

            /* Write our new transaction to disk. */
            if(!LLD::Logical->WriteTx(hashPrevTx, tx))
                return debug::error(FUNCTION, "failed to update previous ", VARIABLE(hashPrevTx.SubString()));
        }
        else
        {
            /* Write our first index if applicable. */
            if(!LLD::Logical->WriteFirst(hashGenesis, hash))
                return debug::error(FUNCTION, "failed to write first index for ", VARIABLE(hashGenesis.SubString()));
        }

        /* Push new transaction to database. */
        if(!LLD::Logical->WriteTx(hash, *this))
            return debug::error(FUNCTION, "failed to write ", VARIABLE(hash.SubString()));

        /* Write our last index to the database. */
        if(hashNextTx == 0 && !LLD::Logical->WriteLast(hashGenesis, hash))
            return debug::error(FUNCTION, "failed to write last index for ", VARIABLE(hashGenesis.SubString()));

        /* Index our transaction level data now. */
        if(nStatus == ACCEPTED)
            index_registers(hash);

        return true;
    }


    /* Delete this transaction from the logical database. */
    bool Transaction::Delete(const uint512_t& hash)
    {
        /* Read our previous transaction. */
        if(!IsFirst())
        {
            /* Read our previous transaction to build indexes for it. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(hashPrevTx, tx))
                return debug::error(FUNCTION, "failed to read previous ", VARIABLE(hashPrevTx.SubString()));

            /* Set our forward iteration hash. */
            tx.hashNextTx = 0;

            /* Erase our new transaction from disk. */
            if(!LLD::Logical->WriteTx(hashPrevTx, tx))
                return debug::error(FUNCTION, "failed to update previous ", VARIABLE(hashPrevTx.SubString()));

            /* Erase our last index from the database. */
            if(!LLD::Logical->WriteLast(hashGenesis, hashPrevTx))
                return debug::error(FUNCTION, "failed to write last index for ", VARIABLE(hashGenesis.SubString()));
        }
        else
        {
            /* Erase our first index if applicable. */
            if(!LLD::Logical->EraseFirst(hashGenesis))
                return debug::error(FUNCTION, "failed to write first index for ", VARIABLE(hashGenesis.SubString()));

            /* Erase our last index if applicable. */
            if(!LLD::Logical->EraseLast(hashGenesis))
                return debug::error(FUNCTION, "failed to write first index for ", VARIABLE(hashGenesis.SubString()));
        }

        /* Erase our transaction from the database. */
        if(!LLD::Logical->EraseTx(hash))
            return debug::error(FUNCTION, "failed to erase ", VARIABLE(hash.SubString()));

        /* Index our transaction level data now. */
        deindex_registers(hash);

        return true;
    }


    /* Check if transaction is last in sigchain. */
    bool Transaction::IsLast() const
    {
        return (hashNextTx != 0);
    }


    /* Index registers for logged in sessions. */
    void Transaction::index_registers(const uint512_t& hash)
    {
        /* Track unique register addresses. */
        std::set<uint256_t> setRegisters;

        /* Check all the tx contracts. */
        for(uint32_t nContract = 0; nContract < vContracts.size(); nContract++)
        {
            /* Grab reference of our contract. */
            const TAO::Operation::Contract& rContract = vContracts[nContract];

            /* Track our register address. */
            TAO::Register::Address hashRegister;
            if(!TAO::Register::Unpack(rContract, hashRegister))
                continue;

            /* Skip to our primitive. */
            rContract.SeekToPrimitive();

            /* Check the contract's primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;

            /* Switch based on our valid operations. */
            switch(nOP)
            {
                /* Transfer we need to mark this address as spent. */
                case TAO::Operation::OP::TRANSFER:
                {
                    /* Write a transfer index if transferring from our sigchain. */
                    if(!LLD::Logical->WriteTransfer(hashGenesis, hashRegister))
                        debug::warning(FUNCTION, "failed to write transfer for ", VARIABLE(hashRegister.SubString()));

                    break;
                }

                /* Create should add the register to the list. */
                case TAO::Operation::OP::CREATE:
                case TAO::Operation::OP::CLAIM:
                {
                    /* Write our register to database. */
                    if(nOP == TAO::Operation::OP::CREATE && !LLD::Logical->PushRegister(hashGenesis, hashRegister))
                        debug::warning(FUNCTION, "OP::CREATE: failed to push register ", VARIABLE(hashGenesis.SubString()));

                    /* Erase a transfer if claiming again after transferring. */
                    if(nOP == TAO::Operation::OP::CLAIM && !LLD::Logical->HasRegister(hashGenesis, hashRegister))
                    {
                        /* Erase our transfer index if claiming a register. */
                        if(!LLD::Logical->PushRegister(hashGenesis, hashRegister))
                            debug::warning(FUNCTION, "failed to erase transfer for ", VARIABLE(hashRegister.SubString()));
                    }

                    /* Erase a transfer if claiming again after transferring. */
                    if(nOP == TAO::Operation::OP::CLAIM && LLD::Logical->HasTransfer(hashGenesis, hashRegister))
                    {
                        /* Erase our transfer index if claiming a register. */
                        if(!LLD::Logical->EraseTransfer(hashGenesis, hashRegister))
                            debug::warning(FUNCTION, "failed to erase transfer for ", VARIABLE(hashRegister.SubString()));
                    }

                    /* Check for accounts we can discover for tokenization. */
                    if(hashRegister.IsAccount())
                    {
                        /* Handle some checks for tokenized assets. */
                        TAO::Register::Object oRegister;
                        if(LLD::Register->ReadObject(hashRegister, oRegister, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            /* Check our token type. */
                            const uint256_t hashToken =
                                oRegister.get<uint256_t>("token");

                            /* Check for our transfer event. */
                            TAO::Ledger::Transaction tx;
                            if(LLD::Ledger->ReadEvent(hashToken, 0, tx))
                            {
                                /* Loop through contracts to find the source event. */
                                for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
                                {
                                    /* Get a reference of the transaction contract. */
                                    const TAO::Operation::Contract& rEvent = tx[nContract];

                                    /* Reset the op stream */
                                    rEvent.SeekToPrimitive();

                                    /* The operation */
                                    uint8_t nOp;
                                    rEvent >> nOp;

                                    /* Skip over non transfers. */
                                    if(nOp != TAO::Operation::OP::TRANSFER)
                                        continue;

                                    /* The register address being transferred */
                                    uint256_t hashTransfer;
                                    rEvent >> hashTransfer;

                                    /* Get the new owner hash */
                                    uint256_t hashTokenized;
                                    rEvent >> hashTokenized;

                                    /* Check that the recipient of the transfer is the token */
                                    if(hashTokenized != hashToken)
                                        continue;

                                    /* Read the force transfer flag */
                                    uint8_t nType = 0;
                                    rEvent >> nType;

                                    /* Ensure this was a forced transfer (which tokenized asset transfers must be) */
                                    if(nType != TAO::Operation::TRANSFER::FORCE)
                                        continue;

                                    /* Push our tokenized account to our indexes. */
                                    if(!LLD::Logical->PushTokenized(hashGenesis, std::make_pair(hashRegister, hashTransfer)))
                                        debug::warning(FUNCTION, "failed to push tokenized ", VARIABLE(hashGenesis.SubString()));

                                    break; //we can exit now
                                }
                            }
                        }
                    }

                    break;
                }
            }

            /* Push transaction to the queue so we can track what modified given register. */
            if(!setRegisters.count(hashRegister) && LLD::Logical->PushTransaction(hashRegister, hash))
            {
                /* Track unique addresses to erase only once. */
                setRegisters.insert(hashRegister);
                continue;
            }
        }
    }


    /* Index registers for logged in sessions. */
    void Transaction::deindex_registers(const uint512_t& hash)
    {
        /* Track unique register addresses. */
        std::set<uint256_t> setRegisters;

        /* Check all the tx contracts. */
        for(int32_t nContract = vContracts.size() - 1; nContract >= 0; nContract--)
        {
            /* Grab reference of our contract. */
            const TAO::Operation::Contract& rContract = vContracts[nContract];

            /* Track our register address. */
            uint256_t hashRegister;
            if(!TAO::Register::Unpack(rContract, hashRegister))
                continue;

            /* Skip to our primitive. */
            rContract.SeekToPrimitive();

            /* Check the contract's primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;

            /* Switch based on relevant operations. */
            switch(nOP)
            {
                /* Transfer we need to mark this address as spent. */
                case TAO::Operation::OP::TRANSFER:
                {
                    /* Erase a transfer index if rolling back a transfer. */
                    if(!LLD::Logical->EraseTransfer(hashGenesis, hashRegister))
                        debug::warning(FUNCTION, "failed to write transfer for ", VARIABLE(hashRegister.SubString()));

                    break;
                }

                /* Claim should unmark address as spent. */
                case TAO::Operation::OP::CLAIM:
                {
                    /* Write a transfer index if rolling back a claim. */
                    if(!LLD::Logical->WriteTransfer(hashGenesis, hashRegister))
                        debug::warning(FUNCTION, "failed to erase transfer for ", VARIABLE(hashRegister.SubString()));

                    break;
                }

                case TAO::Operation::OP::CREATE:
                {
                    /* Write our register to database. */
                    if(!LLD::Logical->EraseRegister(hashGenesis, hashRegister))
                        debug::warning(FUNCTION, "failed to erase register ", VARIABLE(hashGenesis.SubString()));

                    break;
                }
            }

            /* Erase transaction from the queue so we can track what modified given register. */
            if(!setRegisters.count(hashRegister) && LLD::Logical->EraseTransaction(hashRegister))
                setRegisters.insert(hashRegister);
        }
    }
}
