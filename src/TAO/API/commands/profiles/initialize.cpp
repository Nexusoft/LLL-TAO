/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/profiles.h>

#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/count.h>
#include <TAO/API/types/operators/max.h>
#include <TAO/API/types/operators/mean.h>
#include <TAO/API/types/operators/min.h>
#include <TAO/API/types/operators/mode.h>
#include <TAO/API/types/operators/floor.h>
#include <TAO/API/types/operators/sum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Profiles::Initialize()
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

        /* Handle for the COUNT operator. */
        mapOperators["count"] = Operator
        (
            std::bind
            (
                &Operators::Count,
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

        /* Handle for the MAX operator. */
        mapOperators["max"] = Operator
        (
            std::bind
            (
                &Operators::Max,
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

        /* Handle for the MIN operator. */
        mapOperators["min"] = Operator
        (
            std::bind
            (
                &Operators::Min,
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
        

        /* Populate our MASTER standard. */
        mapStandards["master"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return false;
            }
        );


        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Profiles::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all transactions operations. */
        mapFunctions["notifications"] = Function
        (
            std::bind
            (
                &Profiles::Notifications,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all transactions operations. */
        mapFunctions["status"] = Function
        (
            std::bind
            (
                &Profiles::Status,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all transactions operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Profiles::Transactions,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
