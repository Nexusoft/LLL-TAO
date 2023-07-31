/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_WALLETKEY_H
#define NEXUS_LEGACY_TYPES_WALLETKEY_H

#include <LLC/include/eckey.h>

#include <Util/templates/serialize.h>

namespace Legacy
{

    /** @class WalletKey
     *
     *  Class to hold unencrypted private key binary data.
     *
     *  @deprecated This class is no longer used or written to the wallet database. It
     *              is supported for backward compatibility, so values can be read
     *              that were previously written into the database.
     *
     *  Database key is wkey<publickey>
     **/
    class WalletKey
    {
    public:

        /** Unencrypted private key data **/
        LLC::CPrivKey vchPrivKey;


        /** timestamp when this wallet key was created. Only relevant if nTimeExpires has a value **/
        uint64_t nTimeCreated;


        /** Number of seconds after nTimeCreated that this wallet key expires **/
        uint64_t nTimeExpires;


        /** Space to include comments **/
        std::string strComment;


        IMPLEMENT_SERIALIZE
        (
            if(!(nSerType & SER_GETHASH))
                READWRITE(nSerVersion);

            READWRITE(vchPrivKey);
            READWRITE(nTimeCreated);
            READWRITE(nTimeExpires);
            READWRITE(strComment);
        )
        

        /** Default Constructor **/
        WalletKey(const uint64_t nExpires = 0);


        /** Copy Constructor. **/
        WalletKey(const WalletKey& key);


        /** Move Constructor. **/
        WalletKey(WalletKey&& key) noexcept;


        /** Copy Assignment. **/
        WalletKey& operator=(const WalletKey& key);


        /** Move Assignment. **/
        WalletKey& operator=(WalletKey&& key) noexcept;


        /** Default Destructor **/
        ~WalletKey();

    };

}

#endif
