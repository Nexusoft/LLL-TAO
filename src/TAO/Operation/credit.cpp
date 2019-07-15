/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/credit.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Credit::Commit(const TAO::Register::Object& account, const Contract& debit,
                            const uint256_t& hashAddress, const uint256_t& hashProof, const uint512_t& hashTx,
                            const uint32_t nContract, const uint64_t nAmount, const uint8_t nFlags)
        {
            /* Check if this transfer is already claimed. */
            if(LLD::Ledger->HasProof(hashProof, hashTx, nContract, nFlags))
                return debug::error(FUNCTION, "credit is already claimed");

            /* Write the claimed proof. */
            if(!LLD::Ledger->WriteProof(hashProof, hashTx, nContract, nFlags))
                return debug::error(FUNCTION, "failed to write credit proof");

            /* Read the debit. */
            debit.Reset();
            debit.Seek(1);

            /* Get address from. */
            uint256_t hashFrom = 0;
            debit  >> hashFrom;

            /* Check for partial credits. */
            if(account.hashOwner != hashProof && hashFrom != hashProof)
            {
                /* Get the partial amount. */
                uint64_t nClaimed = 0;
                if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, nFlags))
                    nClaimed = 0; //reset value to double check here and continue

                /* Write the new claimed amount. */
                if(!LLD::Ledger->WriteClaimed(hashTx, nContract, (nClaimed + nAmount), nFlags))
                    return debug::error(FUNCTION, "failed to update claimed amount");
            }

            /* Write the new register's state. */
            if(!LLD::Register->WriteState(hashAddress, account, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Commits funds from an account to an account */
        bool Credit::Execute(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check that we are crediting to an account object register. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "cannot credit to a non-account base object");

            /* Write the new balance to object register. */
            if(!account.Write("balance", account.get<uint64_t>("balance") + nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            account.nModified = nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }


        /* Verify claim validation rules and caller. */
        bool Credit::Verify(const Contract& contract, const Contract& debit, const uint8_t nFlags)
        {
            /* Rewind to first OP. */
            contract.Rewind(69, Contract::OPERATIONS);

            /* Reset register streams. */
            contract.Reset(Contract::REGISTERS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::CREDIT)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the transaction from contract. */
            uint512_t hashTx = 0;
            contract >> hashTx;

            /* Extract the contract-id. */
            uint32_t nContract = 0;
            contract >> nContract;

            /* Read the to account that contract is operating on. */
            TAO::Register::Address hashAccount;
            contract >> hashAccount;

            /* Get the proof hash. */
            TAO::Register::Address hashProof;
            contract >> hashProof;

            /* Read the contract totals. */
            uint64_t  nCredit = 0;
            contract >> nCredit;

            /* Get the byte from pre-state. */
            uint8_t nState = 0;
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register contract not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object account;
            contract >>= account;

            /* Check contract account */
            if(contract.Caller() != account.hashOwner)
                return debug::error(FUNCTION, "no write permissions for caller ", contract.Caller().SubString());

            /* Parse the account. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account");

            /* Seek claim read position to first. */
            debit.Reset();

            /* Get operation byte. */
            OP = 0;
            debit >> OP;

            /* Check for condition or validate. */
            switch(OP)
            {
                /* Handle a condition. */
                case OP::CONDITION:
                {
                    /* Get new OP. */
                    debit >> OP;

                    break;
                }


                /* Handle a validate. */
                case OP::VALIDATE:
                {
                    /* Seek past validate. */
                    debit.Seek(68);

                    /* Get new OP. */
                    debit >> OP;

                    break;
                }
            }

            /* Check that prev is coinbase. */
            if(OP == OP::COINBASE)
            {
                /* Check that the proof is the genesis. */
                if(hashProof != contract.Caller())
                    return debug::error(FUNCTION, "proof for coinbase needs to be genesis");

                /* Extract the coinbase public-id. */
                uint256_t hashPublic = 0;
                debit  >> hashPublic;

                /* Make sure the claimed account is the debited account. */
                if(hashPublic != contract.Caller())
                    return debug::error(FUNCTION, "cannot claim coinbase from different sigchain");

                /* Get the coinbase amount. */
                uint64_t nCoinbase = 0;
                debit >> nCoinbase;

                /* Check the claimed total versus coinbase. */
                if(nCredit != nCoinbase)
                    return debug::error(FUNCTION, "credit and coinbase mismatch");

                /* Check the identifier. */
                if(account.get<uint256_t>("token") != 0)
                    return debug::error(FUNCTION, "credit disabled for coinbase of non-native token");

                /* Seek read position to first position. */
                contract.Rewind(72, Contract::OPERATIONS);
                contract.Reset(Contract::REGISTERS);

                /* Seek read position to first position. */
                debit.Reset(Contract::OPERATIONS | Contract::REGISTERS);

                return true;
            }

            /* Check that prev is debit. */
            else if(OP != OP::DEBIT)
                return debug::error(FUNCTION, "tx claim is not a debit");

            /* Get the hashFrom */
            TAO::Register::Address hashFrom;
            debit  >> hashFrom;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashFrom))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Get the hashTo. */
            TAO::Register::Address hashTo;
            debit  >> hashTo;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTo))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Check for wildcard from (which is a flag for credit from UTXO). */
            if(hashFrom == ~uint256_t(0))
            {
                /* Check the proof as being the caller. */
                if(hashProof != hashFrom)
                    return debug::error(FUNCTION, "proof must equal from for credit");

                /* Check the proof as being the caller. */
                if(hashTo != hashAccount)
                    return debug::error(FUNCTION, "must have same address as UTXO to");

                /* Get the debit amount. */
                uint64_t nDebit = 0;
                debit >> nDebit;

                /* Check the debit amount. */
                if(nDebit != nCredit)
                    return debug::error(FUNCTION, "debit and credit value mismatch");

                /* Seek read position to first position. */
                contract.Rewind(72, Contract::OPERATIONS);
                contract.Reset(Contract::REGISTERS);

                /* Seek read position to first position. */
                debit.Reset(Contract::OPERATIONS | Contract::REGISTERS);

                return true;
            }

            /* Get the byte from pre-state. */
            nState = 0;
            debit >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register debit not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object accountFrom;
            debit >>= accountFrom;

            /* Parse the account. */
            if(!accountFrom.Parse())
                return debug::error(FUNCTION, "failed to parse account from");

            /* Check debit hashFrom is a base object account. */
            if(accountFrom.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "debit from must have a base account object");

            /* Check token identifiers. */
            if(accountFrom.get<uint256_t>("token") != account.get<uint256_t>("token"))
                return debug::error(FUNCTION, "credit can't be of different identifier");

            /* Handle one-to-one debit to credit or return to self. */
            if(hashTo == hashAccount    //regular debit to credit
            || hashTo == ~uint256_t(0)  //wildcard address (anyone can credit)
            || hashFrom == hashAccount) //return to self
            {
                /* Check the proof as being the caller. */
                if(hashProof != hashFrom)
                    return debug::error(FUNCTION, "proof must equal from for credit");

                /* Get the debit amount. */
                uint64_t nDebit = 0;
                debit >> nDebit;

                /* Special rule for credit back to self on partial payments. */
                if(hashFrom == hashAccount)
                {
                    /* Get the partial amount. */
                    uint64_t nClaimed = 0;
                    if(LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, nFlags))
                    {
                        /* Check the partial to the debit amount. */
                        if(nDebit != (nClaimed + nCredit))
                            return debug::error(FUNCTION, "debit and partial credit value mismatch");
                    }
                }

                /* Check the debit amount. */
                else if(nDebit != nCredit)
                    return debug::error(FUNCTION, "debit and credit value mismatch");

                /* Seek read position to first position. */
                contract.Rewind(72, Contract::OPERATIONS);
                contract.Reset(Contract::REGISTERS);

                /* Seek read position to first position. */
                debit.Reset(Contract::OPERATIONS | Contract::REGISTERS);

                return true;
            }

            /* Read the register to address. */
            TAO::Register::State stateTo;
            if(!LLD::Register->ReadState(hashTo, stateTo, nFlags))
                return debug::error(FUNCTION, "failed to read hash to");

            /* Credits specific to account objects. */
            if(!TAO::Register::Range(stateTo.nType))
                return debug::error(FUNCTION, "debit to is out of range");

            /* Get the state register of this register's owner. */
            TAO::Register::Object proof;
            if(!LLD::Register->ReadState(hashProof, proof, nFlags))
                return debug::error(FUNCTION, "failed to read credit proof");

            /* Parse the owner object register. */
            if(!proof.Parse())
                return debug::error(FUNCTION, "failed to parse owner object register");

            /* Compare the last timestamp update to transaction timestamp. */
            if(proof.nModified > contract.Timestamp())
                return debug::error(FUNCTION, "temporal proof is stale");

            /* Check that owner is of a valid type. */
            if(proof.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "owner object is not a token");

            /* Check the ownership of proof register. */
            if(proof.hashOwner != contract.Caller())
                return debug::error(FUNCTION, "not authorized to use this temporal proof");

            /* Get the state register of this register's owner. */
            TAO::Register::Object token;
            if(!LLD::Register->ReadState(stateTo.hashOwner, token, nFlags))
                return debug::error(FUNCTION, "credit from raw object can't be without owner");

            /* Parse the owner object register. */
            if(!token.Parse())
                return debug::error(FUNCTION, "failed to parse owner object register");

            /* Check that owner is of a valid type. */
            if(token.Standard() != TAO::Register::OBJECTS::TOKEN)
                return debug::error(FUNCTION, "owner object is not a token");

            /* Check that the token indetifier matches token identifier. */
            if(proof.get<uint256_t>("token") != token.get<uint256_t>("token"))
                return debug::error(FUNCTION, "account proof identifier not token identifier");

            /* Get the total amount of the debit. */
            uint64_t nDebit = 0;
            debit >> nDebit;

            /* Get the total tokens to be distributed. */
            uint64_t nPartial = (proof.get<uint64_t>("balance") * nDebit) / token.get<uint64_t>("supply");

            /* Check that the partial amount matches. */
            if(nCredit != nPartial)
                return debug::error(FUNCTION, "partial credit mismatch with claimed proof");

            /* Get the partial amount. */
            uint64_t nClaimed = 0;
            if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, nFlags))
                nClaimed = 0; //reset claimed on fail just in case

            /* Check the partial to the debit amount. */
            if((nClaimed + nCredit) > nDebit)
                return debug::error(FUNCTION, "credit is beyond claimable debit amount");

            /* Seek read position to first position. */
            contract.Rewind(72, Contract::OPERATIONS);
            contract.Reset(Contract::REGISTERS);

            /* Seek read position to first position. */
            debit.Reset(Contract::OPERATIONS | Contract::REGISTERS);

            return true;
        }
    }
}
