/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <Legacy/include/evaluate.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/conditions.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/notifications.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/names.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/credentials.h>

#include <Util/include/convert.h>
#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Build a credential set that engages sigchain or modifies its authentication data. This is done not logged in. */
    bool BuildCredentials(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                          const SecureString& strPIN, const uint8_t nKeyType, TAO::Ledger::Transaction &tx)
    {
        /* Create the transaction. */
        if(!TAO::Ledger::CreateTransaction(pCredentials, strPIN, tx, nKeyType))
            return false;

        /* We need at least one contract to change our next hash. */
        tx << BuildCrypto(pCredentials, strPIN, nKeyType);

        /* Now set the new credentials */
        tx.NextHash(pCredentials->Generate(tx.nSequence + 1, strPIN));

        /* Add our network fee if applicable. */
        AddFee(tx);

        /* Execute the operations layer. */
        if(!tx.Build())
            return false;

        /* Double check our next hash if -safemode enabled. */
        if(config::GetBoolArg("-safemode", false))
        {
            /* Re-calculate our next hash if safemode forcing not to use cache. */
            const uint256_t hashNext =
                TAO::Ledger::Transaction::NextHash(pCredentials->Generate(tx.nSequence + 1, strPIN), tx.nNextType);

            /* Check that this next hash is what we are expecting. */
            if(tx.hashNext != hashNext)
                return false;
        }

        return true;
    }


    /* Build a response object for a transaction that was built. */
    encoding::json BuildResponse(const encoding::json& jParams, const TAO::Register::Address& hashRegister,
                             const std::vector<TAO::Operation::Contract>& vContracts)
    {
        /* Build a JSON response object. */
        encoding::json jRet;
        jRet["success"] = true; //just a little response for if using -autotx
        jRet["address"] = hashRegister.ToString();

        /* Handle passing txid if not in -autotx mode. */
        const std::vector<uint512_t> vHashes = BuildAndAccept(jParams, vContracts, TAO::Ledger::PinUnlock::TRANSACTIONS);
        if(!vHashes.empty())
        {
            /* Check for single item. */
            if(vHashes.size() == 1)
                jRet["txid"] = vHashes[0].ToString();
            else
            {
                /* Build an array of hashes in response. */
                encoding::json jHashes =
                    encoding::json::array();

                /* Iterate our hashes and add to json array. */
                for(const auto& rHash : vHashes)
                    jHashes.push_back(rHash.ToString());

                /* Add to return value as array now. */
                jRet["txid"] = jHashes;
            }
        }

        return jRet;
    }


    /* Build a response object for a transaction that was built. */
    encoding::json BuildResponse(const encoding::json& jParams, const std::vector<TAO::Operation::Contract>& vContracts)
    {
        /* Build a JSON response object. */
        encoding::json jRet;
        jRet["success"] = true; //just a little response for if using -autotx

        /* Handle passing txid if not in -autotx mode. */
        const std::vector<uint512_t> vHashes = BuildAndAccept(jParams, vContracts, TAO::Ledger::PinUnlock::TRANSACTIONS);
        if(!vHashes.empty())
        {
            /* Check for single item. */
            if(vHashes.size() == 1)
                jRet["txid"] = vHashes[0].ToString();
            else
            {
                /* Build an array of hashes in response. */
                encoding::json jHashes =
                    encoding::json::array();

                /* Iterate our hashes and add to json array. */
                for(const auto& rHash : vHashes)
                    jHashes.push_back(rHash.ToString());

                /* Add to return value as array now. */
                jRet["txid"] = jHashes;
            }
        }

        return jRet;
    }


    /* Build a response object that indicates a command succeeded. */
    encoding::json BuildResponse(const encoding::json& jResponse)
    {
        /* Build our response from given parameters. */
        encoding::json jRet = jResponse;
        jRet["success"] = true; //little response to indicate success

        return jRet;
    }


    namespace
    {
        /* Records SessionDB links that Index() may update. Index() writes the previous
         * or first link, then the tx record, then the last link, and returns false after
         * any individual write. Transaction::Delete() uses the same non-atomic sequence,
         * so a failed Index() is undone by restoring this snapshot instead. */
        struct SessionIndexSnapshot
        {
            /* True when a record existed but could not be read. Restoring would guess. */
            bool        fIncomplete = false;

            bool        fHavePrev   = false;
            Transaction txPrev;

            bool        fHaveFirst  = false;
            uint512_t   hashFirst   = 0;

            /* Confirmed absent, as opposed to an unreadable first index. */
            bool        fAbsentFirst = false;

            bool        fHaveLast   = false;
            uint512_t   hashLast    = 0;

            /* Confirmed absent, as opposed to an unreadable last index. */
            bool        fAbsentLast = false;

            bool        fHaveTx     = false;
            Transaction txStored;

            /* Confirmed absent, as opposed to an unreadable tx record. */
            bool        fAbsentTx   = false;
        };


        /* Read the SessionDB records Index() can mutate. Confirmed absence is not an error. */
        bool CaptureSessionIndex(const Transaction& tIndex, const uint512_t& hashTx, SessionIndexSnapshot& snap)
        {
            if(!LLD::Sessions)
                return false;

            if(!tIndex.IsFirst())
            {
                if(LLD::Sessions->HasTx(tIndex.hashPrevTx))
                {
                    snap.fHavePrev = LLD::Sessions->ReadTx(tIndex.hashPrevTx, snap.txPrev);
                    if(!snap.fHavePrev)
                        snap.fIncomplete = true;
                }
            }
            else if(LLD::Sessions->HasFirst(tIndex.hashGenesis))
            {
                snap.fHaveFirst = LLD::Sessions->ReadFirst(tIndex.hashGenesis, snap.hashFirst);
                if(!snap.fHaveFirst)
                    snap.fIncomplete = true;
            }
            else
                snap.fAbsentFirst = true;

            /* ReadLast() is false for both absence and an unreadable value.
             * HasLast() is the existence check, matching HasFirst()/HasTx(). */
            if(LLD::Sessions->HasLast(tIndex.hashGenesis))
            {
                snap.fHaveLast = LLD::Sessions->ReadLast(tIndex.hashGenesis, snap.hashLast);
                if(!snap.fHaveLast)
                    snap.fIncomplete = true;
            }
            else
                snap.fAbsentLast = true;

            if(LLD::Sessions->HasTx(hashTx))
            {
                snap.fHaveTx = LLD::Sessions->ReadTx(hashTx, snap.txStored);
                if(!snap.fHaveTx)
                    snap.fIncomplete = true;
            }
            else
                snap.fAbsentTx = true;

            return true;
        }


        /* Restore records captured before Index(). Only erase a link that was confirmed
         * absent and now points at this tx. An incomplete snapshot cannot be restored.
         * A failed read of a key that still exists is unverifiable (Read returns false
         * for both absence and an unreadable value) and must not be reported as success. */
        bool RestoreSessionIndex(const Transaction& tIndex, const uint512_t& hashTx, const SessionIndexSnapshot& snap)
        {
            if(!LLD::Sessions)
                return debug::error(FUNCTION, "session database unavailable");

            if(snap.fIncomplete)
                return debug::error(FUNCTION, "incomplete session index snapshot for ", VARIABLE(hashTx.SubString()));

            if(!tIndex.IsFirst())
            {
                if(snap.fHavePrev && !LLD::Sessions->WriteTx(tIndex.hashPrevTx, snap.txPrev))
                    return debug::error(FUNCTION, "failed to restore previous ", VARIABLE(tIndex.hashPrevTx.SubString()));
            }
            else if(snap.fHaveFirst)
            {
                if(!LLD::Sessions->WriteFirst(tIndex.hashGenesis, snap.hashFirst))
                    return debug::error(FUNCTION, "failed to restore first index for ", VARIABLE(tIndex.hashGenesis.SubString()));
            }
            else if(snap.fAbsentFirst)
            {
                uint512_t hashNow = 0;
                const bool fReadFirst = LLD::Sessions->ReadFirst(tIndex.hashGenesis, hashNow);
                if(!fReadFirst)
                {
                    /* HasFirst is true when the key exists even if the value cannot
                     * be deserialized. That is not confirmed absence. */
                    if(LLD::Sessions->HasFirst(tIndex.hashGenesis))
                        return debug::error(FUNCTION, "unreadable first index for ", VARIABLE(tIndex.hashGenesis.SubString()));
                }
                else if(hashNow == hashTx && !LLD::Sessions->EraseFirst(tIndex.hashGenesis))
                    return debug::error(FUNCTION, "failed to erase partial first index for ", VARIABLE(tIndex.hashGenesis.SubString()));
            }

            /* Index() does not publish the last link when it returns false. Restore a
             * captured value, but do not treat an existing unreadable last link as
             * absence — that would report success and drop the accepted chain. */
            if(snap.fHaveLast)
            {
                if(!LLD::Sessions->WriteLast(tIndex.hashGenesis, snap.hashLast))
                    return debug::error(FUNCTION, "failed to restore last index for ", VARIABLE(tIndex.hashGenesis.SubString()));
            }
            else if(!snap.fAbsentLast || LLD::Sessions->HasLast(tIndex.hashGenesis))
                return debug::error(FUNCTION, "unreadable last index for ", VARIABLE(tIndex.hashGenesis.SubString()));

            if(snap.fHaveTx)
            {
                if(!LLD::Sessions->WriteTx(hashTx, snap.txStored))
                    return debug::error(FUNCTION, "failed to restore ", VARIABLE(hashTx.SubString()));
            }
            else if(snap.fAbsentTx && LLD::Sessions->HasTx(hashTx))
            {
                /* HasTx alone is not a readable restore. An existing record that
                 * cannot be read must fail closed so the accepted tx is kept. */
                Transaction txNow;
                if(!LLD::Sessions->ReadTx(hashTx, txNow))
                    return debug::error(FUNCTION, "unreadable partial ", VARIABLE(hashTx.SubString()));

                if(!LLD::Sessions->EraseTx(hashTx))
                    return debug::error(FUNCTION, "failed to erase partial ", VARIABLE(hashTx.SubString()));
            }

            return true;
        }
    }


    /* Builds a transaction based on a list of contracts, to be deployed as a single tx or batched. */
    std::vector<uint512_t> BuildAndAccept(const encoding::json& jParams, const std::vector<TAO::Operation::Contract>& vContracts,
                                          const uint8_t nUnlockedActions)
    {
        /* Get the calling genesis-id. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Handle auto-tx feature. */
        if(config::GetBoolArg("-autotx", false) && !(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS))
        {
            /* Get the current session-id. */
            const uint256_t hashSession =
                Authentication::ExtractSession(jParams);

            /* Check we have an active session established. */
            if(Notifications::mapDispatch->count(hashSession))
            {
                /* Make a copy of contracts to sanitize. */
                std::vector<TAO::Operation::Contract> vSanitize =
                    std::vector<TAO::Operation::Contract>(vContracts.begin(), vContracts.end());

                /* Sanitize our contract here to make sure we build a valid transaction. */
                std::map<uint256_t, TAO::Register::State> mapStates;
                for(auto& rContract : vSanitize)
                {
                    /* Bind our contract now to a timestamp and caller. */
                    rContract.Bind(runtime::unifiedtimestamp(), hashGenesis);

                    /* Sanitize the contract. */
                    if(!Notifications::SanitizeContract(rContract, mapStates, true))
                        throw Exception(-32, "-autotx invalid contract: ", debug::strLastError);
                }

                /* Push to our notifications queue. */
                std::vector<TAO::Operation::Contract>& rQueue =
                    std::ref(Notifications::mapDispatch->at(hashSession));

                /* Push the new contracts to our dispatch queue. */
                rQueue.insert(rQueue.end(), vContracts.begin(), vContracts.end());
                return std::vector<uint512_t>();
            } //otherwise we just proceed like normal
        }

        /* Check for sigchain maturity on mainnet. */
        if(!config::fHybrid.load())
        {
            /* Check if sigchain is mature. */
            if(!CheckMature(hashGenesis))
                throw Exception(-202, "Signature chain not mature. Please wait for coinbase/coinstake maturity");

            /* Make sure our sigchain is in sync with our history. */
            if(config::fClient.load())
            {
                Indexing::BroadcastUnconfirmed(hashGenesis); //if we still have queud transactions, re-broadcast before creating
                Indexing::DownloadSigchain(hashGenesis); //sanity check to make sure we don't orphan another sigchain branch
            }
        }

        /* The new key scheme */
        const uint8_t nScheme =
            ExtractScheme(jParams, "brainpool, falcon");

        /* The PIN to be used for this API call */
        SecureString strPIN;

        /* Unlock grabbing the pin, while holding a new authentication lock */
        RECURSIVE(Authentication::Unlock(jParams, strPIN, nUnlockedActions));

        /* Get an instance of our credentials. */
        const auto& pCredentials =
            Authentication::Credentials(jParams);

        /* Build our return value list. */
        std::vector<uint512_t> vHashes;

        /* Get our limits from commandline. */
        const uint32_t nLimits =
            std::min(uint32_t(config::GetArg("-maxcontracts", 99)), uint32_t(99));

        /* Build our transaction if there are contracts. */
        uint64_t nIndex = 0, nTotal = 0;
        while(nIndex < vContracts.size())
        {
            /* Check for shutdown. */
            if(config::fShutdown.load())
                break;

            /* Check that our last transaction was more than 10 seconds ago. */
            if((nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS) && !CheckTimespan(hashGenesis, config::fHybrid.load() ? 1 : 10))
            {
                runtime::sleep(500);
                continue;
            }

            /* Build in batches of at most 99 user contracts. AddFee may append
             * one OP::FEE. If AddFee adds nothing and this batch already holds
             * 99 contracts, one more user contract is appended below, so a
             * fee-free batch may hold 100. See docs/architecture/BUILD_CPP.md. */
            const uint64_t nBatchStart = nIndex;
            std::vector<TAO::Operation::Contract> vBuild;
            for( ; vBuild.size() < nLimits && nIndex < vContracts.size(); ++nIndex)
                vBuild.emplace_back(std::move(vContracts[nIndex]));

            /* Check for available contracts. */
            if(vBuild.empty())
                break;

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(pCredentials, strPIN, tx, nScheme))
                throw Exception(-17, "Failed to create transaction");

            /* Add the contracts. */
            for(const auto& rContract : vBuild)
                tx << rContract;

            /* Add the contract fees. */
            /* Fee-free only: a 99-contract batch with more user contracts
             * waiting grows to 100. Fee-bearing batches stop at 99 user
             * contracts plus the fee. */
            if(!AddFee(tx) && nIndex < vContracts.size() && tx.Size() == 99)
                tx << vContracts[nIndex++];

            /* Execute the operations layer. */
            if(!tx.Build())
                throw Exception(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(pCredentials->Generate(tx.nSequence, strPIN)))
                throw Exception(-31, "Ledger failed to sign transaction");

            /* Double check our next hash if -safemode enabled. */
            if(config::GetBoolArg("-safemode", false))
            {
                /* Re-calculate our next hash if safemode forcing not to use cache. */
                const uint256_t hashNext =
                    TAO::Ledger::Transaction::NextHash(pCredentials->Generate(tx.nSequence + 1, strPIN, false), tx.nNextType);

                /* Check that this next hash is what we are expecting. */
                if(tx.hashNext != hashNext)
                    throw Exception(-67, "-safemode next hash mismatch, broadcast terminated");
            }

            /* Check that account is unlocked and no pin has been supplied. */
            if(!Authentication::Unlocked(nUnlockedActions, jParams) && !CheckParameter(jParams, "pin", "string, number"))
                break;

            const bool fActiveSession = Authentication::Active(tx.hashGenesis);
            const uint512_t hashTx = tx.GetHash();

            /* Check() and Notifications::SanitizeUnconfirmed() can disconnect
             * this tx and delete its API indexes as soon as Accept() drops the
             * mempool mutex. Hold that lock across accept, snapshot, index, and
             * rollback so those steps cannot interleave. SanitizeUnconfirmed()
             * takes the same mutex before collecting its unconfirmed tail and
             * holds it through Disconnect/Delete/Remove. */
            std::unique_lock<std::recursive_mutex> POOL_LOCK(TAO::Ledger::mempool.MUTEX, std::defer_lock);
            if(fActiveSession)
                POOL_LOCK.lock();

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");

            /* Check that we have an active session to index for. */
            if(fActiveSession)
            {
                /* Accept() can return true without inserting mapLedger (orphan
                 * predecessor). Do not index or snapshot a tx Check() can already
                 * have removed, and do not index one that was never live. */
                if(!TAO::Ledger::mempool.InPool(hashTx))
                    throw Exception(-32, "Transaction was not admitted to the live mempool");

                /* Build an API transaction. */
                TAO::API::Transaction tIndex =
                    TAO::API::Transaction(tx);

                /* IndexSigchain() and BuildIndexes() mutate the same SessionDB
                 * links without mempool.MUTEX. Hold the per-genesis index lock
                 * across snapshot, Index(), and restore. mempool.MUTEX is
                 * already held; never take these locks in the other order.
                 *
                 * The snapshot is taken after Accept() returns. IndexSigchain()
                 * (CLIENT_MUTEX, no pool lock) can write this genesis in that
                 * gap, so a later Index() can overwrite a confirmed tail, and a
                 * failed Index() restores the post-race snapshot rather than
                 * the pre-accept links. Do not move IndexLock ahead of
                 * mempool.MUTEX to close it. Window W1 in
                 * docs/architecture/BUILD_CPP.md. */
                std::unique_lock<std::recursive_mutex> INDEX_LOCK(TAO::API::Transaction::IndexLock(tx.hashGenesis));

                /* Capture links before Index() so a partial write can be restored. */
                SessionIndexSnapshot snapIndex;
                const bool fCapturedIndex = CaptureSessionIndex(tIndex, hashTx, snapIndex);

                /* Index the transaction to the database. */
                if(!tIndex.Index(hashTx))
                {
                    const std::string strIndexError = debug::GetLastError();

                    /* Delete() commits each SessionDB write as it goes and returns
                     * false after any one of them fails. Restore the pre-index
                     * snapshot instead, and retry that restore. If it still fails,
                     * keep the accepted mempool entry rather than dropping a tx
                     * whose session links may be only partly undone. */
                    bool fRestored = false;
                    std::string strRestoreError;
                    if(!fCapturedIndex)
                        strRestoreError = "failed to capture session index";
                    else if(snapIndex.fIncomplete)
                        strRestoreError = "incomplete session index snapshot";
                    else
                    {
                        for(uint32_t nTry = 0; nTry < 3 && !fRestored; ++nTry)
                            fRestored = RestoreSessionIndex(tIndex, hashTx, snapIndex);

                        if(!fRestored)
                            strRestoreError = debug::GetLastError();
                    }

                    if(!fRestored)
                    {
                        throw Exception(-32, "Failed to roll back session index for ", hashTx.SubString(),
                                        "; leaving accepted mempool entry in place: ", strRestoreError,
                                        " (index error: ", strIndexError, ")");
                    }

                    /* A failed restore attempt must not leak into the index error below. */
                    debug::strLastError.clear();

                    /* Accept() already committed Connect(FLAGS::MEMPOOL) for this tx
                     * and for any orphan/conflict tail it admitted. Remove() only
                     * erases pool maps, so roll the whole claimed chain back under
                     * one MEMPOOL memory transaction before dropping the entries.
                     * Disconnect descendants first so their predecessor is still live. */
                    std::vector<TAO::Ledger::Transaction> vDescendants;
                    TAO::Ledger::mempool.ClaimedDescendants(hashTx, vDescendants);

                    if(!LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY))
                        throw Exception(-32, "Failed to begin mempool rollback for ", hashTx.SubString());

                    bool fRolledBack = false;
                    uint512_t hashFailed = hashTx;
                    try
                    {
                        fRolledBack = true;
                        for(auto it = vDescendants.rbegin(); it != vDescendants.rend(); ++it)
                        {
                            hashFailed = it->GetHash();
                            if(!it->Disconnect(TAO::Ledger::FLAGS::MEMPOOL))
                            {
                                fRolledBack = false;
                                break;
                            }
                        }

                        if(fRolledBack)
                        {
                            hashFailed = hashTx;
                            fRolledBack = tx.Disconnect(TAO::Ledger::FLAGS::MEMPOOL);
                        }
                    }
                    catch(const std::exception& e)
                    {
                        debug::error(FUNCTION, "mempool rollback threw for ", hashFailed.SubString(), ": ", e.what());
                        fRolledBack = false;
                    }
                    catch(...)
                    {
                        debug::error(FUNCTION, "mempool rollback threw for ", hashFailed.SubString());
                        fRolledBack = false;
                    }

                    if(!fRolledBack)
                    {
                        const std::string strRollbackError = debug::GetLastError();
                        LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY);

                        /* Leave the pool entries in place: the overlay is still live. */
                        throw Exception(-32, "Failed to roll back mempool transaction ", hashFailed.SubString(),
                                        " after index failure: ", strRollbackError,
                                        " (index error: ", strIndexError, ")");
                    }

                    if(!LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY))
                    {
                        LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY);
                        debug::error(FUNCTION, "failed to commit mempool rollback ", hashTx.SubString());
                        throw Exception(-32, "Failed to commit mempool rollback for ", hashTx.SubString(),
                                        " after index failure: ", strIndexError);
                    }

                    for(auto it = vDescendants.rbegin(); it != vDescendants.rend(); ++it)
                    {
                        const uint512_t hashDesc = it->GetHash();
                        if(!TAO::Ledger::mempool.Remove(hashDesc))
                            debug::warning(FUNCTION, "failed to remove rolled-back descendant ", VARIABLE(hashDesc.SubString()));
                    }

                    if(!TAO::Ledger::mempool.Remove(hashTx))
                        debug::warning(FUNCTION, "failed to remove rolled-back tx ", VARIABLE(hashTx.SubString()));

                    throw Exception(-32, "Failed to index accepted transaction: ", strIndexError);
                }

                /* Debug output for notifications. */
                if(nUnlockedActions & TAO::Ledger::PinUnlock::NOTIFICATIONS)
                {
                    const uint64_t nCompleted = nTotal + (nIndex - nBatchStart);
                    debug::log(0, FUNCTION, "Indexed ", hashTx.SubString(), " completed ", nCompleted, "/", vContracts.size(), " (", (nCompleted * 100.0) / vContracts.size(), "%) contracts");
                }
            }

            nTotal += (nIndex - nBatchStart);

            /* Add our hashes to a return vector. */
            vHashes.push_back(hashTx);
        }

        return vHashes;
    }


    /* Calculates the required fee for the transaction and adds the OP::FEE contract to the transaction if necessary.
     *  If a specified fee account is not specified, the method will lookup the "default" NXS account and use this account
     *  to pay the fees.  An exception will be thrownIf there are insufficient funds to pay the fee. */
    bool AddFee(TAO::Ledger::Transaction& tx, const TAO::Register::Address& hashFeeAccount)
    {
        /* First we need to ensure that the transaction is built so that the contracts have their pre states */
        tx.Build();

        /* Obtain the transaction cost */
        uint64_t nCost = tx.Cost();

        /* If a fee needs to be applied then add it */
        if(nCost > 0)
        {
            /* The register adddress of the account to deduct fees from */
            TAO::Register::Address hashRegister;

            /* If the caller has specified a fee account to use then use this */
            if(hashFeeAccount.IsValid() && hashFeeAccount.IsAccount())
            {
                hashRegister = hashFeeAccount;
            }
            else
            {
                /* Otherwise we need to look up the default fee account */
                TAO::Register::Object objectDefaultName;
                if(!TAO::Register::GetNameRegister(tx.hashGenesis, std::string("default"), objectDefaultName))
                    throw TAO::API::Exception(-163, "Could not retrieve default NXS account to debit fees.");

                /* Get the address of the default account */
                hashRegister = objectDefaultName.get<uint256_t>("address");
            }

            /* Retrieve the account */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw TAO::API::Exception(-13, "Object not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw TAO::API::Exception(-14, "Object failed to parse");

            /* Get the object standard. */
            const uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT)
                throw TAO::API::Exception(-65, "Object is not an account");

            /* Check the account is a NXS account */
            if(object.get<uint256_t>("token") != TOKEN::NXS)
                throw TAO::API::Exception(-164, "Fee account is not a NXS account.");

            /* Get the account balance */
            const uint64_t nCurrentBalance = object.get<uint64_t>("balance");

            /* Check that there is enough balance to pay the fee */
            if(nCurrentBalance < nCost)
                throw TAO::API::Exception(-214, "Insufficient funds to pay fee");

            /* Add the fee contract */
            uint32_t nContractPos = tx.Size();
            tx[nContractPos] << uint8_t(TAO::Operation::OP::FEE) << hashRegister << nCost;

            return true;
        }

        return false;
    }


    /* Builds a credit contract based on given txid. */
    bool BuildContracts(const encoding::json& jParams, const uint512_t& hashTx, std::vector<TAO::Operation::Contract> &vContracts,
                        const build_function_t& xBuild)
    {
        /* Track contracts added to compare values for return */
        const uint32_t nContracts = vContracts.size();

        /* Check our transaction types. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Read the debit transaction that is being credited. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx))
                throw Exception(-40, "Previous transaction not found.");

            /* Loop through all transactions. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                /* Build the credit contract within the contract loop. */
                if(!xBuild(jParams, nContract, rContract, vContracts))
                    continue;
            }
        }
        else if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Read the debit transaction that is being credited. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx))
                throw Exception(-40, "Previous transaction not found.");

            /* Iterate through all TxOut's in the legacy transaction to see which are sends to a sig chain  */
            for(uint32_t nContract = 0; nContract < tx.vout.size(); ++nContract)
            {
                /* Check that this output is a valid register script. */
                if(!Legacy::VerifyRegister(tx.vout[nContract].scriptPubKey))
                    continue;

                /* Build our contract we will operate on from the legacy output. */
                const TAO::Operation::Contract tContract = TAO::Operation::Contract(tx, nContract);

                /* Build the credit contract within the contract loop. */
                if(!xBuild(jParams, nContract, tContract, vContracts))
                    continue;
            }
        }
        else
            throw Exception(-41, "Invalid transaction-id");

        return (vContracts.size() > nContracts);
    }


    /* Builds a credit contract based on given contract and related parameters. */
    bool BuildCredit(const encoding::json& jParams, const uint32_t nContract, const TAO::Operation::Contract& rDebit,
        std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Extract some parameters from input data. */
        const TAO::Register::Address addrCredit =
            ExtractAddress(jParams, "", config::GetArg("-creditacct", "default"));

        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Copy our txid out of the contract. */
        const uint512_t hashTx = rDebit.Hash();

        /* Reset the contract to the position of the primitive. */
        const uint8_t nPrimitive = rDebit.Primitive();
        rDebit.SeekToPrimitive(false); //false to skip our primitive

        /* Check for coinbase credits first. */
        if(nPrimitive == TAO::Operation::OP::COINBASE)
        {
            /* Enforce address resolution for coinbase. */
            if(addrCredit == TAO::API::ADDRESS_NONE)
                throw Exception(-35, "Invalid parameter [name], expecting [exists]");

            /* Get the genesisHash of the user who mined the coinbase*/
            uint256_t hashMiner = 0;
            rDebit >> hashMiner;

            /* Check that the coinbase was mined by the caller */
            if(hashMiner != hashGenesis)
                return false;

            /* Get the amount from the coinbase transaction. */
            uint64_t nAmount = 0;
            rDebit >> nAmount;

            /* Now lets check our expected types match. */
            if(!CheckStandard(jParams, addrCredit))
                return false;

            /* if we passed all of these checks then insert the credit contract into the tx */
            TAO::Operation::Contract tContract;
            tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
            tContract << addrCredit << hashGenesis << nAmount;

            /* Push to our contract queue. */
            vContracts.push_back(tContract);

            return true;
        }

        /* Next check for debit credits. */
        else if(nPrimitive == TAO::Operation::OP::DEBIT)
        {
            /* Get the source account the debit transaction. */
            TAO::Register::Address addrSource;
            rDebit >> addrSource;

            /* Get the recipient from the debit transaction. */
            TAO::Register::Address addrRecipient;
            rDebit >> addrRecipient;

            /* Get the amount to respond to. */
            uint64_t nAmount = 0;
            rDebit >> nAmount;

            /* Check if the credit is already claimed. */
            if(LLD::Ledger->HasProof(addrSource, hashTx, nContract, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Special debug output if we indexed our proofs. */
                if(config::fIndexProofs.load())
                {
                    /* Read the transction by indexed proofs. */
                    TAO::Ledger::Transaction tx;
                    if(LLD::Ledger->ReadTx(addrSource, hashTx, nContract, tx))
                        throw Exception(-44, "credit is already claimed by txid ", tx.GetHash().ToString());
                }

                throw Exception(-44, "credit is already claimed ", addrSource.SubString(), " txid ", hashTx.SubString(), " contract ", nContract);
            }

            /* Check for a legacy output debit. */
            if(addrSource == TAO::Register::WILDCARD_ADDRESS)
            {
                /* Read our crediting account. */
                TAO::Register::Object oCredit;
                if(!LLD::Register->ReadObject(addrRecipient, oCredit, TAO::Ledger::FLAGS::LOOKUP))
                    return false;

                /* Let's check our credit account is correct token. */
                if(oCredit.get<uint256_t>("token") != 0)
                    throw Exception(-59, "Account to credit is not a NXS account");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oCredit))
                    throw Exception(-49, "Unsupported type for name/address");

                /* If we passed these checks then insert the credit contract into the tx */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrRecipient << addrSource << nAmount;

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }

            /* Check for wildcard for conditional contract (OP::VALIDATE). */
            if(addrRecipient == TAO::Register::WILDCARD_ADDRESS)
            {
                /* Enforce address resolution for wildcard claim. */
                if(addrCredit == TAO::API::ADDRESS_NONE)
                    throw Exception(-35, "Invalid parameter [name], expecting [exists]");

                /* Check for conditions. */
                if(!rDebit.Empty(TAO::Operation::Contract::CONDITIONS)) //XXX: unit tests for this scope in finance unit tests
                {
                    /* Check for condition. */
                    if(rDebit.Operations()[0] == TAO::Operation::OP::CONDITION)
                    {
                        /* Read the contract database. */
                        uint256_t hashValidator = 0;
                        if(LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator))
                        {
                            /* Check that the caller is the claimant. */
                            if(hashValidator != hashGenesis)
                                return false;
                        }
                    }
                }

                /* Retrieve the account we are debiting from */
                TAO::Register::Object oSource;
                if(!LLD::Register->ReadObject(addrSource, oSource, TAO::Ledger::FLAGS::LOOKUP))
                    return false;

                /* Read our crediting account. */
                TAO::Register::Object oCredit;
                if(!LLD::Register->ReadObject(addrCredit, oCredit, TAO::Ledger::FLAGS::LOOKUP))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Let's check our credit account is correct token. */
                if(oSource.get<uint256_t>("token") != oCredit.get<uint256_t>("token"))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oCredit))
                    throw Exception(-49, "Unsupported type for name/address");

                /* If we passed these checks then insert the credit contract into the tx */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrCredit << addrSource << nAmount;

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }

            /* Get the token / account object that the debit was made to. */
            TAO::Register::Object oRecipient;
            if(!LLD::Register->ReadObject(addrRecipient, oRecipient, TAO::Ledger::FLAGS::LOOKUP))
                return false;

            /* Check for standard credit to account. */
            const uint8_t nStandardBase = oRecipient.Base();
            if(nStandardBase == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* Check that the debit was made to an account that we don't own, we are checking here to reverse payments */
                if(oRecipient.hashOwner != hashGenesis)
                {
                    /* Retrieve the account we are debiting from */
                    TAO::Register::Object oSource;
                    if(!LLD::Register->ReadObject(addrSource, oSource, TAO::Ledger::FLAGS::LOOKUP))
                        return false;

                    /* Check if we are the sender. */
                    if(oSource.hashOwner != hashGenesis)
                        return false;

                    /* Otherwise build a credit back to self. */
                    TAO::Operation::Contract tContract;
                    tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                    tContract << addrSource << addrSource << nAmount; //we use addrRecipient from our debit contract instead of addrCredit

                    /* Push to our contract queue. */
                    vContracts.push_back(tContract);

                    return true;
                }

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oRecipient))
                    throw Exception(-49, "Unsupported type for name/address");

                /* Create our new contract now. */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrRecipient << addrSource << nAmount; //we use addrRecipient from our debit contract instead of addrCredit

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }

            /* If addressed to non-standard object, this could be a tokenized debit, so do some checks to find out. */
            else if(addrRecipient.IsObject())
            {
                /* Enforce address resolution for wildcard claim. */
                if(addrCredit == TAO::API::ADDRESS_NONE)
                    throw Exception(-35, "Invalid parameter [name], expecting [exists]");

                /* Attempt to get the proof from the parameters. */
                const TAO::Register::Address addrProof =
                    ExtractAddress(jParams, "proof");

                /* Check that the owner is a token. */
                if(oRecipient.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                    return false;

                /* Read our token contract now since we now know it's correct. */
                TAO::Register::Object oToken;
                if(!LLD::Register->ReadObject(oRecipient.hashOwner, oToken, TAO::Ledger::FLAGS::LOOKUP))
                    return false;

                /* We shouldn't ever evaluate to true here, but if we do let's not make things worse for ourselves. */
                if(oToken.Standard() != TAO::Register::OBJECTS::TOKEN)
                    return false;

                /* Retrieve the hash proof account and check that it is the same token type as the asset owner */
                TAO::Register::Object oProof;
                if(!LLD::Register->ReadObject(addrProof, oProof, TAO::Ledger::FLAGS::LOOKUP))
                    return false;

                /* Check that the proof is an account for the same token as the asset owner */
                if(oProof.get<uint256_t>("token") != oToken.get<uint256_t>("token"))
                    throw Exception(-61, "Proof account is for a different token than the asset token.");

                /* Retrieve the account to debit from. */
                TAO::Register::Object oSource;
                if(!LLD::Register->ReadObject(addrSource, oSource, TAO::Ledger::FLAGS::LOOKUP))
                    return false;

                /* Read our crediting account. */
                TAO::Register::Object oCredit;
                if(!LLD::Register->ReadObject(addrCredit, oCredit, TAO::Ledger::FLAGS::LOOKUP))
                    throw Exception(-33, "Incorrect or missing name / address");

                /* Check that the account being debited from is the same token type as credit. */
                if(oSource.get<uint256_t>("token") != oCredit.get<uint256_t>("token"))
                    throw Exception(-33, "Source token account mismatch with recipient token");

                /* Now lets check our expected types match. */
                if(!CheckStandard(jParams, oCredit))
                    throw Exception(-49, "Unsupported type for name/address");

                /* Calculate the partial amount we want to claim based on our share of the proof tokens */
                const uint64_t nPartial =
                    (oProof.get<uint64_t>("balance") * nAmount) / oToken.get<uint64_t>("supply");

                /* Create our new contract now. */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract);
                tContract << addrCredit << addrProof << nPartial;

                /* Push to our contract queue. */
                vContracts.push_back(tContract);

                return true;
            }
        }

        return false; //if we get this far, this contract is not creditable
    }


    /* Builds a claim contract based on given contract and related parameters. */
    bool BuildClaim(const encoding::json& jParams, const uint32_t nContract,
        const TAO::Operation::Contract& rTransfer, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Copy our txid out of the contract. */
        const uint512_t hashTx = rTransfer.Hash();

        /* Reset the contract to the position of the primitive. */
        const uint8_t nPrimitive = rTransfer.Primitive();
        rTransfer.SeekToPrimitive(false); //false to skip our primitive

        /* Check that primitive is correct. */
        if(nPrimitive != TAO::Operation::OP::TRANSFER)
            return false;

        /* Grab our address to pass back. */
        uint256_t hashAddress;
        rTransfer >> hashAddress;

        /* Deserialize some values. */
        uint256_t hashRecipient;
        rTransfer >> hashRecipient;

        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check that recipient is current session. */
        if(hashRecipient != hashGenesis)
            return false;

        /* Check out our object now. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashAddress, tObject, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Create our new contract now. */
        TAO::Operation::Contract tContract;
        tContract << uint8_t(TAO::Operation::OP::CLAIM) << hashTx << uint32_t(nContract) << hashAddress;

        /* Push to our contract queue. */
        vContracts.push_back(tContract);

        return true; //if we get this far, this claim was a success
    }


    /* Builds a void contract based on given contract and related parameters. */
    bool BuildVoid(const encoding::json& jParams, const uint32_t nContract,
        const TAO::Operation::Contract& rDependent, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check that we aren't voiding a transaction not owned by us. */
        if(rDependent.Caller() != hashGenesis)
            return false;

        /* Attempt to add our void contract now. */
        TAO::Operation::Contract tVoided;
        if(!AddVoid(rDependent, nContract, tVoided))
            return false;

        /* Push our void contract to outgoing queue. */
        vContracts.push_back(tVoided);

        return true;
    }


    /* Builds a name contract based on given input parameters. */
    void BuildName(const encoding::json& jParams, const uint256_t& hashRegister, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
        if(jParams.find("name") != jParams.end())
        {
            /* Check for empty name and alert caller of error. */
            const std::string& strName = jParams["name"].get<std::string>();
            if(strName.empty())
                return;

            /* Check if creating token, which would mean this is a global name. */
            std::string strNamespace = ""; //default to our local namespace

            /* Check for namespace parameter. */
            if(jParams.find("namespace") != jParams.end())
                strNamespace = jParams["namespace"].get<std::string>();

            /* Check for a global name override for tokens. */
            if(strNamespace.empty() && hashRegister.GetType() == TAO::Register::Address::TOKEN)
                strNamespace = TAO::Register::NAMESPACE::GLOBAL;

            /* Add an optional name if supplied. */
            vContracts.push_back
            (
                Names::CreateName(Authentication::Caller(jParams),
                strName, strNamespace, hashRegister)
            );
        }
    }


    /* Build a standard object based on hard-coded standards of object register. */
    TAO::Register::Object BuildStandard(const encoding::json& jParams, uint256_t &hashRegister)
    {
        /* Extract our type from input parameters. */
        const std::string strStandard = ExtractType(jParams);

        /* Handle for standard account type. */
        if(strStandard == "account")
        {
            /* Generate register address for account. */
            hashRegister =
                TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* If this is not a NXS token account, verify that the token identifier is for a valid token */
            const TAO::Register::Address hashToken = ExtractToken(jParams);
            if(hashToken != 0)
            {
                /* Check our address before hitting the database. */
                if(hashToken.GetType() != TAO::Register::Address::TOKEN)
                    throw Exception(-57, "Invalid parameter [token]");

                /* Get the register off the disk. */
                TAO::Register::Object tToken;
                if(!LLD::Register->ReadObject(hashToken, tToken, TAO::Ledger::FLAGS::LOOKUP))
                    throw Exception(-13, "Object not found");

                /* Check the standard */
                if(tToken.Standard() != TAO::Register::OBJECTS::TOKEN)
                    throw Exception(-57, "Invalid parameter [token]");
            }

            /* Create an account object register. */
            return TAO::Register::CreateAccount(hashToken);
        }

        /* Handle for standard token type. */
        if(strStandard == "token")
        {
            /* Generate register address for token. */
            hashRegister =
                TAO::Register::Address(TAO::Register::Address::TOKEN);

            /* Extract the supply parameter */
            const uint64_t nSupply  =
                ExtractInteger<uint64_t>(jParams, "supply");

            /* Check for nDecimals parameter. */
            const uint8_t nDecimals =
                ExtractInteger<uint64_t>(jParams, "decimals", 2, convert::MAX_DIGITS); //2, 8: default of 2 decimals, max of 8

            /* Sanitize the supply/decimals combination for uint64 overflow */
            const uint64_t nCoinFigures = math::pow(10, nDecimals);
            if(nDecimals > 0 && nSupply > (std::numeric_limits<uint64_t>::max() / nCoinFigures))
                throw Exception(-178, "Invalid supply/decimals. The maximum supply/decimals cannot exceed 18446744073709551615");

            /* Create a token object register. */
            return TAO::Register::CreateToken(hashRegister, nSupply * nCoinFigures, nDecimals);
        }

        /* Handle for standard name type. */
        if(strStandard == "name" || strStandard == "global")
        {
            /* Get our genesis-id for this call. */
            const uint256_t hashGenesis =
                Authentication::Caller(jParams);

            /* Check for required parameters. */
            if(!CheckParameter(jParams, "name", "string"))
                throw Exception(-28, "Missing parameter [name] for command");

            /* Check for required parameters. */
            if(!CheckParameter(jParams, "register", "string"))
                throw Exception(-28, "Missing parameter [register] for command");

            /* Grab our name parameter now. */
            const std::string strName =
                jParams["name"].get<std::string>();

            /* Grab our new register address to point towards. */
            const TAO::Register::Address hashExternal =
                TAO::Register::Address(jParams["register"].get<std::string>());

            /* Check for valid address now. */
            if(!hashExternal.IsValid())
                throw Exception(-57, "Invalid Parameter [register]");

            /* Check for global parameters. */
            const bool fGlobal =
                ExtractBoolean(jParams, "global", (strStandard == "global"));

            /* Check for reserved values. */
            if(fGlobal && TAO::Register::NAME::Reserved(strName))
                throw Exception(-22, "Field [", strName, "] is a reserved name");

            /* Check for namespace parameter. */
            if(CheckParameter(jParams, "namespace", "string"))
            {
                /* Grab our namespace parameter now. */
                const std::string strNamespace =
                    jParams["namespace"].get<std::string>();

                /* Grab our namespace's address. */
                const uint256_t hashNamespace =
                    TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

                /* Check for global names. */
                if(fGlobal)
                    throw Exception(-57, "Invalid parameter [global]");

                /* Make sure the namespace exists. */
                TAO::Register::Object rNamespace;
                if(!LLD::Register->ReadObject(hashNamespace, rNamespace, TAO::Ledger::FLAGS::LOOKUP))
                    throw Exception(-95, "Namespace does not exist [", strNamespace, "]");

                /* Check that the namespace is owned by us. */
                if(rNamespace.hashOwner != hashGenesis)
                    throw Exception(-96, "Cannot create name in [", strNamespace, "] you don't own");

                /* Calculate our address now. */
                hashRegister =
                    TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

                /* Build our name if in namespace. */
                return TAO::Register::CreateName(strNamespace, strName, hashExternal);
            }

            /* If none found assume in local namespace. */
            else if(!fGlobal)
            {
                /* Calculate our address now. */
                hashRegister =
                    TAO::Register::Address(strName, hashGenesis, TAO::Register::Address::NAME);

                /* Build our name if in local user-space which has an empty namespace. */
                return TAO::Register::CreateName("", strName, hashExternal);
            }

            /* Handle for global names from here. */
            if(!fGlobal)
                throw Exception(-28, "Missing parameter [global] for command");

            /* Grab our namespace's address. */
            const uint256_t hashNamespace =
                TAO::Register::Address(TAO::Register::NAMESPACE::GLOBAL, TAO::Register::Address::NAMESPACE);

            /* Calculate our address now. */
            hashRegister =
                TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

            /* Build our name if not in namespace. */
            return TAO::Register::CreateName(TAO::Register::NAMESPACE::GLOBAL, strName, hashExternal);
        }

        /* Handle for standard namespace type. */
        if(strStandard == "namespace")
        {
            /* Check for required parameters. */
            if(!CheckParameter(jParams, "namespace", "string"))
                throw Exception(-28, "Missing parameter [namespace] for command");

            /* Grab our namespace now. */
            const std::string strNamespace =
                jParams["namespace"].get<std::string>();

            /* Check namespace for case/allowed characters */
            if (!std::all_of(strNamespace.cbegin(), strNamespace.cend(),
                [](char c) { return std::islower(c) || std::isdigit(c) || c == '.'; })) //allow letters, numbers, and '.'
                throw Exception(-162, "Namespace can only contain lowercase letters, numbers, periods (.)");

            /* Check for reserved names. */
            if(TAO::Register::NAMESPACE::Reserved(strNamespace))
                throw Exception(-200, "Namespaces can't contain reserved names");

            /* Grab our namespace's address. */
            hashRegister =
                TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

            return TAO::Register::CreateNamespace(strNamespace);
        }

        throw Exception(-23, "Object standard [", strStandard, "] not available for command");
    }


    /* Build a blank object based on _commands string, generating register address as well. */
    TAO::Register::Object BuildObject(uint256_t &hashRegister, const uint16_t nUserType)
    {
        /* Check for invalid user-types. */
        if(!USER_TYPES::Valid(nUserType))
            throw Exception(-56, "Invalid user-type enum [", nUserType, "]");

        /* Start with our blank object. */
        TAO::Register::Object tObject =
            TAO::Register::Object();

        /* Serialize our commands name if applicable. */
        tObject << std::string("_usertype") << uint8_t(TAO::Register::TYPES::UINT16_T) << uint16_t(nUserType);

        /* Set the address return value now. */
        hashRegister =
            TAO::Register::Address(TAO::Register::Address::OBJECT);

        return tObject;
    }


    /* Update the public keys in crypto object register. */
    TAO::Operation::Contract BuildCrypto(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                                         const SecureString& strPIN, const uint8_t nKeyType)
    {
        /* Generate register address for crypto register deterministically */
        const TAO::Register::Address addrCrypto =
            TAO::Register::Address(std::string("crypto"), pCredentials->Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the existing crypto object register. */
        TAO::Register::Object oCrypto;
        if(!LLD::Register->ReadObject(addrCrypto, oCrypto))
            throw Exception(-33, "Failed to read crypto object register");

        /* Declare operation stream to serialize all of the field updates*/
        TAO::Operation::Stream ssUpdate;

        /* Update the AUTH key always. */
        ssUpdate << std::string("auth") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
        ssUpdate << pCredentials->KeyHash("auth", 0, strPIN, nKeyType);

        /* Update the LISP network key if enabled. */
        if(oCrypto.get<uint256_t>("lisp") != 0)
        {
            ssUpdate << std::string("lisp") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("lisp", 0, strPIN, nKeyType);
        }

        /* Update the NETWORK key if enabled. */
        if(oCrypto.get<uint256_t>("network") != 0)
        {
            ssUpdate << std::string("network") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("network", 0, strPIN, nKeyType);
        }

        /* Update the SIGN key if enabled. */
        if(oCrypto.get<uint256_t>("sign") != 0)
        {
            ssUpdate << std::string("sign") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("sign", 0, strPIN, nKeyType);
        }

        /* Update the VERIFY key if enabled. */
        if(oCrypto.get<uint256_t>("verify") != 0)
        {
            ssUpdate << std::string("verify") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("verify", 0, strPIN, nKeyType);
        }

        /* Add the crypto update contract. */
        TAO::Operation::Contract tContract;
        tContract << uint8_t(TAO::Operation::OP::WRITE) << addrCrypto << ssUpdate.Bytes();

        return tContract;
    }
} // End TAO namespace
