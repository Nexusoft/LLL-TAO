/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/walletkey.h>

namespace Legacy
{
    /* Default Constructor */
    WalletKey::WalletKey(const uint64_t nExpires)
    : vchPrivKey   ( )
    , nTimeCreated (nExpires ? runtime::unifiedtimestamp() : 0)
    , nTimeExpires (nExpires)
    , strComment   ( )
    {
    }


    /* Copy Constructor. */
    WalletKey::WalletKey(const WalletKey& key)
    : vchPrivKey   (key.vchPrivKey)
    , nTimeCreated (key.nTimeCreated)
    , nTimeExpires (key.nTimeExpires)
    , strComment   (key.strComment)
    {
    }


    /* Move Constructor. */
    WalletKey::WalletKey(WalletKey&& key) noexcept
    : vchPrivKey   (std::move(key.vchPrivKey))
    , nTimeCreated (std::move(key.nTimeCreated))
    , nTimeExpires (std::move(key.nTimeExpires))
    , strComment   (std::move(key.strComment))
    {
    }


    /* Copy Assignment. */
    WalletKey& WalletKey::operator=(const WalletKey& key)
    {
        vchPrivKey   = key.vchPrivKey;
        nTimeCreated = key.nTimeCreated;
        nTimeExpires = key.nTimeExpires;
        strComment   = key.strComment;

        return *this;
    }


    /* Move Assignment. */
    WalletKey& WalletKey::operator=(WalletKey&& key) noexcept
    {
        vchPrivKey   = std::move(key.vchPrivKey);
        nTimeCreated = std::move(key.nTimeCreated);
        nTimeExpires = std::move(key.nTimeExpires);
        strComment   = std::move(key.strComment);

        return *this;
    }


    /* Default Destructor */
    WalletKey::~WalletKey()
    {
    }
}
