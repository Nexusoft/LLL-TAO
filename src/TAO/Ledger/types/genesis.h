/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_GENESIS_H
#define NEXUS_TAO_LEDGER_TYPES_GENESIS_H

#include <LLC/types/uint1024.h>
/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Genesis
         *
         *  An object that keeps track of a genesis address type.
         *
         **/
        class Genesis : public uint256_t
        {
        public:

            /** Default constructor. **/
            Genesis();


            /** Copy Constructor */
            Genesis(const uint256_t& hashAddress, bool fSet = false);


            /** Assignment operator.
             *
             *  @param[in] gen Address to assign this to.
             *
             **/
            Genesis& operator=(const Genesis& gen);


            /** IsValid
             *
             *  Check if genesis has a valid indicator byte.
             *
             *  @return True if type has valid header byte.
             *
             **/
            bool IsValid() const;
        };
    }
}

#endif
