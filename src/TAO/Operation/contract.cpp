/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/transaction.h>
#include <Legacy/types/txout.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/state.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/timelocks.h>

/* Global TAO namespace. */
namespace TAO::Operation
{
    /* Default Constructor. */
    Contract::Contract()
    : ssOperation ( )
    , ssCondition ( )
    , ssRegister  ( )
    , hashCaller  (0)
    , nTimestamp  (0)
    , hashTx      (0)
    , nVersion    (TAO::Ledger::CurrentTransactionVersion())
    {
    }


    /* Copy Constructor. */
    Contract::Contract(const Contract& contract)
    : ssOperation (contract.ssOperation)
    , ssCondition (contract.ssCondition)
    , ssRegister  (contract.ssRegister)
    , hashCaller  (contract.hashCaller)
    , nTimestamp  (contract.nTimestamp)
    , hashTx      (contract.hashTx)
    , nVersion    (contract.nVersion)
    {
    }


    /* Move Constructor. */
    Contract::Contract(Contract&& contract) noexcept
    : ssOperation (std::move(contract.ssOperation))
    , ssCondition (std::move(contract.ssCondition))
    , ssRegister  (std::move(contract.ssRegister))
    , hashCaller  (std::move(contract.hashCaller))
    , nTimestamp  (std::move(contract.nTimestamp))
    , hashTx      (std::move(contract.hashTx))
    , nVersion    (std::move(contract.nVersion))
    {
    }


    /* Copy Assignment Operator */
    Contract& Contract::operator=(const Contract& contract)
    {
        ssOperation = contract.ssOperation;
        ssCondition = contract.ssCondition;
        ssRegister  = contract.ssRegister;
        hashCaller  = contract.hashCaller;
        nTimestamp  = contract.nTimestamp;
        hashTx      = contract.hashTx;
        nVersion    = contract.nVersion;

        return *this;
    }


    /* Move Assignment Operator */
    Contract& Contract::operator=(Contract&& contract) noexcept
    {
        ssOperation = std::move(contract.ssOperation);
        ssCondition = std::move(contract.ssCondition);
        ssRegister  = std::move(contract.ssRegister);
        hashCaller  = std::move(contract.hashCaller);
        nTimestamp  = std::move(contract.nTimestamp);
        hashTx      = std::move(contract.hashTx);
        nVersion    = std::move(contract.nVersion);

        return *this;
    }


    /* Destructor. */
    Contract::~Contract()
    {
    }


    /* Construct based on legacy transaction. */
    Contract::Contract(const Legacy::Transaction& tx, const uint32_t& nContract)
    {
        /* Check boundaries. */
        if(nContract >= tx.vout.size())
            throw debug::exception(FUNCTION, "contract output out of bounds");

        /* Grab a reference of our output. */
        const Legacy::TxOut& txout = tx.vout[nContract];

        /* Check script size. */
        if(txout.scriptPubKey.size() != 34)
            throw debug::exception(FUNCTION, "invalid script size ", txout.scriptPubKey.size());

        /* Get the script output. */
        uint256_t hashAccount;
        std::copy((uint8_t*)&txout.scriptPubKey[1], (uint8_t*)&txout.scriptPubKey[1] + 32, (uint8_t*)&hashAccount);

        /* Check for OP::RETURN. */
        if(txout.scriptPubKey[33] != Legacy::OP_RETURN)
            throw debug::exception(FUNCTION, "last OP has to be OP_RETURN");

        /* Create Contract. */
        ssOperation << uint8_t(TAO::Operation::OP::DEBIT)    << TAO::Register::WILDCARD_ADDRESS;
        ssOperation << hashAccount << uint64_t(txout.nValue) << uint64_t(0);

        /* Populate our contract level data. */
        hashCaller = 0; //superfluous, but being explicit
        nTimestamp = tx.nTime;
        hashTx     = tx.GetHash();
        nVersion   = tx.nVersion;
    }


    /* Bind the contract to a transaction. */
    void Contract::Bind(const TAO::Ledger::Transaction* tx, const bool fGetHash) const
    {
        /* Check for nullptr bind. */
        if(tx == nullptr)
            throw debug::exception(FUNCTION, "cannot bind to a nullptr");

        /* Don't bind a again if already bound as calling GetHash is expensive */
        hashCaller = tx->hashGenesis;
        nTimestamp = tx->nTimestamp;
        nVersion   = tx->nVersion;

        /* Don't calculate the txid if it's not specified. */
        if(fGetHash)
            hashTx     = tx->GetHash();
    }


    /* Bind the contract to a transaction with txid passed as param. */
    void Contract::Bind(const TAO::Ledger::Transaction* tx, const uint512_t& hash) const
    {
        /* Check for nullptr bind. */
        if(tx == nullptr)
            throw debug::exception(FUNCTION, "cannot bind to a nullptr");

        /* Don't bind a again if already bound as calling GetHash is expensive */
        hashCaller = tx->hashGenesis;
        nTimestamp = tx->nTimestamp;
        hashTx     = hash;
        nVersion   = tx->nVersion;
    }


    /* Bind the contract to a transaction with timestamp and caller passed as param. */
    void Contract::Bind(const uint64_t nTimestampIn, const uint256_t& hashCallerIn) const
    {
        /* Set the variables. */
        hashCaller = hashCallerIn;
        nTimestamp = nTimestampIn;

        /* Set the transaction version based on the timestamp. */
        const uint32_t nCurrent = TAO::Ledger::CurrentTransactionVersion();
        if(TAO::Ledger::TransactionVersionActive(nTimestamp, nCurrent))
            nVersion = nCurrent;
        else
            nVersion = nCurrent - 1;
    }


    /* Get the primitive operation. */
    uint8_t Contract::Primitive() const
    {
        /* Sanity checks. */
        if(ssOperation.size() == 0)
            throw debug::exception(FUNCTION, "cannot get primitive when empty");

        /* Get the operation code.*/
        const uint8_t nOP = ssOperation.get(0);

        /* Switch for validate or condition. */
        switch(nOP)
        {
            /* Check for condition. */
            case OP::CONDITION:
            {
                /* Get next op. */
                return ssOperation.get(1);
            }

            /* Check for validate. */
            case OP::VALIDATE:
            {
                /* Skip over on validate. */
                return ssOperation.get(69);
            }
        }

        /* Return first byte. */
        return nOP;
    }


    /* Get this contract's execution time. */
    const uint64_t& Contract::Timestamp() const
    {
        return nTimestamp;
    }


    /* Get this contract's caller */
    const uint256_t& Contract::Caller() const
    {
        return hashCaller;
    }


    /* Get the hash of calling tx */
    const uint512_t& Contract::Hash() const
    {
        return hashTx;
    }


    /* Get the version of calling tx */
    const uint32_t& Contract::Version() const
    {
        return nVersion;
    }


    /* Check if a given contract has been spent yet */
    bool Contract::Spent(const uint32_t nContract) const
    {
        /* Reset the contract to the position of the primitive. */
        SeekToPrimitive();

        /* The operation */
        uint8_t nOP;
        ssOperation >> nOP;

        /* Check proofs based on spend type. */
        switch(nOP)
        {
            /* Handle if checking for basic primitives. */
            case TAO::Operation::OP::COINBASE:
            case TAO::Operation::OP::TRANSFER:
            case TAO::Operation::OP::DEBIT:
            {
                /* Get our proof to check. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Check for forced transfers. */
                if(nOP == TAO::Operation::OP::TRANSFER)
                {
                    /* Seek over recipient. */
                    ssOperation.seek(32);

                    /* Read the force transfer flag */
                    uint8_t nType = 0;
                    ssOperation >> nType;

                    /* Forced transfers don't require a proof. */
                    if(nType == TAO::Operation::TRANSFER::FORCE)
                        return true;
                }

                /* Check for a valid proof. */
                return LLD::Ledger->HasProof(hashRegister, hashTx, nContract, TAO::Ledger::FLAGS::LOOKUP);
            }
        }

        /* Otherwise check for validated contract. */
        return LLD::Contract->HasContract(std::make_pair(hashTx, nContract), TAO::Ledger::FLAGS::LOOKUP);
    }


    /* Get the value of the contract if valid */
    bool Contract::Value(uint64_t &nValue) const
    {
        /* Reset the contract to the position of the primitive. */
        SeekToPrimitive();

        /* Set value. */
        nValue = 0;

        /* Get the operation code.*/
        uint8_t nOP = 0;
        ssOperation >> nOP;

        /* Switch for validate or condition. */
        switch(nOP)
        {
            /* Check for debit. */
            case OP::DEBIT:
            {
                /* Skip over from and to. */
                ssOperation.seek(64);

                /* Get value. */
                ssOperation >> nValue;

                break;
            }

            /* Check for credit. */
            case OP::CREDIT:
            {
                /* Skip over unrelated data. */
                ssOperation.seek(132);

                /* Get value. */
                ssOperation >> nValue;

                break;
            }

            /* Check for coinbase. */
            case OP::COINBASE:
            {
                /* Skip over genesis. */
                ssOperation.seek(32);

                /* Get coinbase reward. */
                ssOperation >> nValue;

                break;
            }

            /* Check for Trust Coinstake. */
            case OP::TRUST:
            {
                /* Skip over hashLast, nScore, nStakeChange. */
                ssOperation.seek(80);

                /* Get stake reward for trust. */
                ssOperation >> nValue;

                break;
            }

            /* Check for Genesis Coinstake. */
            case OP::GENESIS:
            {
                /* Get stake reward for genesis. */
                ssOperation >> nValue;

                break;
            }

            /* Check for fee. */
            case OP::FEE:
            {
                /* Skip over fee account . */
                ssOperation.seek(32);

                /* Get value. */
                ssOperation >> nValue;

                break;
            }
        }

        return (nValue > 0);
    }


    /* Get the previous tx hash if valid for contract */
    bool Contract::Previous(uint512_t &hashPrev) const
    {
        /* Reset the contract. */
        ssOperation.seek(0, STREAM::BEGIN);

        /* Initialize the has. */
        hashPrev = 0;

        /* Get the operation code.*/
        uint8_t nOP = 0;
        ssOperation >> nOP;

        /* Switch for validate or condition. */
        switch(nOP)
        {
            /* Check for condition. */
            case OP::CONDITION:
            {
                /* Get next op. */
                ssOperation >> nOP;

                break;
            }

            /* Check for validate. */
            case OP::VALIDATE:
            {
                /* Get hash tx for condition contract */
                ssOperation >> hashPrev;

                return (hashPrev > 0);
            }
        }

        /* Switch for validate or condition. */
        switch(nOP)
        {
            /* Check for claim. */
            case OP::CLAIM:
            {
                /* Get hash tx of previous transfer. */
                ssOperation >> hashPrev;

                break;
            }

            /* Check for credit. */
            case OP::CREDIT:
            {
                /* Get hash tx of previous debit. */
                ssOperation >> hashPrev;

                break;
            }

            /* Check for trust coinstake. */
            case OP::TRUST:
            {
                /* Get last stake hash */
                ssOperation >> hashPrev;

                break;
            }
        }

        return (hashPrev > 0);
    }


    /* Get the previous tx hash if valid for contract */
    bool Contract::Dependant(uint512_t &hashPrev, uint32_t &nContract) const
    {
        /* Reset values. */
        hashPrev  = 0;
        nContract = 0;

        /* Reset the contract to the position of the primitive. */
        SeekToPrimitive();

        /* Get the operation code.*/
        uint8_t nOP = 0;
        ssOperation >> nOP;

        /* Switch for validate or condition. */
        switch(nOP)
        {
            /* Check for claim. */
            case OP::CLAIM:
            {
                /* Get dependants from binary stream. */
                ssOperation >> hashPrev;
                ssOperation >> nContract;

                return true;
            }

            /* Check for credit. */
            case OP::CREDIT:
            {
                /* Get dependants from binary stream. */
                ssOperation >> hashPrev;
                ssOperation >> nContract;

                return true;
            }
        }

        return false;
    }


    /* Get the legacy converted output of the contract if valid */
    bool Contract::Legacy(Legacy::TxOut &txout) const
    {
        /* Check for LEGACY. */
        if(Primitive() != OP::LEGACY)
            return false;

        /* Skip over address. */
        ssOperation.seek(33, STREAM::BEGIN);

        /* Get the value. */
        uint64_t nValue = 0;
        ssOperation >> nValue;

        /* Get the script. */
        Legacy::Script scriptPubKey;
        ssOperation >> scriptPubKey;

        /* Get legacy converted output.*/
        txout = Legacy::TxOut(int64_t(nValue), scriptPubKey);

        /* Reset before return. */
        ssOperation.seek(0, STREAM::BEGIN);

        return true;
    }


    /* Detect if the stream is empty.*/
    bool Contract::Empty(const uint8_t nFlags) const
    {
        /* Return flag. */
        bool fRet = false;

        /* Check the operations. */
        if(nFlags & OPERATIONS)
            fRet = (fRet || ssOperation.size() == 0);

        /* Check the operations. */
        if(nFlags & CONDITIONS)
            fRet = (fRet || ssCondition.size() == 0);

        /* Check the operations. */
        if(nFlags & REGISTERS)
            fRet = (fRet || ssRegister.size() == 0);

        return fRet;
    }


    /* Reset the internal stream read pointers.*/
    void Contract::Reset(const uint8_t nFlags) const
    {
        /* Check the operations. */
        if(nFlags & OPERATIONS)
            ssOperation.seek(0, STREAM::BEGIN);

        /* Check the operations. */
        if(nFlags & CONDITIONS)
            ssCondition.seek(0, STREAM::BEGIN);

        /* Check the operations. */
        if(nFlags & REGISTERS)
            ssRegister.seek(0, STREAM::BEGIN);
    }


    /* Clears all contract data*/
    void Contract::Clear(const uint8_t nFlags)
    {
        /* Check the operations. */
        if(nFlags & OPERATIONS)
            ssOperation.SetNull();

        /* Check the operations. */
        if(nFlags & CONDITIONS)
            ssCondition.SetNull();

        /* Check the operations. */
        if(nFlags & REGISTERS)
            ssRegister.SetNull();
    }


    /* Get's a size from internal stream. */
    uint64_t Contract::ReadCompactSize(const uint8_t nFlags) const
    {
        /* We don't use masks here, because it needs to be exclusive to the stream. */
        switch(nFlags)
        {
            case OPERATIONS:
                return ::ReadCompactSize(ssOperation);

            case CONDITIONS:
                return ::ReadCompactSize(ssCondition);

            case REGISTERS:
                return ::ReadCompactSize(ssRegister);
        }

        return 0;
    }


    /* End of the internal stream.*/
    bool Contract::End(const uint8_t nFlags) const
    {
        /* Return flag. */
        bool fRet = false;

        /* Check the operations. */
        if(nFlags & OPERATIONS)
            fRet = (fRet || ssOperation.end());

        /* Check the operations. */
        if(nFlags & CONDITIONS)
            fRet = (fRet || ssCondition.end());

        /* Check the operations. */
        if(nFlags & REGISTERS)
            fRet = (fRet || ssRegister.end());

        return fRet;
    }


    /* Get the raw operation bytes from the contract.*/
    const std::vector<uint8_t>& Contract::Operations() const
    {
        return ssOperation.Bytes();
    }


    /* Get the raw conditions bytes from the contract.*/
    const std::vector<uint8_t>& Contract::Conditions() const
    {
        return ssCondition.Bytes();
    }


    /* Get the register's pre-state from the register script. */
    const TAO::Register::State Contract::PreState() const
    {
        /* Check our minimum register size. */
        if(ssRegister.size() <= 9) //1 byte prestate byte, 8 byte checksum at minimum
            return debug::error(FUNCTION, "contract register script too small ", ssRegister.size());

        /* Seek to first byte. */
        ssRegister.seek(0, STREAM::BEGIN);

        /* Verify the first register code. */
        uint8_t nState = 0;
        ssRegister >> nState;

        /* Check the state is prestate. */
        if(nState != TAO::Register::STATES::PRESTATE)
            return debug::error(FUNCTION, "register state not in pre-state");

        /* Verify the register's prestate. */
        TAO::Register::State tPreState;
        ssRegister >> tPreState;

        /* Return stream to first. */
        ssRegister.seek(0, STREAM::BEGIN);

        return tPreState;
    }


    /* Seek the internal operation stream read pointers.*/
    void Contract::Seek(const uint32_t nPos, const uint8_t nFlags, const uint8_t nType) const
    {
        /* Check the operations. */
        if(nFlags & OPERATIONS)
            ssOperation.seek(nPos, nType);

        /* Check the operations. */
        if(nFlags & CONDITIONS)
            ssCondition.seek(nPos, nType);

        /* Check the operations. */
        if(nFlags & REGISTERS)
            ssRegister.seek(nPos, nType);
    }


    /* Seek the internal operation stream read pointers.*/
    void Contract::Rewind(const uint32_t nPos, const uint8_t nFlags) const
    {
        /* Check the operations. */
        if(nFlags & OPERATIONS)
            ssOperation.seek(int32_t(-1 * nPos));

        /* Check the operations. */
        if(nFlags & CONDITIONS)
            ssCondition.seek(int32_t(-1 * nPos));

        /* Check the operations. */
        if(nFlags & REGISTERS)
            ssRegister.seek(int32_t(-1 * nPos));
    }

    /* Returns the current position of the required stream */
    uint64_t Contract::Position(const uint8_t nFlags) const
    {
        /* We don't use masks here, because it needs to be exclusive to the stream. */
        switch(nFlags)
        {
            case OPERATIONS:
                return ssOperation.pos();

            case CONDITIONS:
                return ssCondition.pos();

            case REGISTERS:
                return ssRegister.pos();
        }

        return 0;
    }


    /* Move the internal operation stream pointer to the position of the primitive operation byte. */
    void Contract::SeekToPrimitive(bool fInclude) const
    {
        /* Sanity checks. */
        if(ssOperation.size() == 0)
            throw debug::exception(FUNCTION, "cannot get primitive when empty");

        /* Reset the operations stream to the beginning */
        ssOperation.seek(0, STREAM::BEGIN);

        /* Get the operation code.*/
        uint8_t nOP = ssOperation.get(0);

        /* Switch for validate or condition. */
        switch(nOP)
        {
            /* Check for condition. */
            case OP::CONDITION:
            {
                /* Seek forward to the next byte, which will contain the primitive op. */
                ssOperation.seek(1 + (fInclude ? 0 : 1));
                break;
            }

            /* Check for validate. */
            case OP::VALIDATE:
            {
                /* Skip over the validate byte, 64 bytes for the transaction hash and 4 bytes for the contract ID . */
                ssOperation.seek(69 + (fInclude ? 0 : 1));
                break;
            }

            /* Nothing to do as the stream is already in the correct position (0) */
            default:
            {
                /* If we don't need primitive at cursor, seek 1 byte ahead since default catch will be at origin cursor 0 */
                if(!fInclude)
                    ssOperation.seek(1);

                break;
            }

        }
    }
}
