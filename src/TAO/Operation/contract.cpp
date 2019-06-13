/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Default Constructor. */
        Contract::Contract()
        : ssOperation()
        , ssCondition()
        , ssRegister()
        , hashCaller(0)
        , nTimestamp(0)
        , hashTx(0)
        , nOutput(0)
        {
        }


        /* Copy Constructor. */
        Contract::Contract(const Contract& contract)
        : ssOperation(contract.ssOperation)
        , ssCondition(contract.ssCondition)
        , ssRegister(contract.ssRegister)
        , hashCaller(contract.hashCaller)
        , nTimestamp(contract.nTimestamp)
        , hashTx(contract.hashTx)
        , nOutput(contract.nOutput)
        {
        }


        /* Move Constructor. */
        Contract::Contract(const Contract&& contract)
        : ssOperation(contract.ssOperation)
        , ssCondition(contract.ssCondition)
        , ssRegister(contract.ssRegister)
        , hashCaller(contract.hashCaller)
        , nTimestamp(contract.nTimestamp)
        , hashTx(contract.hashTx)
        , nOutput(contract.nOutput)
        {
        }


        /* Assignment Operator */
        Contract& Contract::operator=(const Contract& contract)
        {
            /* Set the main streams. */
            ssOperation = contract.ssOperation;
            ssCondition = contract.ssCondition;
            ssRegister  = contract.ssRegister;

            /* Set the transaction reference. */
            hashCaller  = contract.hashCaller;
            nTimestamp  = contract.nTimestamp;
            hashTx      = contract.hashTx;
            nOutput     = contract.nOutput;


            return *this;
        }


        /* Bind the contract to a transaction. */
        void Contract::Bind(const TAO::Ledger::Transaction& tx, uint32_t nContract) const
        {
            hashCaller = tx.hashGenesis;
            nTimestamp = tx.nTimestamp;
            hashTx     = tx.GetHash();
            nOutput    = nContract;
        }


        /* Get the primitive operation. */
        uint8_t Contract::Primitive() const
        {
            /* Sanity checks. */
            if(ssOperation.size() == 0)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "cannot get primitive when empty"));

            /* Return first byte. */
            return ssOperation.get(0);
        }


        uint32_t Contract::Output() const
        {
            return nOutput;
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
            /* We don't use masks here, becuase it needs to be exclusive to the stream. */
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
    }
}
