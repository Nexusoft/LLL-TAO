/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/list.h>

#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Checks the params for the existence of the "expires" field.  If supplied, a condition will be added to this contract
    *  for the expiration */
    void AddExpires(const encoding::json& params, const uint256_t& hashCaller, TAO::Operation::Contract& contract, bool fTokenizedDebit)
    {
        using namespace TAO::Operation;

        /* The optional expiration time */
        const uint64_t nExpires = ExtractInteger<uint64_t>(params, "expires", 604800);

        /* Check if there is an expiry set for the contract */
        if(nExpires > 0)
        {
            /* Add conditional statements to only allow the transaction to be credited before the expiration time. */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::NOTEQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::AND);
            contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
            contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);
            contract <= uint8_t(OP::UNGROUP);

            contract <= uint8_t(OP::OR);

            /* Add condition to prevent the sender from reversing the transaction until after the expiration time */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::AND);
            contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
            contract <= uint8_t(OP::LESSTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);
            contract <= uint8_t(OP::UNGROUP);

            /* If the contract is a debit to a tokenized asset then add an additional clause to bypass the expiration if the
            recipient is from the sending sig chain  */
            if(fTokenizedDebit)
            {
                //(caller.genesis == uint256_t("0xfffffff")
                // AND contract.operations.subdata(1, 32) != contract.operations.subdata(101, 32)
                // AND contract.timestamp + 800 > ledger.timestamp)
                //tx.submit();

                contract <= uint8_t(OP::OR);

                contract <= uint8_t(OP::GROUP);
                contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
                contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
                contract <= uint8_t(OP::CALLER::OPERATIONS)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(nExpires);
                contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);
                contract <= uint8_t(OP::UNGROUP);
            }
        }
        else
        {
            /* Add a conditional statement to check that the sender is not the receiver. */
            contract <= uint8_t(OP::GROUP);
            contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::NOTEQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
            contract <= uint8_t(OP::UNGROUP);

            /* If the contract is a debit to a tokenized asset then add an additional clause to bypass the expiration if the
            recipient is from the sending sig chain  */
            if(fTokenizedDebit)
            {
                contract <= uint8_t(OP::OR);

                contract <= uint8_t(OP::GROUP);
                contract <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashCaller;
                contract <= uint8_t(OP::AND);
                contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
                contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
                contract <= uint8_t(OP::CALLER::OPERATIONS)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof
                contract <= uint8_t(OP::UNGROUP);
            }
        }
    }


    /* Creates a void contract for the specified transaction  */
    bool AddVoid(const TAO::Operation::Contract& contract, const uint32_t nContract, TAO::Operation::Contract &voidContract)
    {
        /* The return flag indicating the contract was voided */
        bool fVoided = false;

        /* Get the transaction hash */
        const uint512_t hashTx = contract.Hash();

        /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
        contract.Reset();

        /* Get the operation nByte. */
        uint8_t nType = 0;
        contract >> nType;

        /* Check for conditional OP */
        if(nType == TAO::Operation::OP::CONDITION)
            contract >> nType;

        /* Ensure that it is a debit or transfer */
        if(nType != TAO::Operation::OP::DEBIT && nType != TAO::Operation::OP::TRANSFER)
            return false;

        /* Process crediting a debit */
        if(nType == TAO::Operation::OP::DEBIT)
        {
            /* Get the hashFrom from the debit transaction. This is the account we are going to return the credit to*/
            TAO::Register::Address hashFrom;
            contract >> hashFrom;

            /* Get the hashTo from the debit transaction. */
            TAO::Register::Address hashTo;
            contract >> hashTo;

            /* Get the amount to respond to. */
            uint64_t nAmount = 0;
            contract >> nAmount;

            /* Check to see whether there are any partial credits already claimed against the debit */
            uint64_t nClaimed = 0;
            if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL))
                nClaimed = 0;

            /* Check that there is something to be claimed */
            if(nClaimed == nAmount)
                throw Exception(-173, "Cannot void debit transaction as it has already been fully credited by all recipients");

            /* Reduce the amount to credit by the amount already claimed */
            nAmount -= nClaimed;

            /* Create the credit contract  */
            voidContract<< uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashFrom <<  hashFrom << nAmount;

            fVoided = true;
        }
        /* Process voiding a transfer */
        else if(nType == TAO::Operation::OP::TRANSFER)
        {
            /* Get the address of the asset being transferred from the transaction. */
            TAO::Register::Address hashAddress;
            contract >> hashAddress;

            /* Get the genesis hash (recipient) of the transfer*/
            uint256_t hashGenesis = 0;
            contract >> hashGenesis;

            /* Read the force transfer flag */
            uint8_t nForceFlag = 0;
            contract >> nForceFlag;

            /* Ensure this wasn't a forced transfer (which requires no Claim) */
            if(nForceFlag == TAO::Operation::TRANSFER::FORCE)
                return false;

            /* Create the claim contract  */
            voidContract << (uint8_t)TAO::Operation::OP::CLAIM << hashTx << uint32_t(nContract) << hashAddress;

            fVoided = true;
        }

        return fVoided;
    }
}/* End TAO namespace */
