/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/users.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium_minter.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/trust.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>

#include <Util/include/debug.h>

#include <vector>


namespace TAO
{
    namespace API
    {

        // TODO:
        // get identifer from notification (you need to add identifier to notification JSON)
        // check event processor config to see if we have an account or register address configured for identifier X
        // if so then use that to process this debit...
        //2 = jack:savings OR 2=asdfasdfsdfsdf
        // if they have configured a register address then set hashTo from that address
        // if they have configured an account name then we need to generate the register address from that name
        //  - register address is namespacehash:token:name
        //  - namespacehash is argon2 hash of username
        //  - Look at Names::ResolveAddress() method as you can probably use that

        /*  Background thread to handle/suppress sigchain notifications. */
        void Users::EventsThread()
        {
            /* Auto-login feature if configured. */
            try
            {
                if(config::GetBoolArg("-autologin") && !config::fMultiuser.load())
                {
                    /* Check for username and password. */
                    if(config::GetArg("-username", "") != ""
                    && config::GetArg("-password", "") != ""
                    && config::GetArg("-pin", "")      != "")
                    {
                        /* Keep a the credentials in secure allocated strings. */
                        SecureString strUsername = config::GetArg("-username", "").c_str();
                        SecureString strPassword = config::GetArg("-password", "").c_str();
                        SecureString strPin = config::GetArg("-pin", "").c_str();

                        /* Create the sigchain. */
                        memory::encrypted_ptr<TAO::Ledger::SignatureChain> user =
                            new TAO::Ledger::SignatureChain(strUsername.c_str(), strPassword.c_str());

                        /* Get the genesis ID. */
                        uint256_t hashGenesis = user->Genesis();

                        /* See if the sig chain exists */
                        if(!LLD::Ledger->HasGenesis(hashGenesis) || TAO::Ledger::mempool.Has(hashGenesis))
                        {
                            /* If it doesn't exist then create it if configured to do so */
                            if(config::GetBoolArg("-autocreate"))
                            {
                                /* The genesis transaction  */
                                TAO::Ledger::Transaction tx;

                                /* Create the sig chain genesis transaction */
                                CreateSigchain(strUsername, strPassword, strPin, tx);

                            }
                            else
                                throw APIException(-203, "Autologin user not found");
                        }

                        /* Check for duplicates in ledger db. */
                        TAO::Ledger::Transaction txPrev;

                        /* Get the last transaction. */
                        uint512_t hashLast;
                        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            user.free();
                            throw APIException(-138, "No previous transaction found");
                        }

                        /* Get previous transaction */
                        if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            user.free();
                            throw APIException(-138, "No previous transaction found");
                        }

                        /* Genesis Transaction. */
                        TAO::Ledger::Transaction tx;
                        tx.NextHash(user->Generate(txPrev.nSequence + 1, config::GetArg("-pin", "").c_str(), false), txPrev.nNextType);

                        /* Check for consistency. */
                        if(txPrev.hashNext != tx.hashNext)
                        {
                            user.free();
                            throw APIException(-139, "Invalid credentials");
                        }

                        /* Setup the account. */
                        {
                            LOCK(MUTEX);
                            mapSessions.emplace(0, std::move(user));
                        }

                        /* Extract the PIN. */
                        if(!pActivePIN.IsNull())
                            pActivePIN.free();

                        /* The unlock actions to apply for autologin.  NOTE we do NOT unlock for transactions */
                        uint8_t nUnlockActions = TAO::Ledger::PinUnlock::UnlockActions::MINING
                                               | TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS
                                               | TAO::Ledger::PinUnlock::UnlockActions::STAKING;

                        /* Set account to unlocked. */
                        pActivePIN = new TAO::Ledger::PinUnlock(config::GetArg("-pin", "").c_str(), nUnlockActions);

                        /* Display that login was successful. */
                        debug::log(0, FUNCTION, "Auto-Login Successful");

                        /* Start the stake minter if successful login. */
                        TAO::Ledger::TritiumMinter::GetInstance().Start();
                    }
                }
            }
            catch(const APIException& e)
            {
                debug::error(FUNCTION, e.what());
            }

            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {
                /* Reset the events flag. */
                fEvent = false;

                /* If mining is enabled, notify miner LLP that events processor is finished processing transactions so mined blocks
                   can include these transactions and not orphan a mined block. */
                if(LLP::MINING_SERVER)
                    LLP::MINING_SERVER->NotifyEvent();

                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lock(EVENTS_MUTEX);
                CONDITION.wait_for(lock, std::chrono::milliseconds(5000), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                try
                {
                    /* Ensure that the user is logged, in, wallet unlocked, and unlocked for notifications. */
                    if(!LoggedIn() || Locked() || !CanProcessNotifications())
                        continue;

                    /* Make sure we don't send transactions out when processing a block. */
                    std::unique_lock<std::mutex> lock(TAO::Ledger::PROCESSING_MUTEX);

                    /* Get the session to be used for this API call */
                    json::json params;
                    uint256_t nSession = users->GetSession(params);

                    /* Get the account. */
                    memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
                    if(!user)
                        throw APIException(-10, "Invalid session ID");

                    /* Lock the signature chain. */
                    LOCK(users->CREATE_MUTEX);

                    /* Set the hash genesis for this user. */
                    uint256_t hashGenesis = user->Genesis();

                    /* Set the genesis id for the user. */
                    params["genesis"] = hashGenesis.ToString();

                    /* Get the PIN to be used for this API call */
                    SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::NOTIFICATIONS);

                    /* Retrieve user's default NXS account. */
                    std::string strAccount = "default";
                    TAO::Register::Object defaultAccount;
                    if(!TAO::Register::GetNameRegister(hashGenesis, strAccount, defaultAccount))
                        throw APIException(-63, "Could not retrieve default NXS account to credit");

                    /* Get the list of outstanding contracts. */
                    std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vContracts;
                    GetOutstanding(hashGenesis, vContracts);

                    /* Get the list of outstanding legacy transactions . */
                    std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> vLegacyTx;
                    GetOutstanding(hashGenesis, vLegacyTx);

                    /* Check if there is anything to process */
                    if(vContracts.size() == 0 && vLegacyTx.size() == 0)
                        continue;

                    /* Ensure that the signature is mature.  Note we only check this after we know there is something to process */
                    uint32_t nBlocksToMaturity = users->BlocksToMaturity(hashGenesis);

                    if(nBlocksToMaturity > 0)
                    {
                        debug::log(2, FUNCTION, "Skipping notifications as signature chain not mature. ", nBlocksToMaturity, " more confirmation(s) required.");
                        continue;
                    }

                    /* The transaction hash. */
                    uint512_t hashTx;

                    /* hash from, hash to, and amount for operations. */
                    TAO::Register::Address hashFrom;
                    TAO::Register::Address hashTo;


                    uint64_t nAmount = 0;
                    uint32_t nOut = 0;

                    /* Create the transaction output. */
                    TAO::Ledger::Transaction txout;
                    if(!TAO::Ledger::CreateTransaction(user, strPIN, txout))
                        throw APIException(-17, "Failed to create transaction");

                    /* Loop through each contract in the notification queue. */
                    for(const auto& contract : vContracts)
                    {
                        /* Ensure we don't breach the max contracts/per transaction, leaving room for the fee contract */
                        if(txout.Size() == TAO::Ledger::MAX_TRANSACTION_CONTRACTS -1)
                            break;

                        /* Get a reference to the contract */
                        const TAO::Operation::Contract& refContract = std::get<0>(contract);

                        /* Set the transaction hash. */
                        hashTx = refContract.Hash();

                        /* Get the maturity for this transaction. */
                        bool fMature = LLD::Ledger->ReadMature(hashTx);

                        /* Reset the contract operation stream. */
                        refContract.Reset();

                        /* Get the opcode. */
                        uint8_t OPERATION;
                        refContract >> OPERATION;

                        /* Check the opcodes for debit, coinbase or transfers. */
                        switch (OPERATION)
                        {
                            /* Check for Debits. */
                            case Operation::OP::DEBIT:
                            {
                                /* Check to see if there is a proof for the contract, indicating this is a split dividend payment
                                   and the hashProof is the account the proves the ownership of it*/
                                TAO::Register::Address hashProof;
                                hashProof = std::get<2>(contract);

                                if(hashProof != 0)
                                {
                                    /* If this is a split dividend payment then we can only (currently) process it if it is NXS.
                                       Therefore we need to retrieve the account/token the debit is from so that we can check */

                                    /* Get the token/account we are debiting from */
                                    refContract >> hashFrom;
                                    TAO::Register::Object from;
                                    if(!LLD::Register->ReadState(hashFrom, from))
                                        continue;

                                    /* Parse the object register. */
                                    if(!from.Parse())
                                        continue;

                                    /* Check the token type */
                                    if( from.get<uint256_t>("token") != 0)
                                    {
                                        debug::log(2, FUNCTION, "Skipping split dividend DEBIT as token is not NXS");
                                        continue;
                                    }

                                    /* If this is a NXS debit then process the credit to the default account */
                                    hashTo = defaultAccount.get<uint256_t>("address");

                                    /* Read the object register, which is the proof account . */
                                    TAO::Register::Object account;
                                    if(!LLD::Register->ReadState(hashProof, account, TAO::Ledger::FLAGS::MEMPOOL))
                                        continue;

                                    /* Parse the object register. */
                                    if(!account.Parse())
                                        continue;

                                    /* Check that this is an account */
                                    if(account.Standard() != TAO::Register::OBJECTS::ACCOUNT )
                                        continue;

                                    /* Get the token address */
                                    TAO::Register::Address hashToken = account.get<uint256_t>("token");

                                    /* Read the token register. */
                                    TAO::Register::Object token;
                                    if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                                        continue;

                                    /* Parse the object register. */
                                    if(!token.Parse())
                                        continue;

                                    /* Get the token supply so that we an determine our share */
                                    uint64_t nSupply = token.get<uint64_t>("supply");

                                    /* Get the balance of our token account */
                                    uint64_t nBalance = account.get<uint64_t>("balance");

                                    /* Get the amount from the debit contract*/
                                    uint64_t nAmount = 0;
                                    TAO::Register::Unpack(refContract, nAmount);

                                    /* Calculate the partial debit amount that this token holder is entitled to. */
                                    uint64_t nPartial = (nAmount * nBalance) / nSupply;

                                    /* Submit the payload object for the split dividend. Notice we use the hashProof */
                                    txout[nOut] << uint8_t(TAO::Operation::OP::CREDIT);
                                    txout[nOut] << hashTx << std::get<1>(contract);
                                    txout[nOut] << hashTo << hashProof;
                                    txout[nOut] << nPartial;

                                }
                                else
                                {
                                    /* Set to and from hashes and amount. */
                                    refContract >> hashFrom;
                                    refContract >> hashTo;
                                    refContract >> nAmount;

                                    /* Submit the payload object. */
                                    txout[nOut] << uint8_t(TAO::Operation::OP::CREDIT);
                                    txout[nOut] << hashTx << std::get<1>(contract);
                                    txout[nOut] << hashTo << hashFrom;
                                    txout[nOut] << nAmount;
                                }

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching DEBIT with CREDIT");

                                break;
                            }

                            /* Check for Coinbases. */
                            case Operation::OP::COINBASE:
                            {
                                /* Check that the coinbase is mature and ready to be credited. */
                                if(!fMature)
                                {
                                    debug::error(FUNCTION, "Immature coinbase.");
                                    continue;
                                }

                                /* Set the genesis hash and the amount. */
                                refContract >> hashFrom;
                                refContract >> nAmount;

                                /* Get the address that this name register for default account is pointing to. */
                                hashTo = defaultAccount.get<uint256_t>("address");

                                /* Submit the payload object. */
                                txout[nOut] << uint8_t(TAO::Operation::OP::CREDIT);
                                txout[nOut] << hashTx << std::get<1>(contract);
                                txout[nOut] << hashTo << hashFrom;
                                txout[nOut] << nAmount;

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching COINBASE with CREDIT");

                                break;
                            }

                            /* Check for Transfers. */
                            case Operation::OP::TRANSFER:
                            {
                                /* Get the address of the asset being transfered from the transaction. */
                                refContract >> hashFrom;

                                /* Get the genesis hash (recipient) of the transfer. */
                                refContract >> hashTo;

                                /* Read the force transfer flag */
                                uint8_t nType = 0;
                                refContract >> nType;

                                /* Ensure this wasn't a forced transfer (which requires no Claim) */
                                if(nType == TAO::Operation::TRANSFER::FORCE)
                                    continue;

                                /* Submit the payload object. */
                                txout[nOut] << uint8_t(TAO::Operation::OP::CLAIM);
                                txout[nOut] << hashTx << std::get<1>(contract);
                                txout[nOut] << hashFrom;

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching TRANSFER with CLAIM");

                                break;
                            }
                            default:
                                break;
                        }
                    }


                    /* Now process the legacy transactions */
                    for(const auto& contract : vLegacyTx)
                    {
                        /* Ensure we don't breach the max contracts/per transaction, leaving room for the fee contract */
                        if(txout.Size() == TAO::Ledger::MAX_TRANSACTION_CONTRACTS -1)
                            break;

                        /* Set the transaction hash. */
                        hashTx = contract.first->GetHash();

                        /* The index of the output in the legacy transaction */
                        uint32_t nContract = contract.second;

                        /* The TxOut to be checked */
                        const Legacy::TxOut& txLegacy = contract.first->vout[nContract];

                        /* The hash of the receiving account. */
                        TAO::Register::Address hashAccount;

                        /* Extract the sig chain account register address from the legacy script */
                        if(!Legacy::ExtractRegister(txLegacy.scriptPubKey, hashAccount))
                            continue;

                        /* Get the token / account object that the debit was made to. */
                        TAO::Register::Object debit;
                        if(!LLD::Register->ReadState(hashAccount, debit))
                            continue;

                        /* Parse the object register. */
                        if(!debit.Parse())
                            throw APIException(-41, "Failed to parse object from debit transaction");

                        /* Check for the owner to make sure this was a send to the current users account */
                        if(debit.hashOwner == user->Genesis())
                        {
                            /* Identify trust migration to create OP::MIGRATE instead of OP::CREDIT */

                            /* Check if output is new trust account (no stake or balance) */
                            if(debit.Standard() == TAO::Register::OBJECTS::TRUST
                                    && debit.get<uint64_t>("stake") == 0 && debit.get<uint64_t>("trust") == 0)
                            {
                                /* Need to check for migration.
                                 * Trust migration converts a legacy trust key to a trust account register.
                                 * It will send all inputs from an existing trust key, with one output to a new trust account.
                                 */
                                bool fMigration = false; //if this stays false, not a migration, fall through to OP::CREDIT

                                /* Trust key data we need for OP::MIGRATE */
                                uint32_t nScore;
                                uint512_t hashKey;
                                uint512_t hashLast;

                                /* This loop will only have one iteration. If it breaks out before end, fMigration stays false */
                                while(1)
                                {
                                    Legacy::TrustKey trustKey;

                                    /* Trust account output must be only output for the transaction */
                                    if(nContract != 0 || contract.first->vout.size() > 1)
                                        break;

                                    /* Retrieve the trust key being converted */
                                    if(!Legacy::FindMigratedTrustKey(*contract.first, trustKey))
                                        break;

                                    /* Verify trust key not already converted */
                                    hashKey = trustKey.GetHash();
                                    if(LLD::Legacy->HasTrustConversion(hashKey))
                                        break;

                                    /* Get last trust for the legacy trust key */
                                    TAO::Ledger::BlockState stateLast;
                                    if(!LLD::Ledger->ReadBlock(trustKey.hashLastBlock, stateLast))
                                        break;

                                    /* Last stake block must be at least v5 and coinstake must be a legacy transaction */
                                    if(stateLast.nVersion < 5 || stateLast.vtx[0].first != TAO::Ledger::LEGACY)
                                        break;

                                    /* Extract the coinstake from the last trust block */
                                    Legacy::Transaction txLast;
                                    if(!LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast))
                                        break;

                                    hashLast = txLast.GetHash();

                                    /* Extract the trust score from the coinstake */
                                    uint1024_t hashLastBlock;
                                    uint32_t nSequence;

                                    if(txLast.IsGenesis())
                                        nScore = 0;

                                    else if(!txLast.ExtractTrust(hashLastBlock, nSequence, nScore))
                                        break;

                                    fMigration = true;
                                    break;
                                }

                                /* Everything verified for migration and we have the data we need. Set up OP::MIGRATE */
                                if(fMigration)
                                {
                                    /* The amount to migrate */
                                    const int64_t nLegacyAmount = txLegacy.nValue;
                                    uint64_t nAmount = 0;
                                    if(nLegacyAmount > 0)
                                        nAmount = nLegacyAmount;

                                    /* Set up the OP::MIGRATE */
                                    txout[nOut] << uint8_t(TAO::Operation::OP::MIGRATE) << hashTx << hashAccount << hashKey
                                                << nAmount << nScore << hashLast;

                                    /* Increment the contract ID. */
                                    ++nOut;

                                    /* Log debug message. */
                                    debug::log(0, FUNCTION, "Matching LEGACY SEND with trust key MIGRATE");

                                    continue;
                                }
                            }

                            /* No migration. Use normal credit process */

                            /* Check the object base to see whether it is an account. */
                            if(debit.Base() == TAO::Register::OBJECTS::ACCOUNT)
                            {
                                if(debit.get<uint256_t>("token") != 0)
                                    throw APIException(-51, "Debit transaction is not for a NXS account.  Please use the tokens API for crediting token accounts.");

                                /* The amount to credit */
                                const uint64_t nAmount = txLegacy.nValue;

                                /* if we passed these checks then insert the credit contract into the tx */
                                txout[nOut] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashAccount <<  TAO::Register::WILDCARD_ADDRESS << nAmount;

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching LEGACY SEND with CREDIT");
                            }
                            else
                                continue;
                        }
                    }


                    /* If any of the notifications have been matched, execute the operations layer and sign the transaction. */
                    if(nOut)
                    {
                        /* Add the fee */
                        AddFee(txout);

                        /* Execute the operations layer. */
                        if(!txout.Build())
                            throw APIException(-30, "Failed to build register pre-states");

                        /* Sign the transaction. */
                        if(!txout.Sign(users->GetKey(txout.nSequence, strPIN, users->GetSession(params))))
                            throw APIException(-31, "Ledger failed to sign transaction");

                        /* Execute the operations layer. */
                        if(!TAO::Ledger::mempool.Accept(txout))
                            throw APIException(-32, "Failed to accept");
                    }
                }
                catch(const APIException& e)
                {
                    debug::error(FUNCTION, e.what());
                }

            }
        }


        /*  Notifies the events processor that an event has occurred so it
         *  can check and update it's state. */
        void Users::NotifyEvent()
        {
            fEvent = true;
            CONDITION.notify_one();
        }
    }
}
