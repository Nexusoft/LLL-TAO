/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/users/types/users.h>

#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/mean.h>
#include <TAO/API/types/operators/mode.h>
#include <TAO/API/types/operators/floor.h>
#include <TAO/API/types/operators/sum.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

namespace TAO::API
{
    /* Standard initialization function. */
    void Users::Initialize()
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

        /* Handle for the FLOOR operator. */
        mapOperators["floor"] = Operator
        (
            std::bind
            (
                &Operators::Floor,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for the MEAN operator. */
        mapOperators["mean"] = Operator
        (
            std::bind
            (
                &Operators::Mean,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for the MODE operator. */
        mapOperators["mode"] = Operator
        (
            std::bind
            (
                &Operators::Mode,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for the SUM operator. */
        mapOperators["sum"] = Operator
        (
            std::bind
            (
                &Operators::Sum,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        mapFunctions["create/user"]              = Function(std::bind(&Users::Create,        this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["login/user"]               = Function(std::bind(&Users::Login,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["logout/user"]              = Function(std::bind(&Users::Logout,        this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["lock/user"]                = Function(std::bind(&Users::Lock,          this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["unlock/user"]              = Function(std::bind(&Users::Unlock,        this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["update/user"]              = Function(std::bind(&Users::Update,        this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["recover/user"]             = Function(std::bind(&Users::Recover,       this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/status"]               = Function(std::bind(&Users::Status,        this, std::placeholders::_1, std::placeholders::_2));

        mapFunctions["list/transactions"]        = Function(std::bind(&Users::Transactions,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/notifications"]       = Function(std::bind(&Users::Notifications, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["process/notifications"]    = Function(std::bind(&Users::ProcessNotifications, this, std::placeholders::_1, std::placeholders::_2));

        mapFunctions["list/processed"]           = Function(std::bind(&Users::Processed,      this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["clear/processed"]          = Function(std::bind(&Users::Clear,      this, std::placeholders::_1, std::placeholders::_2));

        mapFunctions["load/session"]             = Function(std::bind(&Users::Load,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["save/session"]             = Function(std::bind(&Users::Save,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["has/session"]              = Function(std::bind(&Users::Has,         this, std::placeholders::_1, std::placeholders::_2));


        //mapFunctions["list/items"]               = Function(std::bind(&Users::Items,         this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["list/tokens"]              = Function(std::bind(&Users::Tokens,        this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["list/accounts"]            = Function(std::bind(&Users::Accounts,      this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["list/names"]               = Function(std::bind(&Users::Names,         this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["list/namespaces"]          = Function(std::bind(&Users::Namespaces,    this, std::placeholders::_1, std::placeholders::_2));


        /* DEPRECATED */
        mapFunctions["list/assets"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use assets/list/assets instead"
        );

        /* DEPRECATED */
        mapFunctions["list/invoices"] = Function
        (
            std::bind
            (
                &Templates::List,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use invoices/list/invoices command instead"
        );

        /* DEPRECATED */
        mapFunctions["list/items"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use supply/list/items instead"
        );

        /* DEPRECATED */
        mapFunctions["list/tokens"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/list/tokens instead"
        );

        /* DEPRECATED */
        mapFunctions["list/accounts"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/list/accounts instead"
        );

        /* DEPRECATED */
        mapFunctions["list/names"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use names/list/names instead"
        );

        /* DEPRECATED */
        mapFunctions["list/namespaces"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use names/list/namespaces instead"
        );
    }
}
