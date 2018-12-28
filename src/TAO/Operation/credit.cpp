/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/objects/account.h>
#include <TAO/Register/objects/token.h>

namespace TAO::Operation
{

    /* Commits funds from an account to an account */
    bool Credit(uint512_t hashTx, uint256_t hashProof, uint256_t hashAccount, uint64_t nAmount, uint256_t hashCaller)
    {

        /* Read the claimed transaction. */
        TAO::Ledger::Transaction tx;
        if(!LLD::legDB->ReadTx(hashTx, tx))
            return debug::error(FUNCTION "%s tx doesn't exist", __PRETTY_FUNCTION__, hashTx.ToString().c_str());

        /* Extract the state from tx. */
        uint8_t TX_OP;
        tx >> TX_OP;

        /* Check that prev is coinbase. */
        if(TX_OP == TAO::Operation::OP::COINBASE)
        {
            /* Get the debit from account. */
            uint256_t hashTo;
            tx >> hashTo;

            /* Get the coinbase amount. */
            uint64_t nCredit;
            tx >> nCredit;

        }

        /* Check that prev is debit. */
        else if(TX_OP != TAO::Operation::OP::DEBIT)
            return debug::error(FUNCTION "%s tx claim is not a debit", __PRETTY_FUNCTION__, hashTx.ToString().c_str());

        /* Get the debit from account. */
        uint256_t hashFrom;
        tx >> hashFrom;

        /* Get the debit to account. */
        uint256_t hashTo;
        tx >> hashTo;

        /* Read the to account state. */
        TAO::Register::State stateTo;
        if(!LLD::regDB->ReadState(hashTo, stateTo))
            return debug::error(FUNCTION "%s state to claim not in database", __PRETTY_FUNCTION__, hashTo.ToString().c_str());

        /* Credits specific to account objects. */
        if(stateTo.nType == TAO::Register::OBJECT::ACCOUNT)
        {
            /* Check if this is a whole credit that the transaction is not already connected. */
            if(tx.fConnected)
                return debug::error(FUNCTION "transaction is already spent", __PRETTY_FUNCTION__);

            /* Connect the transaction and write its new state to disk. */
            tx.fConnected = true;
            if(!LLD::legDB->WriteTx(hashTx, tx))
                return debug::error(FUNCTION "failed to change debit transaction state", __PRETTY_FUNCTION__);

            //check the hash proof to the transaction database. Proofs claim a debit so it is no longer sendable
            //transaction state needs to be update in the transaction database as well. The state willb e flagged as true
            //when the debit validation script no longer allows returning to sender
            //hash proof in this case is std::pair<hashtx, hashproof> - consider adding boolean index to keychain with no sector
            //hash proof for object account credit is the genesis id
            //ensure that hashProof == hashCaller in this logic sequence

            /* Read the state from. */
            TAO::Register::State stateAccount;
            if(!LLD::regDB->ReadState(hashAccount, stateAccount))
                return debug::error(FUNCTION "can't read state from", __PRETTY_FUNCTION__);

            /* Check that the creditor has permissions. */
            if(stateAccount.hashOwner != hashCaller)
                return debug::error(FUNCTION "not authorized to credit to this register", __PRETTY_FUNCTION__);

            /* Make sure the claimed account is the debited account. */
            if(hashAccount != hashTo)
                return debug::error(FUNCTION "credit claim is not same account as debit", __PRETTY_FUNCTION__);

            /* Read the state from. */
            TAO::Register::State stateFrom;
            if(!LLD::regDB->ReadState(hashFrom, stateFrom))
                 return debug::error(FUNCTION "can't read state from", __PRETTY_FUNCTION__);

            /* Check the token identifiers. */
            TAO::Register::Account acctFrom;
            stateFrom >> acctFrom;

            TAO::Register::Account acctTo;
            stateTo >> acctTo;

            if(acctFrom.nIdentifier != acctTo.nIdentifier)
                return debug::error(FUNCTION "credit can't be of different identifier", __PRETTY_FUNCTION__);

            /* Get the debit amount. */
            uint64_t nTotal;
            tx >> nTotal;

            /* Check the proper balance requirements. */
            if(nAmount != nTotal)
                 return debug::error(FUNCTION "credit and debit totals don't match", __PRETTY_FUNCTION__);

            /* Credit account balance. */
            acctTo.nBalance += nAmount;

            /* Write new state to the regdb. */
            stateTo.ClearState();
            stateTo << acctTo;

            Write(hashTo, stateTo.GetState(), hashCaller);
            //regDB->WriteState(hashTo, stateTo); //this needs to be executing write script to check for ownership
        }
        else if(stateTo.nType == TAO::Register::OBJECT::RAW || stateTo.nType == TAO::Register::OBJECT::READONLY)
        {
            /* Connect the transaction and write its new state to disk. */
            tx.fConnected = true;
            if(!LLD::legDB->WriteTx(hashTx, tx))
                return debug::error(FUNCTION "failed to change debit transaction state", __PRETTY_FUNCTION__);

            /* Get the state register of this register's owner. */
            TAO::Register::State stateOwner;
            if(!LLD::regDB->ReadState(stateTo.hashOwner, stateOwner))
                return debug::error(FUNCTION "credit from raw object can't be without owner", __PRETTY_FUNCTION__);

            /* Disable any account that's not owned by a token (for now). */
            if(stateOwner.nType != TAO::Register::OBJECT::TOKEN)
                return debug::error(FUNCTION "credit from raw object can't be owned by non token", __PRETTY_FUNCTION__);

            /* Get the token object. */
            TAO::Register::Token token;
            stateOwner >> token;

            /* Check that this proof has not been used in a partial credit. */

            //TODO: make operations logic calculated in memory when received. Process this before block is received.
            //Block is the commitment of the data into the database.
            //TODO: need a rule to check that there are no conflicting states between new transactions
            if(LLD::legDB->HasProof(hashProof, hashTx))
                return debug::error(FUNCTION "credit proof has already been spent", __PRETTY_FUNCTION__);

            /* Write the hash proof to disk. */
            if(!LLD::legDB->WriteProof(hashProof, hashTx))
                return debug::error(FUNCTION "failed to write the credit proof", __PRETTY_FUNCTION__);

            //check the hash proof to the transaction database. Proofs claim a debit so it is no longer reversible in validation script
            //transaction state needs to be update in the transaction database as well. The state will be flagged as true
            //when the debit validation script no longer allows returning to sender
            //hash proof in this case is std::pair<hashtx, hashproof> - consider adding boolean index to keychain with no sector

            /* Check the state register that is being used as proof from creditor. */
            TAO::Register::State stateProof;
            if(!LLD::regDB->ReadState(hashProof, stateProof))
                return debug::error(FUNCTION "credit proof register is not found", __PRETTY_FUNCTION__);

            /* Check that the proof is an account being used. */
            if(stateProof.nType != TAO::Register::OBJECT::ACCOUNT)
                return debug::error(FUNCTION "credit proof register must be account", __PRETTY_FUNCTION__);

            /* Check the ownership of proof register. */
            if(stateProof.hashOwner != hashCaller)
                return debug::error(FUNCTION "not authorized to use this proof register", __PRETTY_FUNCTION__);

            /* Get the proof account object. */
            TAO::Register::Account acctProof;
            stateProof >> acctProof;

            /* Check that the token indetifier matches token identifier. */
            if(acctProof.nIdentifier != token.nIdentifier)
                return debug::error(FUNCTION "account proof identifier not token identifier", __PRETTY_FUNCTION__);

            /* Get the state of debit to account. */
            TAO::Register::State stateAccount;
            if(!LLD::regDB->ReadState(hashAccount, stateAccount))
                return debug::error(FUNCTION "cannot read credit to account register", __PRETTY_FUNCTION__);

            /* Make sure the account to is an object account (for now - otherwise you can have chans of chains of chains). */
            if(stateAccount.nType != TAO::Register::OBJECT::ACCOUNT)
                return debug::error(FUNCTION "credit register is not of account type", __PRETTY_FUNCTION__);

            /* Check that the creditor has permissions. */
            if(stateAccount.hashOwner != hashCaller)
                return debug::error(FUNCTION "not authorized to credit to this register", __PRETTY_FUNCTION__);

            /* Get the total amount of the debit. */
            uint64_t nDebit;
            tx >> nDebit;

            /* Get the total tokens to be distributed. */
            uint64_t nTotal = acctProof.nBalance * nDebit / token.nMaxSupply;

            /* Check that the required credit claim is accurate. */
            if(nTotal != nAmount)
                return debug::error(FUNCTION "claimed credit " PRIu64 " mismatch with token holdings" PRIu64, __PRETTY_FUNCTION__, nAmount, nTotal);

            /* Get the account being credited. */
            TAO::Register::Account acctTo;
            stateAccount >> acctTo;

            /* Read the state from. */
            TAO::Register::State stateFrom;
            if(!LLD::regDB->ReadState(hashFrom, stateFrom))
                return debug::error(FUNCTION "can't read state from", __PRETTY_FUNCTION__);

            /* Get the debiter's register. */
            TAO::Register::Account acctFrom;
            stateFrom >> acctFrom;

            /* Check that the debit to credit identifiers match. */
            if(acctFrom.nIdentifier != acctTo.nIdentifier)
                return debug::error(FUNCTION "credit can't be of different identifier", __PRETTY_FUNCTION__);

            /* Update the state account balance. */
            acctTo.nBalance += nAmount;

            /* Update the register. */
            stateAccount.ClearState();
            stateAccount << acctTo;

            /* Write to the register database. */
            Write(hashAccount, stateAccount.GetState(), hashCaller);
            //regDB->WriteState(hashAccount, stateAccount);
        }

        return true;
    }
}
