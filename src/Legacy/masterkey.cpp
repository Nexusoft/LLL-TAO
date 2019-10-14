/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/masterkey.h>

namespace Legacy
{
    /* The default constructor. */
    MasterKey::MasterKey()
    : vchCryptedKey                ( )
    , vchSalt                      ( )
    , nDerivationMethod            (0)
    , nDeriveIterations            (25000)
    , vchOtherDerivationParameters ( )
    {
    }


    /* Copy Constructor. */
    MasterKey::MasterKey(const MasterKey& key)
    : vchCryptedKey                (key.vchCryptedKey)
    , vchSalt                      (key.vchSalt)
    , nDerivationMethod            (key.nDerivationMethod)
    , nDeriveIterations            (key.nDeriveIterations)
    , vchOtherDerivationParameters (key.vchOtherDerivationParameters)
    {
    }


    /* Move Constructor. */
    MasterKey::MasterKey(MasterKey&& key) noexcept
    : vchCryptedKey                (std::move(key.vchCryptedKey))
    , vchSalt                      (std::move(key.vchSalt))
    , nDerivationMethod            (std::move(key.nDerivationMethod))
    , nDeriveIterations            (std::move(key.nDeriveIterations))
    , vchOtherDerivationParameters (std::move(key.vchOtherDerivationParameters))
    {
    }


    /* Copy Assignment. */
    MasterKey& MasterKey::operator=(const MasterKey& key)
    {
        vchCryptedKey                = key.vchCryptedKey;
        vchSalt                      = key.vchSalt;
        nDerivationMethod            = key.nDerivationMethod;
        nDeriveIterations            = key.nDeriveIterations;
        vchOtherDerivationParameters = key.vchOtherDerivationParameters;

        return *this;
    }


    /* Move Assignment. */
    MasterKey& MasterKey::operator=(MasterKey&& key) noexcept
    {
        vchCryptedKey                = std::move(key.vchCryptedKey);
        vchSalt                      = std::move(key.vchSalt);
        nDerivationMethod            = std::move(key.nDerivationMethod);
        nDeriveIterations            = std::move(key.nDeriveIterations);
        vchOtherDerivationParameters = std::move(key.vchOtherDerivationParameters);

        return *this;
    }


    /* Default Destructor */
    MasterKey::~MasterKey()
    {
    }
}
