/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Util/include/json.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>
#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** CheckAddress
         *
         *  Determines whether a Base58 encoded value is a valid register address by checking type.
         *
         *  @param[in] strValue The value to check
         *
         *  @return True if the value can be decoded into a valid Register::Address with a valid Type.
         **/
        bool CheckAddress(const std::string& strValue);


        /** CheckMature
         *
         *  Utilty method that checks that the signature chain is mature and can therefore create new transactions.
         *  Throws an appropriate APIException if it is not mature
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to check
         *
         **/
        void CheckMature(const uint256_t& hashGenesis);




    }
}
