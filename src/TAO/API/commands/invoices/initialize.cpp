/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/invoices.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Standard initialization function. */
        void Invoices::Initialize()
        {
            /* Populate our standard objects. */
            /* Populate our invoice standard. */
            mapStandards["invoice"] = Standard
            (
                /* Lambda expression to determine object standard. */
                [](const TAO::Register::Object& objCheck)
                {
                    /* Check for correct state type. */
                    if(objCheck.nType != TAO::Register::REGISTER::READONLY)
                        return false;

                    /* Reset read position. */
                    objCheck.nReadPos = 0;

                    /* Find our leading type byte. */
                    uint16_t nType;
                    objCheck >> nType;

                    /* Cleanup our read position. */
                    objCheck.nReadPos = 0;

                    /* Check that this matches our user type. */
                    if(nType != USER_TYPES::INVOICE)
                        return false;

                    return true;
                }
            );


            /* Handle for all CREATE operations. */
            mapFunctions["create"] = Function
            (
                std::bind
                (
                    &Invoices::Create,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , TAO::Ledger::StartTransactionTimelock(2)
            );

            /* Handle for all GET operations. */
            mapFunctions["get"] = Function
            (
                std::bind
                (
                    &Invoices::Get,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , TAO::Ledger::StartTransactionTimelock(2)
            );

            /* Handle for all PAY operations. */
            mapFunctions["pay"] = Function
            (
                std::bind
                (
                    &Invoices::Pay,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , TAO::Ledger::StartTransactionTimelock(2)
            );

            /* Handle for all CANCEL operations. */
            mapFunctions["cancel"] = Function
            (
                std::bind
                (
                    &Invoices::Cancel,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , TAO::Ledger::StartTransactionTimelock(2)
            );

            /* Handle for all LIST operations. */
            mapFunctions["list"] = Function
            (
                std::bind
                (
                    &Invoices::List,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , TAO::Ledger::StartTransactionTimelock(2)
            );

            /* Handle for all LIST operations. */
            mapFunctions["list/history"] = Function
            (
                std::bind
                (
                    &Invoices::History,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , TAO::Ledger::StartTransactionTimelock(2)
            );
        }
    }
}
