/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/invoices.h>
#include <TAO/API/types/objects.h>

#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Gets the history of an item. */
        json::json Invoices::History(const json::json& params, bool fHelp)
        {
            /* First ensure that transaction version 2 active, as the conditions required for invoices were not enabled until v2 */
            const uint32_t nCurrent = TAO::Ledger::CurrentTransactionVersion();
            if(nCurrent < 2 || (nCurrent == 2 && !TAO::Ledger::TransactionVersionActive(runtime::unifiedtimestamp(), 2)))
                throw APIException(-254, "Invoices API not yet active.");

            return Objects::History(params, TAO::Register::OBJECTS::NONSTANDARD, std::string("Invoice"));
        }
    }
}
