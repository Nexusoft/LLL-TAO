/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_TRUST_H
#define NEXUS_TAO_LEDGER_INCLUDE_TRUST_H

#include <TAO/Ledger/types/trustkey.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** GetLastTrust
         *
         *  Find the last trust block of given key.
         *
         *  @param[in] trustKey The trust key to search for
         *  @param[out] state The trust key block found.
         *
         *  @return True if the trust block was found, false otherwise.
         *
         **/
        bool GetLastTrust(const TrustKey& trustKey, TAO::Ledger::BlockState& state);


        /** FindGenesis
         *
         *  Find the genesis block of given trust key.
         *
         *  @param[in] cKey The trust key to search for.
         *  @param[out] trustKey The trust that was found
         *  @param[out] hashTrustBlock The trust key block found.
         *
         *  @return True if the trust block was found, false otherwise.
         *
         **/
        bool FindGenesis(const uint576_t& cKey, const uint1024_t& hashTrustBlock, TrustKey& trustKey);
    }
}

#endif
