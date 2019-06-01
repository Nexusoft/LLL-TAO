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

        /** Default Constructor. **/
        Contract::Contract()
        : ssOperation()
        , ssCondition()
        , ssRegister()
        , hashCaller(0)
        , nTimestamp(0)
        , hashTx(0)
        {
        }


        /** Copy Constructor. **/
        Contract::Contract(const Contract& contract)
        : ssOperation(contract.ssOperation)
        , ssCondition(contract.ssCondition)
        , ssRegister(contract.ssRegister)
        , hashCaller(contract.hashCaller)
        , nTimestamp(contract.nTimestamp)
        , hashTx(contract.hashTx)
        {
        }


        /** Move Constructor. **/
        Contract::Contract(const Contract&& contract)
        : ssOperation(contract.ssOperation)
        , ssCondition(contract.ssCondition)
        , ssRegister(contract.ssRegister)
        , hashCaller(contract.hashCaller)
        , nTimestamp(contract.nTimestamp)
        , hashTx(contract.hashTx)
        {
        }


        /** Assignment Operator **/
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


            return *this;
        }


        /* Bind the contract to a transaction. */
        void Contract::Bind(const TAO::Ledger::Transaction& tx) const
        {
            hashCaller = tx.hashGenesis;
            nTimestamp = tx.nTimestamp;
            hashTx     = tx.GetHash();
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
        bool Contract::Empty() const
        {
            return (ssOperation.size() == 0);
        }


        /* Detect if there are any conditions present.*/
        bool Contract::HasConditions() const
        {
            return (ssCondition.size() == 0);
        }


        /*Reset the internal stream read pointers.*/
        void Contract::Reset() const
        {
            /* Set the operation stream to beginning. */
            ssOperation.seek(0, STREAM::BEGIN);

            /* Set the condition stream to beginning. */
            ssCondition.seek(0, STREAM::BEGIN);

            /* Set the register stream to beginning. */
            ssRegister.seek(0, STREAM::BEGIN);
        }


        /* Clears all contract data*/
        void Contract::Clear()
        {
            /* Set the operation stream to beginning. */
            ssOperation.SetNull();

            /* Set the condition stream to beginning. */
            ssCondition.SetNull();

            /* Set the register stream to beginning. */
            ssRegister.SetNull();
        }


        /*End of the internal stream.*/
        bool Contract::End(const uint8_t nType) const
        {
            if(nType == 0)
                return ssOperation.end();

            return ssCondition.end();
        }


        /*Get the raw operation bytes from the contract.*/
        const std::vector<uint8_t>& Contract::Operations() const
        {
            return ssOperation.Bytes();
        }


        /* Get the raw condition bytes from the contract.*/
        const std::vector<uint8_t>& Contract::Conditions() const
        {
            return ssCondition.Bytes();
        }


        /*Seek the internal operation stream read pointers.*/
        void Contract::Seek(const uint32_t nPos) const
        {
            /* Set the operation stream to beginning. */
            ssOperation.seek(nPos, STREAM::BEGIN);

            /* Set the condition stream to beginning. */
            ssCondition.seek(0, STREAM::BEGIN);

            /* Set the register stream to beginning. */
            ssRegister.seek(0, STREAM::BEGIN);
        }


        /*Seek the internal operation stream read pointers.*/
        void Contract::Rewind(const uint32_t nPos) const
        {
            /* Set the operation stream back current position. */
            ssCondition.seek(int32_t(-1 * nPos));
        }
    }
}
