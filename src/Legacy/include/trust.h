/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_INCLUDE_TRUST_H
#define NEXUS_LEGACY_INCLUDE_TRUST_H

#include <Legacy/types/trustkey.h>

namespace Legacy
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

    /** FindMigratedTrustKey
     *
     *  Extract the trust key being migrated from a Legacy migration transaction.
     *
     *  @param[in] tx The Legacy transaction sending from trust key to trust account register
     *  @param[out] trustKey The trust key being migrated
     *
     *  @return True if the trust key was found
     *          False if not a migration tx or could not retrieve trust key from tx
     *
     **/
    bool FindMigratedTrustKey(const Transaction& tx, TrustKey& trustKey);
}

#endif
