/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <exception>
#include <mutex>

#include <LLC/hash/SK.h>

#include <Legacy/wallet/keypool.h>
#include <Legacy/wallet/keypoolentry.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace Legacy
{
    
    /*  Clears any existing keys in the pool and the wallet database
     *  and generates a completely new key set.
     */
    bool CKeyPool::NewKeyPool()
    {

        if (poolWallet.IsFileBacked())
        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

	        if (poolWallet.IsLocked())
	            return false;

            CWalletDB walletdb(poolWallet.GetWalletFile());

            /* Remove all entries for old key pool from database */
            for(int64_t nPoolIndex : setKeyPool)
                walletdb.ErasePool(nPoolIndex);

            setKeyPool.clear();

            /* Generate a new key pool with a full set of keys */
            const int64_t nKeys = std::max(config::GetArg("-keypool", CKeyPool::DEFAULT_KEY_POOL_SIZE), MINIMUM_KEY_POOL_SIZE);

            for (int64_t i = 0; i < nKeys; i++)
            {
                int64_t nPoolIndex = i + 1;

                if (!walletdb.WritePool(nPoolIndex, CKeyPoolEntry(poolWallet.GenerateNewKey())))
                    throw std::runtime_error("CKeyPool::NewKeyPool() : writing generated key failed");

                setKeyPool.insert(nPoolIndex);
            }
            
            if (config::GetBoolArg("-printkeypool"))
                debug::log(0, "CKeyPool::NewKeyPool wrote %" PRI64d " new keys", nKeys);

            walletdb.Close();
        }

        return true;
    }


    /* Adds keys to key pool to top up the number of entries. */
    bool CKeyPool::TopUpKeyPool()
    {
    	bool fPrintKeyPool = config::GetBoolArg("-printkeypool"); // avoids having to call arg function repeatedly
    	bool fKeysAdded = false;

        if (poolWallet.IsFileBacked())
        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

            /* Current key pool size */
            int64_t nStartingSize = setKeyPool.size();

            /* Desired key pool size */
            const int64_t nTargetSize = std::max(config::GetArg("-keypool", CKeyPool::DEFAULT_KEY_POOL_SIZE), MINIMUM_KEY_POOL_SIZE);

            if (nStartingSize >= nTargetSize)
            	return true; // pool already filled

            if (poolWallet.IsLocked())
                return false;

            CWalletDB walletdb(poolWallet.GetWalletFile());

            /* Current max pool index in the pool */
            int64_t nCurrentMaxPoolIndex = 0;
            if (!setKeyPool.empty())
                nCurrentMaxPoolIndex = *(--setKeyPool.cend());

            /* New pool indexes will begin from the current max */
            int64_t nNewPoolIndex = nCurrentMaxPoolIndex;

            /* Top up key pool */
            for (int64_t i = nStartingSize; i < nTargetSize; ++i)
            {
                ++nNewPoolIndex;

                /* Generate a new key and add the key pool entry to the wallet database */
                if (!walletdb.WritePool(nNewPoolIndex, CKeyPoolEntry(poolWallet.GenerateNewKey())))
                    throw std::runtime_error("CKeyPool::TopUpKeyPool() : writing generated key failed");

                /* Store the pool index for the new key in the key pool */
                setKeyPool.insert(nNewPoolIndex);

                if (fPrintKeyPool)
                    debug::log(0, "Keypool added key %" PRI64d "", nNewPoolIndex);

                fKeysAdded = true;
            }

            if (fPrintKeyPool && fKeysAdded)
                debug::log(0, "Keypool topped up, %d keys added, new size=%d", (nTargetSize - nStartingSize), nTargetSize);

            walletdb.Close();
        }

        return true;
    }


    /* Manually adds a key pool entry. This only adds the entry to the pool. */
    int64_t CKeyPool::AddKey(const CKeyPoolEntry& keypoolEntry)
    {
        if (poolWallet.IsFileBacked())
        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

            CWalletDB walletdb(poolWallet.GetWalletFile());

            int64_t nPoolIndex = 1 + *(--setKeyPool.cend());

            if (!walletdb.WritePool(nPoolIndex, keypoolEntry))
                throw std::runtime_error("CKeyPool::AddKey() : writing added key failed");

            setKeyPool.insert(nPoolIndex);

            walletdb.Close();

            return nPoolIndex;
        }

        return -1;
    }


    /* Extracts a key from the key pool. This both reserves and keeps the key,
     * removing it from the pool.
     */
    bool CKeyPool::GetKeyFromPool(std::vector<uint8_t>& key, bool fUseDefaultWhenEmpty)
    {
        int64_t nPoolIndex = 0;
        CKeyPoolEntry keypoolEntry;

        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

            /* Attempt to reserve a key from the key pool */
            ReserveKeyFromPool(nPoolIndex, keypoolEntry);

            if (nPoolIndex == -1)
            {
            	/* Key pool is empty, attempt to use default key when requested */
                auto vchPoolWalletDefaultKey = poolWallet.GetDefaultKey();

                if (fUseDefaultWhenEmpty && !vchPoolWalletDefaultKey.empty())
                {
                    key = vchPoolWalletDefaultKey;
                    return true;
                }

                /* When not using default key, generate a new key */
                if (poolWallet.IsLocked()) return false;

                key = poolWallet.GenerateNewKey();

                return true;
            }

            KeepKey(nPoolIndex);

            key = keypoolEntry.vchPubKey;
        }

        return true;
    }


    /* Reserves a key pool entry out of this key pool. After reserving it, the
     * key pool entry is unavailable for other use. 
     */
    void CKeyPool::ReserveKeyFromPool(int64_t& nPoolIndex, CKeyPoolEntry& keypoolEntry)
    {
        nPoolIndex = -1;
        keypoolEntry.vchPubKey.clear();

        if (poolWallet.IsFileBacked())
        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

            if (!poolWallet.IsLocked())
                TopUpKeyPool();

            if(setKeyPool.empty())
                return;

            CWalletDB walletdb(poolWallet.GetWalletFile());

            /* Get the oldest key (smallest key pool index) */
            auto si = setKeyPool.begin();
            nPoolIndex = *(si);

            /* Reserve key removes it from the key pool, but leaves the key pool entry in the wallet database.
             * This will later be removed by KeepKey() or the key will be re-added to the pool by ReturnKey().
             * Shutting down and later restarting has the same effect as ReturnKey().
             */
            setKeyPool.erase(si);

            /* Retrieve the key pool entry from the database */
            if (!walletdb.ReadPool(nPoolIndex, keypoolEntry))
                throw std::runtime_error("CKeyPool::ReserveKeyFromPool() : unable to read key pool entry");

            /* Validate that the key is a valid key for the containing wallet */
            if (!poolWallet.HaveKey(LLC::SK256(keypoolEntry.vchPubKey)))
                throw std::runtime_error("CKeyPool::ReserveKeyFromPool() : unknown key in key pool");

            assert(!keypoolEntry.vchPubKey.empty());
            if (config::GetBoolArg("-printkeypool"))
                debug::log(0, "Keypool reserve %" PRI64d "", nPoolIndex);

            walletdb.Close();
        }
    }


    /* Marks a reserved key as used, removing it from the key pool. */
    void CKeyPool::KeepKey(int64_t nPoolIndex)
    {
        if (poolWallet.IsFileBacked())
        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

            /* Remove from key pool */
            CWalletDB walletdb(poolWallet.GetWalletFile());
            walletdb.ErasePool(nPoolIndex);

            if (config::GetBoolArg("-printkeypool"))
	            debug::log(0, "Keypool keep %" PRI64d "", nPoolIndex);

	        walletdb.Close();
        }

    }


    /* Returns a reserved key to the key pool. */
    void CKeyPool::ReturnKey(int64_t nPoolIndex)
    {
        if (poolWallet.IsFileBacked())
        {
            std::lock_guard<std::recursive_mutex> walletLock(poolWallet.cs_wallet);

            setKeyPool.insert(nPoolIndex);
        }

        if (config::GetBoolArg("-printkeypool"))
            debug::log(0, "Keypool return %" PRI64d "", nPoolIndex);
    }


    /* Retrieves the creation time for the pool's oldest entry. */
    int64_t CKeyPool::GetOldestKeyPoolTime()
    {
        int64_t nPoolIndex = 0;
        CKeyPoolEntry keypoolEntry;

        /* ReserveKeyFromPool returns the oldest key pool entry */
        ReserveKeyFromPool(nPoolIndex, keypoolEntry);

        if (nPoolIndex == -1)
            return runtime::UnifiedTimestamp();

        /* Reserve call was just to access oldest key pool entry, not to use it, so return it immediately */
        ReturnKey(nPoolIndex);

        return keypoolEntry.nTime;
    }

}
