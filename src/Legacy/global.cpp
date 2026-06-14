/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/ambassador.h>
#include <Legacy/include/global.h>

#include <TAO/Ledger/include/chainstate.h>

namespace Legacy
{
    /*  Instantiate global instances of the API. */
    bool Initialize()
    {
        /* Client mode doesn't need to initialize the wallet. */
        if(config::fClient.load())
            return true;

        /* Initialize the scripts for legacy mode. */
        Legacy::InitializeScripts();

        return true;
    }


    /*  Delete global instances for legacy subsystems. */
    void Shutdown()
    {
        /* Client mode doesn't need to initialize the wallet. */
        if(config::fClient.load())
            return;
    }
}
