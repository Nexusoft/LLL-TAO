/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/commands/operators.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Names::Initialize()
    {
        /* Handle for the ARRAY operator. */
        mapOperators["array"] = Operator
        (
            std::bind
            (
                &Operators::Array,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        /* Populate our asset standard. */
        mapStandards["name"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::NAME;
            }
        );

        /* Populate our raw standard. */
        mapStandards["namespace"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::NAMESPACE;
            }
        );

        /* Populate our any standard. */
        mapStandards["any"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(CheckObject("name", rObject))
                    return true;

                /* Check for correct state type. */
                if(CheckObject("namespace", rObject))
                    return true;

                return false;
            }
        );


        /* Handle for all CLAIM operations. */
        mapFunctions["claim"] = Function
        (
            std::bind
            (
                &Templates::Claim,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Templates::Create,
                std::placeholders::_1,
                std::placeholders::_2,

                /* Our accepted formats for this command-set. */
                "standard",
                USER_TYPES::STANDARD, //the enumerated value for states
                "standard"         //the default format
            )
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
        );

        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Templates::List,
                std::placeholders::_1,
                std::placeholders::_2
            )
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
        );

        /* Handle for all TRANSFER operations. */
        mapFunctions["transfer"] = Function
        (
            std::bind
            (
                &Templates::Transfer,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all UPDATE operations. */
        mapFunctions["update"] = Function
        (
            std::bind
            (
                &Templates::Update,
                std::placeholders::_1,
                std::placeholders::_2,

                /* Our accepted formats for this command-set. */
                "standard",
                USER_TYPES::STANDARD, //the enumerated value for states
                "standard"            //the default format
            )
        );

        /* DEPRECATED */
        mapFunctions["list/name/history"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use names/history/name command instead"
        );

        /* DEPRECATED */
        mapFunctions["list/namespace/history"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use names/history/namespace command instead"
        );

        //mapFunctions["create/name"]             = Function(std::bind(&Names::Create,    this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["update/name"]             = Function(std::bind(&Names::UpdateName,    this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["get/name"]                = Function(std::bind(&Names::Get,       this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["transfer/name"]           = Function(std::bind(&Names::TransferName,  this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["claim/name"]              = Function(std::bind(&Names::ClaimName,  this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["list/name/history"]       = Function(std::bind(&Names::NameHistory,   this, std::placeholders::_1, std::placeholders::_2));

        //mapFunctions["create/namespace"]        = Function(std::bind(&Names::CreateNamespace,    this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["get/namespace"]           = Function(std::bind(&Names::GetNamespace,       this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["transfer/namespace"]      = Function(std::bind(&Names::TransferNamespace,  this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["claim/namespace"]         = Function(std::bind(&Names::ClaimNamespace,  this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["list/namespace/history"]  = Function(std::bind(&Names::NamespaceHistory,   this, std::placeholders::_1, std::placeholders::_2));
    }
}
