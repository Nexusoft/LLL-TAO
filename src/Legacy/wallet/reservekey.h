/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_RESERVEKEY_H
#define NEXUS_LEGACY_WALLET_RESERVEKEY_H

#include <vector>

#include <Legacy/wallet/keypool.h>
#include <Legacy/wallet/wallet.h>

#include <Util/include/args.h>

namespace Legacy
{

    /** @class ReserveKey
     *
     *  Holds the public key value of a key reserved in the key pool but not
     *  yet used.  Supports either returning the key to the pool or keeping it
     *  for use.
     *
     **/
    class ReserveKey
    {
    protected:
        /** Wallet containing the key pool where we want to reserve keys **/
        Wallet& wallet;

        /** The key pool index of a reserved key in the key pool, or -1 if no key reserved **/
        uint64_t nPoolIndex;

        /** The public key value of the reserved key. Empty if no key reserved. **/
        std::vector<uint8_t> vchPubKey;


    public:
        /** Constructor
         *
         *  Initializes reserve key from wallet pointer.
         *
         *  @param[in] pWalletIn The wallet where keys will be reserved, cannot be nullptr
         *
         *  @deprecated supported for backward compatability
         *
         **/
        ReserveKey(Wallet* pWalletIn)
        : wallet(*pWalletIn)
        , nPoolIndex(-1)
        , vchPubKey()

        { }


        /** Constructor
         *
         *  Initializes reserve key from wallet reference.
         *
         *  @param[in] walletIn The wallet where keys will be reserved
         *
         **/
        ReserveKey(Wallet& walletIn)
        : wallet(walletIn)
        , nPoolIndex(-1)
        , vchPubKey()
        { }


        /** Destructor
         *
         *  Returns any reserved key to the key pool.
         *
         **/
        ~ReserveKey()
        {
            if (!config::fShutdown)
                ReturnKey();
        }


        /* If you copy a ReserveKey that has a key reserved, then keep/return it in one copy, the other
         * copy becomes invalid but could still be used. For example, it would be a problem if KeepKey()
         * were called on one copy, then ReturnKey() called on the other. The key would be put back into
         * the key pool even though it had been used. All of this is why copy disallowed.
         */

        /** Copy constructor deleted. No copy allowed **/
        ReserveKey(const ReserveKey&) = delete;


        /** Copy assignment operator deleted. No copy allowed **/
        ReserveKey& operator= (const ReserveKey& rhs) = delete;


        /** GetReservedKey
         *
         *  Retrieves the public key value for the currently reserved key.
         *  If none yet reserved, will first reserve a new key from the key pool of the associated wallet.
         *
         *  @return the public key value of the reserved key
         *
         **/
        std::vector<uint8_t> GetReservedKey();


        /** KeepKey
         *
         *  Marks the reserved key as used, removing it from the key pool.
         *  Must call GetReservedKey first to reserve a key, or this does nothing.
         *
         **/
        void KeepKey();


        /** ReturnKey
         *
         *  Returns a reserved key to the key pool. After call, it is no longer reserved.
         *
         **/
        void ReturnKey();

    };

}

#endif
