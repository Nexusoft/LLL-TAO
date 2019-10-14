/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/network.h>

#include <Legacy/types/keypoolentry.h>
#include <Legacy/types/reservekey.h>

#include <Legacy/wallet/wallet.h>

#include <Util/include/debug.h>

#include <memory>
#include <string>
#include <vector>


namespace Legacy
{

    /* Initializes reserve key from wallet pointer. */
    ReserveKey::ReserveKey(Wallet* pWalletIn)
    : wallet     (*pWalletIn)
    , nPoolIndex (-1)
    , vchPubKey  ()
    {
    }


    /* Initializes reserve key from wallet reference. */
    ReserveKey::ReserveKey(Wallet& walletIn)
    : wallet     (walletIn)
    , nPoolIndex (-1)
    , vchPubKey  ( )
    {
    }


    /* Destructor */
    ReserveKey::~ReserveKey()
    {
        if(!config::fShutdown.load())
            ReturnKey();
    }


    /* Retrieves the public key value for the currently reserved key. */
    std::vector<uint8_t> ReserveKey::GetReservedKey()
    {
        if(nPoolIndex == -1)
        {
            /* Don't have a reserved key in this instance, yet, so need to reserve one */
            KeyPoolEntry keypoolEntry;
            wallet.GetKeyPool().ReserveKeyFromPool(nPoolIndex, keypoolEntry);

            if(nPoolIndex != -1)
                vchPubKey = keypoolEntry.vchPubKey;
            else
            {
                debug::log(0, FUNCTION, "Warning: Using default key instead of a new key, top up your keypool.");
                vchPubKey = wallet.GetDefaultKey();
            }
        }

        assert(!vchPubKey.empty());
        return vchPubKey;
    }


    /* Marks the reserved key as used, removing it from the key pool. */
    void ReserveKey::KeepKey()
    {
        if(nPoolIndex != -1)
            wallet.GetKeyPool().KeepKey(nPoolIndex);

        nPoolIndex = -1;
        vchPubKey.clear();
    }


    /* Returns a reserved key to the key pool. After call, it is no longer reserved. */
    void ReserveKey::ReturnKey()
    {
        if(nPoolIndex != -1)
            wallet.GetKeyPool().ReturnKey(nPoolIndex);

        nPoolIndex = -1;
        vchPubKey.clear();
    }

}
