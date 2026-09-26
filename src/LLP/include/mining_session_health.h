/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_SESSION_HEALTH_H
#define NEXUS_LLP_INCLUDE_MINING_SESSION_HEALTH_H

#include <TAO/API/types/authentication.h>
#include <TAO/Ledger/types/pinunlock.h>

namespace LLP
{

    /** IsDefaultSessionReady
     *
     *  Non-throwing check for whether SESSION::DEFAULT exists and is unlocked
     *  for mining.  Callers can use this before entering block-creation code
     *  that would otherwise throw "Session not found" if the default session
     *  has not been established (e.g. node started without -autologin or the
     *  wallet was not unlocked for mining via the API).
     *
     *  @return true if the default session is present and unlocked for mining.
     *
     **/
    inline bool IsDefaultSessionReady()
    {
        try
        {
            const uint256_t hashDefault = uint256_t(TAO::API::Authentication::SESSION::DEFAULT);

            /* Check that the session is unlocked for mining.
             * Unlocked() may throw "Session not found" if SESSION::DEFAULT was
             * never created, so we wrap the entire check in a try-catch. */
            if(!TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING, hashDefault))
                return false;

            /* Accessing Credentials() throws "Session not found" if the session
             * has been removed since the Unlocked() check above. */
            TAO::API::Authentication::Credentials(hashDefault);
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

} // namespace LLP

#endif
