/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

#include <LLD/include/register.h>
#include <LLD/include/ledger.h>

#include <Util/include/debug.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Register/include/state.h>

namespace TAO
{

    namespace Operation
    {

        /** Execute
         *
         *  Executes a given operation byte sequence.
         *
         *  @param[in] regDB The register database to execute on
         *  @param[in] hashOwner The owner executing the register batch.
         *
         *  @return True if operations executed successfully.
         *
         **/
        bool Execute(std::vector<uint8_t> vchData, LLD::RegisterDB* regDB, LLD::LedgerDB* legDB, uint256_t hashOwner)
        {
            /* Create the operations stream to execute. */
            Stream stream = Stream(vchData);

            while(!stream.End())
            {
                uint8_t OP_CODE;
                stream >> OP_CODE;

                /* Check the current opcode. */
                switch(OP_CODE)
                {

                    /* Record a new state to the register. */
                    case OP_WRITE:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress;
                        stream >> hashAddress;

                        /* Read the binary data of the Register. */
                        TAO::Register::State regState;
                        if(!regDB->ReadState(hashAddress, regState))
                            return error(FUNCTION "Register Address doewn't exist %s", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

                        /* Check ReadOnly permissions. */
                        if(regState.fReadOnly)
                            return error(FUNCTION "Write operation called on read-only register", __PRETTY_FUNCTION__);

                        /*state Check that the proper owner is commiting the write. */
                        if(hashOwner != regState.hashOwner)
                            return error(FUNCTION "No write permissions for owner %s", __PRETTY_FUNCTION__, hashOwner.ToString().c_str());

                        /* Deserialize the register from stream. */
                        TAO::Register::State regNew;
                        stream >> regNew;

                        /* Check the new data size against register's allocated size. */
                        if(regNew.nLength != regState.nLength)
                            return error(FUNCTION "New Register State size %u mismatch %u", __PRETTY_FUNCTION__, regNew.nLength, regState.nLength);

                        /* Set the new state of the register. */
                        regState.SetState(regNew.GetState());

                        /* Write the register to the database. */
                        if(!regDB->WriteState(hashAddress, regState))
                            return error(FUNCTION "Failed to write new state", __PRETTY_FUNCTION__);

                        break;
                    }


                    /* Create a new register. */
                    case OP_REGISTER:
                    {
                        /* Extract the address from the stream. */
                        uint256_t hashAddress;
                        stream >> hashAddress;

                        /* Check that the register doesn't exist yet. */
                        if(regDB->HasState(hashAddress))
                            return error(FUNCTION "Cannot allocate register of same memory address %s", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

                        /* Set the register's state */
                        TAO::Register::State regState = TAO::Register::State();
                        stream >> regState;

                        /* Write the register to database. */
                        if(!regDB->WriteState(hashAddress, regState))
                            return error(FUNCTION "Failed to write state register %s into register DB", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case OP_TRANSFER:
                    {
                        /* Extract the address from the stream. */
                        uint256_t hashAddress;
                        stream >> hashAddress;

                        /* Read the register transfer recipient. */
                        uint256_t hashTransfer;
                        stream >> hashTransfer;

                        /* Read the register from the database. */
                        TAO::Register::State regState = TAO::Register::State();
                        if(!regDB->ReadState(hashAddress, regState))
                            return error(FUNCTION "Register %s doesn't exist in register DB", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

                        /* Make sure that you won the rights to register first. */
                        if(regState.hashOwner != hashOwner)
                            return error(FUNCTION "%s not authorized to transfer register", __PRETTY_FUNCTION__, hashOwner.ToString().c_str());

                        /* Set the new owner of the register. */
                        regState.hashOwner = hashTransfer;
                        if(!regDB->WriteState(hashAddress, regState))
                            return error(FUNCTION "Failed to write new owner for register", __PRETTY_FUNCTION__);

                        break;
                    }

                    /* Debit tokens from an account you own. */
                    case OP_DEBIT:
                    {
                        uint256_t hashFrom; //the register address debit is being sent from. Hard reject if this register isn't account id
                        stream >> hashFrom;

                        uint256_t hashTo;   //the register address debit is being sent to. Hard reject if this register isn't an account id
                        stream >> hashTo;

                        uint64_t  nAmount;  //the amount to be transfered
                        stream >> nAmount;

                        /* Read the register from the database. */
                        TAO::Register::State regFrom = TAO::Register::State();
                        if(!regDB->ReadState(hashFrom, regFrom))
                            return error(FUNCTION "Register %s doesn't exist in register DB", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

                        /* Check ownership of register. */
                        if(regFrom.hashOwner != hashOwner)
                            return error(FUNCTION "%s not authorized to debit from register", __PRETTY_FUNCTION__, hashOwner.ToString().c_str());

                        /* Skip all non account registers for now. */
                        if(regFrom.nType != TAO::Register::OBJECT_ACCOUNT)
                            return error(FUNCTION "%s is not an account object", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

                        /* Get the account object from register. */
                        TAO::Register::Account acctFrom;
                        regFrom >> acctFrom;

                        /* Check the balance of the from account. */
                        if(acctFrom.nBalance < nAmount)
                            return error(FUNCTION "%s doesn't have sufficient balance", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

                        /* Change the state of account register. */
                        acctFrom.nBalance -= nAmount;

                        /* Clear the state of register. */
                        regFrom.ClearState();
                        regFrom << acctFrom;

                        /* Write this to operations stream. */
                        regDB->WriteState(hashFrom, regFrom); //need to do this in operation script to check ownership of register
                        //stream << OP_WRITE << hashFrom << regFrom;
                        //TODO: figure out why adding to stream doesn't work right

                        break;
                    }

                    case OP_CREDIT:
                    {
                        /* The transaction that this credit is claiming. */
                        uint512_t hashTx;
                        stream >> hashTx;

                        /* The proof this credit is using to make claims. */
                        uint256_t hashProof;
                        stream >> hashProof;

                        /* Read the claimed transaction. */
                        TAO::Ledger::Transaction tx;
                        if(!legDB->ReadTx(hashTx, tx))
                            return error(FUNCTION "%s tx doesn't exist", __PRETTY_FUNCTION__, hashTx.ToString().c_str());

                        /* Extract the state from tx. */
                        uint8_t TX_OP;
                        tx >> TX_OP;

                        /* Check that prev is debit. */
                        if(TX_OP != TAO::Operation::OP_DEBIT)
                            return error(FUNCTION "%s tx claim is not a debit", __PRETTY_FUNCTION__, hashTx.ToString().c_str());

                        /* Get the debit from account. */
                        uint256_t hashFrom;
                        tx >> hashFrom;

                        /* Get the debit to account. */
                        uint256_t hashTo;
                        tx >> hashTo;

                        /* Read the to account state. */
                        TAO::Register::State stateTo;
                        if(!regDB->ReadState(hashTo, stateTo))
                            return error(FUNCTION "%s state to claim not in database", __PRETTY_FUNCTION__, hashTo.ToString().c_str());

                        /* Credits specific to account objects. */
                        if(stateTo.nType == TAO::Register::OBJECT_ACCOUNT)
                        {
                            /* The account that is being credited. */
                            uint256_t hashAccount;
                            stream >> hashAccount;

                            /* Make sure the claimed account is the debited account. */
                            if(hashAccount != hashTo)
                                return error(FUNCTION "credit claim is not same account as debit", __PRETTY_FUNCTION__);

                            /* Read the state from. */
                            TAO::Register::State stateFrom;
                            if(!regDB->ReadState(hashFrom, stateFrom))
                                return error(FUNCTION "can't read state from", __PRETTY_FUNCTION__);

                            /* Check the token identifiers. */
                            TAO::Register::Account acctFrom;
                            stateFrom >> acctFrom;

                            TAO::Register::Account acctTo;
                            stateTo >> acctTo;

                            if(acctFrom.nIdentifier != acctTo.nIdentifier)
                                return error(FUNCTION "credit can't be of different identifier");

                            /* The total to be credited. */
                            uint64_t  nAmount;
                            stream >> nAmount;

                            /* Get the debit amount. */
                            uint64_t nTotal;
                            tx >> nTotal;

                            /* Check the proper balance requirements. */
                            if(nAmount != nTotal)
                                return error(FUNCTION "trying to credit more than debit");

                            /* Credit account balance. */
                            acctTo.nBalance += nAmount;

                            /* Write new state to the regdb. */
                            stateTo.ClearState();
                            stateTo << acctTo;

                            regDB->WriteState(hashTo, stateTo); //this needs to be executing write script to check for ownership
                        }

                        /*
                        else if(stateTo.nType == TAO::Register::OBJECT_ACCOUNT)
                        {
                            //this is a normal credit operation. prove that you own this account and write state.
                            ssState >> accountTo;

                            accountTo.nBalance += nAmount;

                            if(stateTo.hashOwner != Transaction.hashGenesis) //fail

                            if(hashProof != stateTo.hashOwner) //fail

                            stateTo.SetState(accountTo.Serialize()); //need serialize in and out methods. CDataStream is too clunky here
                            regDB.WriteState(hashTo, stateTo); //only you can write this state since you are the owner of this register. handle in OP_WRITE operation
                        }
                        */
                    }
                }
            }

            /* If nothing failed, return true for evaluation. */
            return true;
        }
    }
}

#endif
