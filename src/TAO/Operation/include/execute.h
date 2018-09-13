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

                        break;
                    }

                    case OP_CREDIT:
                    {
                        /* The transaction that this credit is claiming. */
                        uint512_t hashTx;
                        stream >> hashTx;

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
                            /* The proof this credit is using to make claims. */
                            uint256_t hashProof;
                            stream >> hashProof;

                            //check the hash proof to the transaction database. Proofs claim a debit so it is no longer sendable
                            //transaction state needs to be update in the transaction database as well. The state willb e flagged as true
                            //when the debit validation script no longer allows returning to sender
                            //hash proof in this case is std::pair<hashtx, hashproof> - consider adding boolean index to keychain with no sector
                            //hash proof for object account credit is the genesis id
                            //ensure that hashProof == hashOwner in this logic sequence

                            /* The account that is being credited. */
                            uint256_t hashAccount;
                            stream >> hashAccount;

                            /* Read the state from. */
                            TAO::Register::State stateAccount;
                            if(!regDB->ReadState(hashAccount, stateAccount))
                                return error(FUNCTION "can't read state from", __PRETTY_FUNCTION__);

                            /* Check that the creditor has permissions. */
                            if(stateAccount.hashOwner != hashOwner)
                                return error(FUNCTION "not authorized to credit to this register", __PRETTY_FUNCTION__);

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
                                return error(FUNCTION "credit can't be of different identifier", __PRETTY_FUNCTION__);

                            /* The total to be credited. */
                            uint64_t  nAmount;
                            stream >> nAmount;

                            /* Get the debit amount. */
                            uint64_t nTotal;
                            tx >> nTotal;

                            /* Check the proper balance requirements. */
                            if(nAmount != nTotal)
                                return error(FUNCTION "credit and debit totals don't match", __PRETTY_FUNCTION__);

                            /* Credit account balance. */
                            acctTo.nBalance += nAmount;

                            /* Write new state to the regdb. */
                            stateTo.ClearState();
                            stateTo << acctTo;

                            regDB->WriteState(hashTo, stateTo); //this needs to be executing write script to check for ownership
                        }
                        else if(stateTo.nType == TAO::Register::OBJECT_RAW)
                        {
                            /* Get the state register of this register's owner. */
                            TAO::Register::State stateOwner;
                            if(!regDB->ReadState(stateTo.hashOwner, stateOwner))
                                return error(FUNCTION "credit from raw object can't be without owner", __PRETTY_FUNCTION__);

                            /* Disable any account that's not owned by a token (for now). */
                            if(stateOwner.nType != TAO::Register::OBJECT_TOKEN)
                                return error(FUNCTION "credit from raw object can't be owned by non token", __PRETTY_FUNCTION__);

                            /* Get the token object. */
                            TAO::Register::Token token;
                            stateOwner >> token;

                            /* The proof this credit is using to make claims. */
                            uint256_t hashProof;
                            stream >> hashProof;

                            //check the hash proof to the transaction database. Proofs claim a debit so it is no longer reversible in validation script
                            //transaction state needs to be update in the transaction database as well. The state willb e flagged as true
                            //when the debit validation script no longer allows returning to sender
                            //hash proof in this case is std::pair<hashtx, hashproof> - consider adding boolean index to keychain with no sector

                            /* Check the state register that is being used as proof from creditor. */
                            TAO::Register::State stateProof;
                            if(!regDB->ReadState(hashProof, stateProof))
                                return error(FUNCTION "credit proof register is not found", __PRETTY_FUNCTION__);

                            /* Check that the proof is an account being used. */
                            if(stateProof.nType != TAO::Register::OBJECT_ACCOUNT)
                                return error(FUNCTION "credit proof register must be account", __PRETTY_FUNCTION__);

                            /* Check the ownership of proof register. */
                            if(stateProof.hashOwner != hashOwner)
                                return error(FUNCTION "not authorized to use this proof register", __PRETTY_FUNCTION__);

                            /* Get the proof account object. */
                            TAO::Register::Account acctProof;
                            stateProof >> acctProof;

                            /* Check that the token indetifier matches token identifier. */
                            if(acctProof.nIdentifier != token.nIdentifier)
                                return error(FUNCTION "account proof identifier not token identifier", __PRETTY_FUNCTION__);

                            /* The account that is being credited. */
                            uint256_t hashAccount;
                            stream >> hashAccount;

                            /* Get the state of debit to account. */
                            TAO::Register::State stateAccount;
                            if(!regDB->ReadState(hashAccount, stateAccount))
                                return error(FUNCTION "cannot read credit to account register", __PRETTY_FUNCTION__);

                            /* Make sure the account to is an object account (for now - otherwise you can have chans of chains of chains). */
                            if(stateAccount.nType != TAO::Register::OBJECT_ACCOUNT)
                                return error(FUNCTION "credit register is not of account type", __PRETTY_FUNCTION__);

                            /* Check that the creditor has permissions. */
                            if(stateAccount.hashOwner != hashOwner)
                                return error(FUNCTION "not authorized to credit to this register", __PRETTY_FUNCTION__);

                            /* Get the total credit is claiming from previous debit. */
                            uint64_t nCredit;
                            stream >> nCredit;

                            /* Get the total amount of the debit. */
                            uint64_t nDebit;
                            tx >> nDebit;

                            /* Get the total tokens to be distributed. */
                            uint64_t nTotal = acctProof.nBalance * nDebit / token.nMaxSupply;

                            /* Check that the required credit claim is accurate. */
                            if(nTotal != nCredit)
                                return error(FUNCTION "claimed credit " PRIu64 " mismatch with token holdings" PRIu64, __PRETTY_FUNCTION__, nCredit, nTotal);

                            /* Get the account being credited. */
                            TAO::Register::Account acctTo;
                            stateAccount >> acctTo;

                            /* Read the state from. */
                            TAO::Register::State stateFrom;
                            if(!regDB->ReadState(hashFrom, stateFrom))
                                return error(FUNCTION "can't read state from", __PRETTY_FUNCTION__);

                            /* Get the debiter's register. */
                            TAO::Register::Account acctFrom;
                            stateFrom >> acctFrom;

                            /* Check that the debit to credit identifiers match. */
                            if(acctFrom.nIdentifier != acctTo.nIdentifier)
                                return error(FUNCTION "credit can't be of different identifier", __PRETTY_FUNCTION__);

                            /* Update the state account balance. */
                            acctTo.nBalance += nCredit;

                            /* Update the register. */
                            stateAccount.ClearState();
                            stateAccount << acctTo;

                            /* Write to the register database. */
                            regDB->WriteState(hashAccount, stateAccount);
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
