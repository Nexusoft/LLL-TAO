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

    /* XXX: we don't want to use O(n) here because any nested data such as 'count' will make it O(n^2)
     */
    bool ListRegisters(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters, uint512_t hashLast)
    {
        /* LRU register cache by genesis hash.  This caches the vector of register addresses along with the last txid of the
           sig chain, so that we can determine whether any new transactions have been added, invalidating the cache.  */
        static LLD::TemplateLRU<uint256_t, std::pair<uint512_t, std::vector<TAO::Register::Address>>> cache(10);

        /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
           have been created recently but not yet included in a block*/
        if(hashLast == 0 && !LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            return false;

        /* Check the cache to see if we have already cached the registers for this sig chain and it is still valid. */
        if(cache.Has(hashGenesis))
        {
            /* The cached register list */
            std::pair<uint512_t, std::vector<TAO::Register::Address>> cacheEntry;

            /* Retrieve the cached register list from the LRU cache */
            cache.Get(hashGenesis, cacheEntry);

            /* Check that the hashlast hasn't changed */
            if(cacheEntry.first == hashLast)
            {
                /* Poplate vector to return */
                vRegisters = cacheEntry.second;

                return true;
            }

        }

        /* Keep a running list of owned and transferred registers. We use a set to store these registers because
         * we are going to be checking them frequently to see if a hash is already in the container,
         * and a set offers us near linear search time
         */
        std::unordered_set<uint256_t> vTransferred;
        std::unordered_set<uint256_t> vOwnedRegisters;

        /* The previous hash in the chain */
        uint512_t hashPrev = hashLast;

        /* Loop until genesis. */
        while(hashPrev != 0)
        {
            /* Get the transaction from disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashPrev, tx, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* In client mode it is possible to not have the full sig chain if it is still being downloaded asynchronously.*/
                if(config::fClient.load())
                    break;
                else
                    throw APIException(-108, "Failed to read transaction");
            }

            /* Set the next last. */
            hashPrev = !tx.IsFirst() ? tx.hashPrevTx : 0;

            /* Iterate through all contracts. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Get the contract output. */
                const TAO::Operation::Contract& contract = tx[nContract];

                /* Reset all streams */
                contract.Reset();

                /* Seek the contract operation stream to the position of the primitive. */
                contract.SeekToPrimitive();

                /* Deserialize the OP. */
                uint8_t nOP = 0;
                contract >> nOP;

                /* Check the current opcode. */
                switch(nOP)
                {

                    /* These are the register-based operations that prove ownership if encountered before a transfer*/
                    case TAO::Operation::OP::WRITE:
                    case TAO::Operation::OP::APPEND:
                    case TAO::Operation::OP::CREATE:
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Extract the address from the contract. */
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* for these operations, if the address is NOT in the transferred list
                           then we know that we must currently own this register */
                       if(vTransferred.find(hashAddress)    == vTransferred.end()
                       && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                       {
                           /* Add to owned set. */
                           vOwnedRegisters.insert(hashAddress);

                           /* Add to return vector. */
                           vRegisters.push_back(hashAddress);
                       }

                        break;
                    }


                    /* Check for credits here. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Seek past irrelevant data. */
                        contract.Seek(68);

                        /* The account that is being credited. */
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* If we find a credit before a transfer transaction for this register then
                           we can know for certain that we must own it */
                       if(vTransferred.find(hashAddress)    == vTransferred.end()
                       && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                       {
                           /* Add to owned set. */
                           vOwnedRegisters.insert(hashAddress);

                           /* Add to return vector. */
                           vRegisters.push_back(hashAddress);
                       }

                        break;

                    }


                    /* Check for a transfer here. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Extract the address from the contract. */
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        TAO::Register::Address hashTransfer;
                        contract >> hashTransfer;

                        /* Read the force transfer flag */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* IF this is not a forced transfer, we need to retrieve the object so we can see whether it has been
                           claimed or not.  If it has not been claimed then we ignore the transfer operation and still show
                           it as ours.  However we need to skip this check in light mode because we will not have the
                           register state available in order to determine if it has been claimed or not */
                        if(nType != TAO::Operation::TRANSFER::FORCE && !config::fClient.load())
                        {
                            /* Retrieve the object so we can see whether it has been claimed or not */
                            TAO::Register::Object object;
                            if(!LLD::Register->ReadState(hashAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
                                throw APIException(-104, "Object not found");

                            /* If we are transferring to someone else but it has not yet been claimed then we ignore the
                               transfer and still show it as ours */
                            if(object.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
                            {
                                /* Ensure it is the caller that made the most recent transfer */
                                uint256_t hashPrevOwner = hashGenesis;;

                                /* Set the SYSTEM byte so that we can compare the prev owner */
                                hashPrevOwner.SetType(TAO::Ledger::GENESIS::SYSTEM);

                                /* If we transferred it  */
                                if(object.hashOwner == hashPrevOwner)
                                    break;

                            }
                                break;
                        }


                        /* If we find a TRANSFER then we can know for certain that we no longer own it */
                        if(vTransferred.find(hashAddress)    == vTransferred.end())
                            vTransferred.insert(hashAddress);

                        break;
                    }


                    /* Check for claims here. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Seek past irrelevant data. */
                        contract.Seek(68);

                        /* Extract the address from the contract. */
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* If we find a CLAIM transaction before a TRANSFER, then we know that we must currently own this register */
                        if(vTransferred.find(hashAddress)    == vTransferred.end()
                        && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                        {
                            /* Add to owned set. */
                            vOwnedRegisters.insert(hashAddress);

                            /* Add to return vector. */
                            vRegisters.push_back(hashAddress);
                        }

                        break;
                    }

                    default:
                        continue;
                }
            }
        }

        /* Add the register list to the LRU cache */
        cache.Put(hashGenesis, std::make_pair(hashLast, vRegisters));

        return true;
    }


    /* Scans a signature chain to work out all assets that it owns */
    bool ListObjects(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vObjects)
    {
        /* Get all registers owned by the sig chain */
        std::vector<TAO::Register::Address> vRegisters;
        ListRegisters(hashGenesis, vRegisters);

        /* Filter out only those that are objects */
        for(const auto& address : vRegisters)
        {
            /* Check that the address is for an object */
            if(address.IsObject())
                vObjects.push_back(address);
        }

        return true;
    }


    /* Scans a signature chain to work out all accounts that it owns */
    bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts, bool fTokens, bool fTrust)
    {
        /* Get all registers owned by the sig chain */
        std::vector<TAO::Register::Address> vRegisters;
        ListRegisters(hashGenesis, vRegisters);

        /* Filter out only those that are accounts */
        for(const auto& address : vRegisters)
        {
            /* Check that the address is for an account or token */
            if(address.IsAccount() || (fTokens && address.IsToken()) || (fTrust && address.IsTrust()))
                vAccounts.push_back(address);
        }

        return true;
    }


    /* Lists all object registers partially owned by way of tokens that the sig chain owns  */
    bool ListPartial(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters)
    {
        /* Find all token accounts owned by the caller */
        std::vector<TAO::Register::Address> vAccounts;
        ListAccounts(hashGenesis, vAccounts, true, false);

        for(const auto& hashAccount : vAccounts)
        {
            /* Make sure it is an account or the token itself (in case not all supply has been distributed)*/
            if(!hashAccount.IsAccount() && !hashAccount.IsToken())
                continue;

            /* Get the account from the register DB. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-13, "Object not found");

            /* Check that this is a non-standard object type so that we can parse it and check the type*/
            if(object.nType != TAO::Register::REGISTER::OBJECT)
                continue;

            /* parse object so that the data fields can be accessed */
            if(!object.Parse())
                throw APIException(-36, "Failed to parse object register");

            /* Check that this is an account or token */
            if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                continue;

            /* Get the token*/
            TAO::Register::Address hashToken = object.get<uint256_t>("token") ;

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
                throw APIException(-125, "Token not found");

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
