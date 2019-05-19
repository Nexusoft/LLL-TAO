/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commits funds from an account to an account */
        bool Credit(const uint512_t& hashTx, const uint256_t& hashProof, const uint256_t& hashAccount,
                    const uint64_t nCredit, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAccount))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Read the claimed transaction. */
            TAO::Ledger::Transaction txSpend;

            /* Check disk of writing new block. */
            if((nFlags & TAO::Register::FLAGS::WRITE) && (!LLD::legDB->ReadTx(hashTx, txSpend) || !LLD::legDB->HasIndex(hashTx)))
                return debug::error(FUNCTION, hashTx.ToString(), " tx doesn't exist or not indexed");

            /* Check mempool or disk if not writing. */
            else if(!TAO::Ledger::mempool.Get(hashTx, txSpend) && !LLD::legDB->ReadTx(hashTx, txSpend))
                return debug::error(FUNCTION, hashTx.ToString(), " tx doesn't exist");

            /* Extract the state from tx. */
            uint8_t TX_OP;
            txSpend.ssOperation >> TX_OP;

            /* Read the binary data of the Register. */
            TAO::Register::Object account;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                if(!LLD::regDB->ReadState(hashAccount, account, nFlags))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashAccount.ToString());

                tx.ssRegister << uint8_t(TAO::Register::STATES::PRESTATE) << account;
            }

            /* Get pre-states on write. */
            if(nFlags & TAO::Register::FLAGS::WRITE
            || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the pre-state. */
                if(nState != TAO::Register::STATES::PRESTATE)
                    return debug::error(FUNCTION, "register script not in pre-state");

                /* Get the pre-state. */
                tx.ssRegister >> account;
            }

            /* Check that the creditor has permissions. */
            if(account.hashOwner != tx.hashGenesis)
                return debug::error(FUNCTION, "not authorized to credit to this register");

            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check that we are crediting to an account object register. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "Cannot credit to a non-account base object");

            /* Check that prev is coinbase. */
            if(TX_OP == OP::COINBASE) //NOTE: thie coinbase can't be spent unless flag is byte 0. Safe to use this for coinbase flag.
            {
                /* Check that the proof is the genesis. */
                if(hashProof != tx.hashGenesis)
                    return debug::error(FUNCTION, "proof for coinbase needs to be genesis");

                /* Check if this is a whole credit that the transaction is not already connected. */
                if(LLD::legDB->HasProof(hashProof, hashTx, nFlags))
                    return debug::error(FUNCTION, "transaction is already spent");

                /* Get the coinbase amount. */
                uint64_t nCoinbase;
                txSpend.ssOperation >> nCoinbase;

                /* Make sure the claimed account is the debited account. */
                if(txSpend.hashGenesis != tx.hashGenesis)
                    return debug::error(FUNCTION, "cannot claim coinbase from different sigchain");

                /* Check the identifier. */
                if(account.get<uint256_t>("identifier") != 0)
                    return debug::error(FUNCTION, "can't credit a coinbase for identifier other than 0");

                /* Check that the balances match. */
                if(nCoinbase != nCredit)
                    return debug::error(FUNCTION, "credit ", nCredit, "and coinbase ", nCredit, " amounts mismatch");

                /* Write the new balance to object register. */
                if(!account.Write("balance", account.get<uint64_t>("balance") + nCredit))
                    return debug::error(FUNCTION, "balance could not be written to object register");

                /* Update the state register's timestamp. */
                account.nTimestamp = tx.nTimestamp;
                account.SetChecksum();

                /* Check that the register is in a valid state. */
                if(!account.IsValid())
                    return debug::error(FUNCTION, "memory address ", hashAccount.ToString(), " is in invalid state");

                /* Write post-state checksum. */
                if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                    tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << account.GetHash();

                /* Verify the post-state checksum. */
                if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssRegister >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::POSTSTATE)
                        return debug::error(FUNCTION, "register script not in post-state");

                    /* Get the post state checksum. */
                    uint64_t nChecksum;
                    tx.ssRegister >> nChecksum;

                    /* Check for matching post states. */
                    if(nChecksum != account.GetHash())
                        return debug::error(FUNCTION, "register script has invalid post-state");

                    /* Write the proof spend. */
                    if(!LLD::legDB->WriteProof(hashProof, hashTx, nFlags))
                        return debug::error(FUNCTION, "failed to write proof");

                    /* Write the register to the database. */
                    if(!LLD::regDB->WriteState(hashAccount, account, nFlags))
                        return debug::error(FUNCTION, "failed to write new state");
                }

                return true;
            }

            /* Check that prev is debit. */
            else if(TX_OP != OP::DEBIT)
                return debug::error(FUNCTION,  hashTx.ToString(), " tx claim is not a debit");

            /* Get the debit from account. */
            uint256_t hashFrom;
            txSpend.ssOperation >> hashFrom;

            /* Get the debit to account. */
            uint256_t hashTo;
            txSpend.ssOperation >> hashTo;

            /* Handle a return to self. */
            if(txSpend.hashGenesis == tx.hashGenesis)
            {
                /* Check the proof as being the caller. */
                if(hashProof != hashFrom)
                    return debug::error(FUNCTION, "hash proof must be hashFrom for return to self");

                /* Check if this is a whole credit that the transaction is not already spent. */
                if(LLD::legDB->HasProof(hashProof, hashTx, nFlags))
                    return debug::error(FUNCTION, "transaction is already spent");

                /* Read the to account state. */
                //if(hashAccount != hashFrom) //NOTE: this rule is disabled for now. Return to self should be able to done to other accounts
                //    return debug::error(FUNCTION, "cannot return funds to self if differnet account");

                /* Get the debit amount. */
                uint64_t nDebit;
                txSpend.ssOperation >> nDebit;

                /* Check the proper balance requirements. */
                if(nCredit != nDebit)
                     return debug::error(FUNCTION, "credit and debit totals don't match");

                /* Write the new balance to object register. */
                if(!account.Write("balance", account.get<uint64_t>("balance") + nCredit))
                    return debug::error(FUNCTION, "balance could not be written to object register");

                /* Update the state register's timestamp. */
                account.nTimestamp = tx.nTimestamp;
                account.SetChecksum();

                /* Check that the register is in a valid state. */
                if(!account.IsValid())
                    return debug::error(FUNCTION, "memory address ", hashAccount.ToString(), " is in invalid state");

                /* Write post-state checksum. */
                if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                    tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << account.GetHash();

                /* Verify the post-state checksum. */
                if(nFlags & TAO::Register::FLAGS::WRITE
                || nFlags & TAO::Register::FLAGS::MEMPOOL)
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssRegister >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::POSTSTATE)
                        return debug::error(FUNCTION, "register script not in post-state");

                    /* Get the post state checksum. */
                    uint64_t nChecksum;
                    tx.ssRegister >> nChecksum;

                    /* Check for matching post states. */
                    if(nChecksum != account.GetHash())
                        return debug::error(FUNCTION, "register script has invalid post-state");

                    /* Read the register from the database. */
                    TAO::Register::State stateTo;
                    if(!LLD::regDB->ReadState(hashTo, stateTo, nFlags))
                        return debug::error(FUNCTION, "register address doesn't exist ", hashTo.ToString());

                    /* Write the proof spend. */
                    if(!LLD::legDB->WriteProof(hashProof, hashTx, nFlags))
                        return debug::error(FUNCTION, "failed to write proof");

                    /* Write the register to the database. */
                    if(!LLD::regDB->WriteState(hashAccount, account, nFlags))
                        return debug::error(FUNCTION, "failed to write new state");

                    /* Erase the event to the ledger database. */
                    //if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::legDB->EraseEvent(stateTo.hashOwner))
                    //    return debug::error(FUNCTION, "failed to rollback event to register DB");
                }

                return true;
            }


            /* Read the account to state. */
            TAO::Register::State stateTo;
            if(!LLD::regDB->ReadState(hashTo, stateTo))
                return debug::error(FUNCTION, "couldn't read debit to address");

            /* Credits specific to account objects. */
            if(stateTo.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Check the proof as being the caller. */
                if(hashProof != hashFrom)
                    return debug::error(FUNCTION, "hash proof must hashFrom for credit to account");

                /* Check if this is a whole credit that the transaction is not already spent. */
                if(LLD::legDB->HasProof(hashProof, hashTx, nFlags))
                    return debug::error(FUNCTION, "transaction is already spent");

                /* Read the to account state. */
                if(hashTo != hashAccount)
                    return debug::error(FUNCTION, "debit to account mismatch with credit to account");

                /* Read the state from. */
                TAO::Register::Object accountFrom;
                if(!LLD::regDB->ReadState(hashFrom, accountFrom, nFlags))
                    return debug::error(FUNCTION, "can't read state from");

                /* Parse the account from. */
                if(!accountFrom.Parse())
                    return debug::error(FUNCTION, "failed to parse account from");

                /* Check debit hashFrom is a base object account. */
                if(accountFrom.Base() != TAO::Register::OBJECTS::ACCOUNT)
                    return debug::error(FUNCTION, "debit from must have a base account object");

                /* Check token identifiers. */
                if(accountFrom.get<uint256_t>("identifier") != account.get<uint256_t>("identifier"))
                    return debug::error(FUNCTION, "credit can't be of different identifier");

                /* Get the debit amount. */
                uint64_t nDebit;
                txSpend.ssOperation >> nDebit;

                /* Check the proper balance requirements. */
                if(nCredit != nDebit)
                     return debug::error(FUNCTION, "credit and debit totals don't match");

                /* Write the new balance to object register. */
                if(!account.Write("balance", account.get<uint64_t>("balance") + nCredit))
                    return debug::error(FUNCTION, "balance could not be written to object register");

                /* Update the state register's timestamp. */
                account.nTimestamp = tx.nTimestamp;
                account.SetChecksum();

                /* Check that the register is in a valid state. */
                if(!account.IsValid())
                    return debug::error(FUNCTION, "memory address ", hashAccount.ToString(), " is in invalid state");

                /* Write post-state checksum. */
                if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                    tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << account.GetHash();

                /* Verify the post-state checksum. */
                if(nFlags & TAO::Register::FLAGS::WRITE
                || nFlags & TAO::Register::FLAGS::MEMPOOL)
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssRegister >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::POSTSTATE)
                        return debug::error(FUNCTION, "register script not in post-state");

                    /* Get the post state checksum. */
                    uint64_t nChecksum;
                    tx.ssRegister >> nChecksum;

                    /* Check for matching post states. */
                    if(nChecksum != account.GetHash())
                        return debug::error(FUNCTION, "register script has invalid post-state");

                    /* Write the proof spend. */
                    if(!LLD::legDB->WriteProof(hashProof, hashTx, nFlags))
                        return debug::error(FUNCTION, "failed to write proof");

                    /* Write the register to the database. */
                    if(!LLD::regDB->WriteState(hashAccount, account, nFlags))
                        return debug::error(FUNCTION, "failed to write new state");
                }
            }
            else if(stateTo.nType == TAO::Register::REGISTER::RAW
                 || stateTo.nType == TAO::Register::REGISTER::READONLY)
            {
                /* Check that this proof has not been used in a partial credit. */
                if(LLD::legDB->HasProof(hashProof, hashTx, nFlags))
                    return debug::error(FUNCTION, "temporal proof has already been spent");

                /* Get the state register of this register's owner. */
                TAO::Register::Object tokenOwner;
                if(!LLD::regDB->ReadState(stateTo.hashOwner, tokenOwner, nFlags))
                    return debug::error(FUNCTION, "credit from raw object can't be without owner");

                /* Parse the owner object register. */
                if(!tokenOwner.Parse())
                    return debug::error(FUNCTION, "failed to parse owner object register");

                /* Check that owner is of a valid type. */
                if(tokenOwner.Standard() != TAO::Register::OBJECTS::TOKEN)
                    return debug::error(FUNCTION, "owner object is not a token");

                /* Check the state register that is being used as proof from creditor. */
                TAO::Register::Object accountProof;
                if(!LLD::regDB->ReadState(hashProof, accountProof, nFlags))
                    return debug::error(FUNCTION, "temporal proof register is not found");

                /* Compare the last timestamp update to transaction timestamp. */
                if(accountProof.nTimestamp > txSpend.nTimestamp)
                    return debug::error(FUNCTION, "temporal proof is stale");

                /* Parse the owner object register. */
                if(!accountProof.Parse())
                    return debug::error(FUNCTION, "failed to parse proof object register");

                /* Check that owner is of a valid type. */
                if(accountProof.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                    return debug::error(FUNCTION, "owner object is not a token");

                /* Check the ownership of proof register. */
                if(accountProof.hashOwner != tx.hashGenesis)
                    return debug::error(FUNCTION, "not authorized to use this temporal proof");

                /* Check that the token indetifier matches token identifier. */
                if(accountProof.get<uint256_t>("identifier") != tokenOwner.get<uint256_t>("identifier"))
                    return debug::error(FUNCTION, "account proof identifier not token identifier");

                /* Get the total amount of the debit. */
                uint64_t nDebit;
                txSpend.ssOperation >> nDebit;

                /* Get the total tokens to be distributed. */
                uint64_t nPartial = (accountProof.get<uint64_t>("balance") * nDebit) / tokenOwner.get<uint64_t>("supply");

                //NOTE: ISSUE here, temporal proofs can't be used if post timestamped. This prevents double spending,
                //but it doesn't prevent coins from getting locked if a temporal proof has been changed. Possible to
                //check tokens to ensure tokens aren't moved if a proof is able to be claimed.
                //find a way to unlock the unspent tokens possibly with validation regitser.
                //We might need to check back the history to find timestamp before proof to use previous state.
                //This will work for now, but is not production ready.

                /* Check that the required credit claim is accurate. */
                if(nPartial != nCredit)
                    return debug::error(FUNCTION, "claimed credit ", nCredit, " mismatch with token holdings ", nPartial);

                /* Read the state from. */
                TAO::Register::Object accountFrom;
                if(!LLD::regDB->ReadState(hashFrom, accountFrom, nFlags))
                    return debug::error(FUNCTION, "can't read state from");

                /* Parse the account from. */
                if(!accountFrom.Parse())
                    return debug::error(FUNCTION, "failed to parse the account from");

                /* Check that account from is a standard object. */
                if(accountFrom.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                    return debug::error(FUNCTION, "account from object register is non-standard type");

                /* Check that the debit to credit identifiers match. */
                if(account.get<uint256_t>("identifier") != accountFrom.get<uint256_t>("identifier"))
                    return debug::error(FUNCTION, "credit can't be of different identifier");

                /* Write the new balance to object register. */
                if(!account.Write("balance", account.get<uint64_t>("balance") + nPartial))
                    return debug::error(FUNCTION, "balance could not be written to object register");

                /* Update the state register's timestamp. */
                account.nTimestamp = tx.nTimestamp;
                account.SetChecksum();

                /* Check that the register is in a valid state. */
                if(!account.IsValid())
                    return debug::error(FUNCTION, "memory address ", hashAccount.ToString(), " is in invalid state");

                /* Write post-state checksum. */
                if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                    tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << account.GetHash();

                /* Verify the post-state checksum. */
                if(nFlags & TAO::Register::FLAGS::WRITE
                || nFlags & TAO::Register::FLAGS::MEMPOOL)
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssRegister >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::POSTSTATE)
                        return debug::error(FUNCTION, "register script not in post-state");

                    /* Get the post state checksum. */
                    uint64_t nChecksum;
                    tx.ssRegister >> nChecksum;

                    /* Check for matching post states. */
                    if(nChecksum != account.GetHash())
                        return debug::error(FUNCTION, "register script has invalid post-state");

                    /* Write the proof spend. */
                    if(!LLD::legDB->WriteProof(hashProof, hashTx, nFlags))
                        return debug::error(FUNCTION, "failed to write proof");

                    /* Write the register to the database. */
                    if(!LLD::regDB->WriteState(hashAccount, account, nFlags))
                        return debug::error(FUNCTION, "failed to write new state");
                }
            }

            /* If nothing was executed, return false. */
            else
                return false;

            return true;
        }
    }
}
