/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unordered_set>

#include <TAO/API/include/list.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/transaction.h>

#include <LLD/include/global.h>
#include <LLP/include/global.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Scans a signature chain to work out all assets that it owns */
    bool ListObjects(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vObjects)
    {
        /* Get all registers owned by the sig chain */
        std::vector<TAO::Register::Address> vRegisters;
        if(!LLD::Logical->ListRegisters(hashGenesis, vRegisters))
            return false;

        /* Filter out only those that are objects */
        for(const auto& rAddress : vRegisters)
        {
            /* Check that the address is for an object */
            if(rAddress.IsObject())
                vObjects.push_back(rAddress);
        }

        return true;
    }


    /* Scans a signature chain to work out all accounts that it owns */
    bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts, bool fTokens, bool fTrust)
    {
        /* Get all registers owned by the sig chain */
        std::vector<TAO::Register::Address> vRegisters;
        if(!LLD::Logical->ListRegisters(hashGenesis, vRegisters))
            return false;

        /* Filter out only those that are accounts */
        for(const auto& rAddress : vRegisters)
        {
            /* Check that the address is for an account or token */
            if(rAddress.IsAccount() || (fTokens && rAddress.IsToken()) || (fTrust && rAddress.IsTrust()))
                vAccounts.push_back(rAddress);
        }

        return true;
    }


    /* Lists all object registers partially owned by way of tokens that the sig chain owns  */
    bool ListPartial(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters)
    {
        /* Find all token accounts owned by the caller */
        std::vector<TAO::Register::Address> vAccounts;
        if(!ListAccounts(hashGenesis, vAccounts, true, false))
            return false;

        /* Loop through all accounts to find partial account ownerships. */
        for(const auto& rAddress : vAccounts)
        {
            /* Make sure it is an account or the token itself (in case not all supply has been distributed)*/
            if(!rAddress.IsAccount() && !rAddress.IsToken())
                continue;

            /* Get the account from the register DB. */
            TAO::Register::Object tObject;
            if(!LLD::Register->ReadObject(rAddress, tObject, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-13, "Object not found");

            /* Check that this is an account or token */
            if(tObject.Base() != TAO::Register::OBJECTS::ACCOUNT)
                continue;

            /* Get the token*/
            const TAO::Register::Address hashToken =
                tObject.get<uint256_t>("token") ;

            /* NXS can't be used to tokenize an asset so if this is a NXS account we can skip it */
            if(hashToken == 0)
                continue;

            /* Get all objects owned by this token */
            std::vector<TAO::Register::Address> vTokenizedObjects;
            ListTokenizedObjects(hashGenesis, hashToken, vTokenizedObjects);

            /* Add them to the list if they are not already in there */
            for(const auto& address : vTokenizedObjects)
            {
                if(std::find(vRegisters.begin(), vRegisters.end(), address) == vRegisters.end())
                    vRegisters.push_back(address);
            }
        }

        return vRegisters.size() > 0;
    }


    /* Finds all objects that have been tokenized and therefore owned by hashToken */
    bool ListTokenizedObjects(const uint256_t& hashGenesis, const TAO::Register::Address& hashToken,
                              std::vector<TAO::Register::Address>& vObjects)
    {
        /* There is no index of the assets owned by a token.  Therefore, to determine which assets the token owns, we can scan
           through the events for the token itself to find all object transfers where the new owner is the token. */


        /* If we are in client mode we need to first check whether we own the token or not.  If we don't then we have to
           download the sig chain of the token owner so that we have access to all of the token's events. */
        if(config::fClient.load())
        {
            /* First grab the token object */
            TAO::Register::Object token;
            if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::LOOKUP))
                throw Exception(-125, "Token not found");

            /* Now check the owner.  We should already be subscribed to events for any tokens that the caller owns, so if this
               token owner is not the caller then we need to synchronize to the events of the token */
            if(token.hashOwner != hashGenesis)
            {
                if(LLP::TRITIUM_SERVER)
                {
                    std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        /* The transaction ID of the last event */
                        uint512_t hashLast = 0;
                        LLD::Ledger->ReadLastEvent(hashToken, hashLast);

                        debug::log(1, FUNCTION, "CLIENT MODE: Synchronizing events for foreign token");

                        /* Request the sig chain. */
                        debug::log(1, FUNCTION, "CLIENT MODE: Requesting LIST::NOTIFICATION for ", hashToken.SubString());

                        LLP::TritiumNode::BlockingMessage(30000, pNode.get(), LLP::TritiumNode::ACTION::LIST, uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION), hashToken, hashLast);

                        debug::log(1, FUNCTION, "CLIENT MODE: LIST::NOTIFICATION received for ", hashToken.SubString());
                    }
                    else
                        debug::error(FUNCTION, "no connections available...");
                }
            }
        }

        /* Transaction for the event. */
        TAO::Ledger::Transaction tx;

        /* Iterate all events in the sig chain */
        uint32_t nSequence = 0;
        while(LLD::Ledger->ReadEvent(hashToken, nSequence, tx))
        {
            /* Loop through transaction contracts. */
            uint32_t nContracts = tx.Size();
            for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
            {
                /* Reset the op stream */
                tx[nContract].Reset();

                /* The operation */
                uint8_t nOp;
                tx[nContract] >> nOp;

                if(nOp == TAO::Operation::OP::TRANSFER)
                {
                    /* The register address being transferred */
                    TAO::Register::Address hashRegister;
                    tx[nContract] >> hashRegister;

                    /* Get the new owner hash */
                    TAO::Register::Address hashTo;
                    tx[nContract] >> hashTo;

                    /* Read the force transfer flag */
                    uint8_t nType = 0;
                    tx[nContract] >> nType;

                    /* Ensure this was a forced transfer (which tokenized asset transfers must be) */
                    if(nType != TAO::Operation::TRANSFER::FORCE)
                        continue;

                    /* Check that the recipient of the transfer is the token */
                    if(hashToken != hashTo)
                        continue;

                    vObjects.push_back(hashRegister);
                }

                else
                    continue;
            }

            /* Iterate the sequence id forward. */
            ++nSequence;
        }

        /* Return true if we found any objects owned by the token */
        return vObjects.size() > 0;
    }
} // End TAO namespace
