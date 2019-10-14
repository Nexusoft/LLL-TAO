/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_ACCOUNT_H
#define NEXUS_LEGACY_TYPES_ACCOUNT_H

#include <Util/templates/serialize.h>

#include <vector>


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


        //serialization methods
        IMPLEMENT_SERIALIZE
        (
            if(!(nSerType & SER_GETHASH))
                READWRITE(nSerVersion);

            READWRITE(vchPubKey);
        )


        /** The default constructor. **/
        Account();


        /** Copy Constructor. **/
        Account(const Account& account);


        /** Move Constructor. **/
        Account(Account&& account) noexcept;


        /** Copy Assignment. **/
        Account& operator=(const Account& account);


        /** Move Assignment. **/
        Account& operator=(Account&& account) noexcept;


        /** Default Destructor **/
        ~Account();


        /** SetNull
         *
         *  Clears the current public key value.
         *
         **/
        void SetNull();
    };

}

#endif
