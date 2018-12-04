/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <memory>
#include <string>
#include <vector>

#include <LLP/include/network.h>

#include <TAO/Legacy/wallet/keypoolentry.h>

#include <Util/include/serialize.h>


namespace Legacy
{

    /* GetReservedKey */
    vector<uint8_t> CReserveKey::GetReservedKey()
    {
        if (nPoolIndex == -1)
        {
            // Don't have a reserved key in this instance, yet, so need to reserve one
            CKeyPoolEntry keypoolEntry;
            wallet.GetKeyPool().ReserveKeyFromPool(nPoolIndex, keypoolEntry);

            if (nPoolIndex != -1)
                vchPubKey = keypoolEntry.vchPubKey;
            else
            {
                printf("CReserveKey::GetReservedKey(): Warning: using default key instead of a new key, top up your keypool.");
                vchPubKey = wallet.GetDefaultKey();
            }
        }

        assert(!vchPubKey.empty());
        return vchPubKey;
    }


    /* KeepKey */
    void CReserveKey::KeepKey()
    {
        if (nPoolIndex != -1)
            wallet.GetKeyPool().KeepKey(nPoolIndex);

        nPoolIndex = -1;
        vchPubKey.clear();
    }


    /* ReturnKey */
    void CReserveKey::ReturnKey()
    {
        if (nPoolIndex != -1)
            wallet.GetKeyPool().ReturnKey(nPoolIndex);

        nPoolIndex = -1;
        vchPubKey.clear();
    }

}
