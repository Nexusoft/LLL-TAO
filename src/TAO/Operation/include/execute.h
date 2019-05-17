/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Man often becomes what he believes himself to be." - Mahatma Gandhi

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

#include <cstdint>


/* Global TAO namespace. */
namespace TAO
{

    /** Forward declatations **/
    namespace Ledger
    {
        class Transaction;
    }

    /* Operation Layer namespace. */
    namespace Operation
    {

        //TODO: stress test and try to break operations transactions. Detect if parameters are sent in wrong order giving deliberate failures

        /** Execute
         *
         *  Executes a given operation byte sequence.
         *
         *  @param[in] regDB The register database to execute on.
         *  @param[in] hashOwner The owner executing the register batch.
         *
         *  @return True if operations executed successfully, false otherwise.
         *
         **/
        bool Execute(TAO::Ledger::Transaction& tx, uint8_t nFlags);
    }
}

#endif
