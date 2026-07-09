/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_LOCAL_MINED_BLOCK_TRACKER_H
#define NEXUS_TAO_LEDGER_INCLUDE_LOCAL_MINED_BLOCK_TRACKER_H

#include <TAO/Ledger/types/tritium.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /** TrackLocalMinedBlock
         *
         *  Records a locally mined block after it has been accepted, so a later
         *  reorganization can identify that this node orphaned its own block.
         *
         **/
        void TrackLocalMinedBlock(const TAO::Ledger::TritiumBlock& block);
    }
}

#endif
