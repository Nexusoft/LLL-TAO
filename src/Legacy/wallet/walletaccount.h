/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_WALLETACCOUNT_H
#define NEXUS_LEGACY_WALLET_WALLETACCOUNT_H

#include <vector>

#include <Util/templates/serialize.h>

namespace Legacy
{

    /** @class Account
     *
     *  Account information.
     *
     *  A wallet account contains the public key value for an account/Nexus address
     *  (which is not stored here). These are used to store public keys in the wallet database.
     *
     *  Database key is acc<account>
     **/
    class Account
    {
    public:
        /** Public key for the account **/
        std::vector<uint8_t> vchPubKey;


        /** Constructor
         *
         *  Calls SetNull() to initialize the account.
         *
         **/
        Account()
        {
            SetNull();
        }


        /** Destructor **/
        ~Account()
        {
        }


        /** SetNull
         *
         *  Clears the current public key value.
         *
         **/
        void SetNull()
        {
            vchPubKey.clear();
        }


        IMPLEMENT_SERIALIZE
        (
            if(!(nSerType & SER_GETHASH))
                READWRITE(nSerVersion);
            READWRITE(vchPubKey);
      )
    };

}

#endif
