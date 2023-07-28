/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Invoices::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Populate our invoice standard. */
        mapStandards["invoice"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(rObject.nType != TAO::Register::REGISTER::READONLY)
                    return false;

                /* Check that this matches our user type. */
                return GetStandardType(rObject) == USER_TYPES::INVOICE;
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "readonly"
        );

        /* Subset of invoice standard, to find outstanding invoices. */
        mapStandards["outstanding"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(!CheckObject("invoice", rObject))
                    return false;

                return (rObject.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM);
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "readonly"
        );

        /* Subset of invoice standard, to find outstanding invoices. */
        mapStandards["paid"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(!CheckObject("invoice", rObject))
                    return false;

                /* Get our recipient. */
                encoding::json jInvoice = RegisterToJSON(rObject);
                if(jInvoice.find("json") == jInvoice.end())
                    return false;

                /* Check for recipient now. */
                if(jInvoice["json"].find("recipient") == jInvoice["json"].end())
                    return false;

                return (rObject.hashOwner == uint256_t(jInvoice["json"]["recipient"].get<std::string>()));
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "readonly"
        );

        /* Subset of invoice standard, to find outstanding invoices. */
        mapStandards["cancelled"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(!CheckObject("invoice", rObject))
                    return false;

                /* Get our recipient. */
                encoding::json jInvoice = RegisterToJSON(rObject);
                if(jInvoice.find("json") == jInvoice.end())
                    return false;

                /* Check for recipient now. */
                if(jInvoice["json"].find("recipient") == jInvoice["json"].end())
                    return false;

                return (rObject.hashOwner != uint256_t(jInvoice["json"]["recipient"].get<std::string>()));
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "readonly"
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
                &Templates::Get,
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
                &Templates::List,
                std::placeholders::_1,
                true
            )
            , TAO::Ledger::StartTransactionTimelock(2)
        );

        /* Handle for all HISTORY operations. */
        mapFunctions["history"] = Function
        (
            std::bind
            (
                &Templates::History,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
        );

        /* Handle for all TRANSACTIONS operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Templates::Transactions,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
        );

        /* DEPRECATED. */
        mapFunctions["list/history"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
            , version::get_version(5, 1, 0)
            , "please use finance/transactions/account instead"
        );
    }
}
