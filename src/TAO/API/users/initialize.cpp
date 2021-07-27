/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/users/types/users.h>

#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands/operators.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

namespace TAO::API
{
    /* Standard initialization function. */
    void Users::Initialize()
    {
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


        /* Handle for list/invoices operations. */
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
        mapFunctions["list/assets"]              = Function(std::bind(&Users::Assets,        this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/items"]               = Function(std::bind(&Users::Items,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/tokens"]              = Function(std::bind(&Users::Tokens,        this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/accounts"]            = Function(std::bind(&Users::Accounts,      this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/names"]               = Function(std::bind(&Users::Names,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/namespaces"]          = Function(std::bind(&Users::Namespaces,    this, std::placeholders::_1, std::placeholders::_2));

        mapFunctions["list/processed"]           = Function(std::bind(&Users::Processed,      this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["clear/processed"]          = Function(std::bind(&Users::Clear,      this, std::placeholders::_1, std::placeholders::_2));

        mapFunctions["load/session"]             = Function(std::bind(&Users::Load,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["save/session"]             = Function(std::bind(&Users::Save,         this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["has/session"]              = Function(std::bind(&Users::Has,         this, std::placeholders::_1, std::placeholders::_2));
    }
}
