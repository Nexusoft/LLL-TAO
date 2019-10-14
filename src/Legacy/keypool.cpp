/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <Legacy/wallet/keypool.h>
#include <Legacy/types/keypoolentry.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <exception>
#include <mutex>


namespace Legacy
{

    /** Move Constructor. **/
    KeyPool::KeyPool(KeyPool&& pool)
    : setKeyPool (std::move(pool.setKeyPool))
    , poolWallet (pool.poolWallet)
    {
    }


    /*  Clears any existing keys in the pool and the wallet database
     *  and generates a completely new key set.
     */
    bool KeyPool::NewKeyPool()
    {

        if(poolWallet.IsFileBacked())
        {
            if(poolWallet.IsLocked())
                return false;

            const uint64_t nKeyPoolSizeSetting = std::max((uint64_t)0, (uint64_t)config::GetArg("-keypool", DEFAULT_KEY_POOL_SIZE));
            const uint64_t nKeys = std::max(nKeyPoolSizeSetting, (uint64_t)MINIMUM_KEY_POOL_SIZE);

            WalletDB walletdb(poolWallet.GetWalletFile());

            std::vector<uint64_t> vPoolIndexList;

            /* Remove all entries for old key pool from database */
            {
                /* Copy set and clear it (removes old keys from availability), then release lock before erasing them from database */
                LOCK(cs_keyPool);

                if(setKeyPool.size() > 0)
                {
                    debug::log(2, FUNCTION, "Erasing previous key pool entries");

                    for(uint64_t nPoolIndex : setKeyPool)
                        vPoolIndexList.push_back(nPoolIndex);
                }

                setKeyPool.clear();
            }


            for(uint64_t nPoolIndex : vPoolIndexList)
                walletdb.ErasePool(nPoolIndex);

            vPoolIndexList.clear();

            /* Generate a new key pool with a full set of keys */
            {
                LOCK(cs_keyPool);

                for(uint64_t nPoolIndex = 0; nPoolIndex < nKeys; nPoolIndex++)
                {
                    setKeyPool.insert(nPoolIndex);
                    vPoolIndexList.push_back(nPoolIndex);
                }
            }

            /* Write keys to database using list of indexes added to pool */
            for(uint64_t nPoolIndex : vPoolIndexList)
            {
                if(!walletdb.WritePool(nPoolIndex, KeyPoolEntry(poolWallet.GenerateNewKey())))
                    throw std::runtime_error("KeyPool::NewKeyPool() : writing generated key failed");
            }

            debug::log(2, FUNCTION, "Added ", nKeys, " new keys to key pool");
        }

        return true;
    }


    /* Adds keys to key pool to top up the number of entries. */
    bool KeyPool::TopUpKeyPool(bool fForceRefill)
    {
    	bool fKeysAdded = false;

        if(poolWallet.IsFileBacked())
        {
            if(poolWallet.IsLocked())
                return false;

            /* Desired key pool size */
            const uint64_t nKeyPoolSizeSetting = std::max((uint64_t)0, (uint64_t)config::GetArg("-keypool", DEFAULT_KEY_POOL_SIZE));
            const uint64_t nTargetSize = std::max(nKeyPoolSizeSetting, (uint64_t)MINIMUM_KEY_POOL_SIZE);
            const uint64_t nMinimumSize = std::min(nKeyPoolSizeSetting, (uint64_t)MINIMUM_KEY_POOL_SIZE);

            WalletDB walletdb(poolWallet.GetWalletFile());

            std::vector<uint64_t> vPoolIndexList;

            /* Current key pool size */
            uint64_t nStartingSize = setKeyPool.size();

            {
                LOCK(cs_keyPool);

                if(nStartingSize > nMinimumSize && !fForceRefill)
                	return true; // Pool does not need refill until it falls to minimum size

                debug::log(2, FUNCTION, "Topping up Keypool, current size = ", nStartingSize, " target size = ",  nTargetSize);

                /* Current max pool index in the pool */
                uint64_t nCurrentMaxPoolIndex = 0;
                if(!setKeyPool.empty())
                    nCurrentMaxPoolIndex = *(--setKeyPool.cend());

                /* New pool indexes will begin from the current max */
                uint64_t nNewPoolIndex = nCurrentMaxPoolIndex;

                vPoolIndexList.resize(nTargetSize - nStartingSize);

                /* Top up key pool */
                for(uint64_t i = (nStartingSize + 1); i <= nTargetSize; ++i)
                {
                    ++nNewPoolIndex;

                    /* Store the pool index for the new key in the key pool */
                    setKeyPool.insert(nNewPoolIndex);
                    vPoolIndexList.push_back(nNewPoolIndex);

                    fKeysAdded = true;
                }
            }

            /* Write keys to database using list of indexes added to pool */
            for(uint64_t nPoolIndex : vPoolIndexList)
            {
                if(!walletdb.WritePool(nPoolIndex, KeyPoolEntry(poolWallet.GenerateNewKey())))
                    throw std::runtime_error("KeyPool::NewKeyPool() : writing generated key failed");
            }

            if(fKeysAdded)
                debug::log(2, FUNCTION, "Keypool topped up, ", (nTargetSize - nStartingSize), " keys added, new size = ",  nTargetSize);
        }

        return true;
    }


    /* Manually adds a key pool entry. This only adds the entry to the pool. */
    uint64_t KeyPool::AddKey(const KeyPoolEntry& keypoolEntry)
    {
        if(poolWallet.IsFileBacked())
        {
            if(poolWallet.IsLocked())
                return false;

            uint64_t nPoolIndex = 1;

            {
                LOCK(cs_keyPool);

                if(!setKeyPool.empty())
                    nPoolIndex += *(--setKeyPool.cend());

                /* This "reserves" the index so we can release pool lock before writing to database */
                setKeyPool.insert(nPoolIndex);
            }

            WalletDB walletdb(poolWallet.GetWalletFile());

            if(!walletdb.WritePool(nPoolIndex, keypoolEntry))
                throw std::runtime_error("KeyPool::AddKey() : writing added key failed");

            return nPoolIndex;
        }

        return 0;
    }


    /* Extracts a key from the key pool. This both reserves and keeps the key,
     * removing it from the pool.
     */
    bool KeyPool::GetKeyFromPool(std::vector<uint8_t>& key, bool fUseDefaultWhenEmpty)
    {
        uint64_t nPoolIndex = 0;
        KeyPoolEntry keypoolEntry;

        /* Attempt to reserve a key from the key pool */
        ReserveKeyFromPool(nPoolIndex, keypoolEntry);

        if(nPoolIndex == -1)
        {
            /* Key pool is empty or wallet is locked, attempt to use default key when requested */
            auto vchPoolWalletDefaultKey = poolWallet.GetDefaultKey();

            if(fUseDefaultWhenEmpty && !vchPoolWalletDefaultKey.empty())
            {
                key = vchPoolWalletDefaultKey;
                return true;
            }

            /* When not using default key, generate a new key */
            if(poolWallet.IsLocked()) return false;

            key = poolWallet.GenerateNewKey();

            return true;
        }

        KeepKey(nPoolIndex);

        key = keypoolEntry.vchPubKey;

        return true;
    }


    /* Reserves a key pool entry out of this key pool. After reserving it, the
     * key pool entry is unavailable for other use.
     */
    void KeyPool::ReserveKeyFromPool(uint64_t& nPoolIndex, KeyPoolEntry& keypoolEntry)
    {
        nPoolIndex = -1;
        keypoolEntry.vchPubKey.clear();

        if(poolWallet.IsFileBacked())
        {
            if(!poolWallet.IsLocked())
                TopUpKeyPool();

            {
                LOCK(cs_keyPool);

                if(setKeyPool.empty())
                    return;

                /* Get the oldest key (smallest key pool index) */
                auto si = setKeyPool.begin();
                nPoolIndex = *(si);

                /* Reserve key removes it from the key pool, but leaves the key pool entry in the wallet database.
                 * This will later be removed by KeepKey() or the key will be re-added to the pool by ReturnKey().
                 * Shutting down and later restarting has the same effect as ReturnKey().
                 */
                setKeyPool.erase(si);
            }

            WalletDB walletdb(poolWallet.GetWalletFile());

            /* Now that we have reserved the index, retrieve the key pool entry from the database */
            if(!walletdb.ReadPool(nPoolIndex, keypoolEntry))
            {
                debug::error(FUNCTION, "KeyPool::ReserveKeyFromPool() : unable to read key pool entry");
                nPoolIndex = -1;
                return;
            }

            /* Validate that the key is a valid key for the containing wallet */
            if(!poolWallet.HaveKey(LLC::SK256(keypoolEntry.vchPubKey)))
                throw std::runtime_error("KeyPool::ReserveKeyFromPool() : unknown key in key pool");

            assert(!keypoolEntry.vchPubKey.empty());
            debug::log(3, FUNCTION, "Keypool reserve ", nPoolIndex);
        }
    }


    /* Marks a reserved key as used, removing it from the key pool. */
    void KeyPool::KeepKey(uint64_t nPoolIndex)
    {
        if(poolWallet.IsFileBacked())
        {
            /* Remove from key pool */
            WalletDB walletdb(poolWallet.GetWalletFile());
            walletdb.ErasePool(nPoolIndex);

            debug::log(3, FUNCTION, "Keypool keep ", nPoolIndex);
        }
    }


    /* Returns a reserved key to the key pool. */
    void KeyPool::ReturnKey(uint64_t nPoolIndex)
    {
        if(poolWallet.IsFileBacked())
        {
            LOCK(cs_keyPool);

            setKeyPool.insert(nPoolIndex);

            debug::log(3, FUNCTION, "Keypool return ", nPoolIndex);
        }
    }


    /* Retrieves the creation time for the pool's oldest entry. */
    uint64_t KeyPool::GetOldestKeyPoolTime()
    {
        uint64_t nPoolIndex = 0;
        KeyPoolEntry keypoolEntry;

        /* ReserveKeyFromPool returns the oldest key pool entry */
        ReserveKeyFromPool(nPoolIndex, keypoolEntry);

        if(nPoolIndex == -1)
            return runtime::unifiedtimestamp();

        /* Reserve call was just to access oldest key pool entry, not to use it, so return it immediately */
        ReturnKey(nPoolIndex);

        return keypoolEntry.nTime;
    }

}
