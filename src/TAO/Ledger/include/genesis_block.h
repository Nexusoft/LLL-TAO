/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_GENESIS_H
#define NEXUS_TAO_LEDGER_INCLUDE_GENESIS_H

#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** LegacyGenesis
         *
         *  Creates the legacy genesis block.
         *
         **/
        const BlockState LegacyGenesis();


        /** TritiumGenesis
         *
         *  Creates the tritium genesis block which is the first Tritium block mined.
         *
         **/
        const BlockState TritiumGenesis();
    }
}

#endif
