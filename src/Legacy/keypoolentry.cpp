/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/keypoolentry.h>
#include <Util/include/runtime.h>

namespace Legacy
{

    /* The default constructor. */
    KeyPoolEntry::KeyPoolEntry()
    : nTime     (runtime::unifiedtimestamp())
    , vchPubKey ( )
    {
    }


    /* Copy Constructor. */
    KeyPoolEntry::KeyPoolEntry(const KeyPoolEntry& entry)
    : nTime     (entry.nTime)
    , vchPubKey (entry.vchPubKey)
    {
    }


    /* Move Constructor. */
    KeyPoolEntry::KeyPoolEntry(KeyPoolEntry&& entry) noexcept
    : nTime     (std::move(entry.nTime))
    , vchPubKey (std::move(entry.vchPubKey))
    {
    }


    /* Copy Assignment. */
    KeyPoolEntry& KeyPoolEntry::operator=(const KeyPoolEntry& entry)
    {
        nTime     = entry.nTime;
        vchPubKey = entry.vchPubKey;

        return *this;
    }


    /* Move Assignment. */
    KeyPoolEntry& KeyPoolEntry::operator=(KeyPoolEntry&& entry) noexcept
    {
        nTime     = std::move(entry.nTime);
        vchPubKey = std::move(entry.vchPubKey);

        return *this;
    }


    /* Default Destructor */
    KeyPoolEntry::~KeyPoolEntry()
    {
    }


    /* Constructor */
    KeyPoolEntry::KeyPoolEntry(const std::vector<uint8_t>& vchPubKeyIn)
    : nTime     (runtime::unifiedtimestamp())
    , vchPubKey (vchPubKeyIn)
    {
    }

}
