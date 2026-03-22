/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/exception.h>

#include <TAO/API/types/operator.h>
#include <TAO/API/types/operators/initialize.h>
#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/count.h>
#include <TAO/API/types/operators/floor.h>
#include <TAO/API/types/operators/lookup.h>
#include <TAO/API/types/operators/max.h>
#include <TAO/API/types/operators/mean.h>
#include <TAO/API/types/operators/min.h>
#include <TAO/API/types/operators/mode.h>
#include <TAO/API/types/operators/sum.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Track what operators are currently supported by initialize. */
    const std::map<std::string, Operator> Operators::mapOperators =
    {
        /* Handle for the ARRAY operator. */
        { "array", Operator
            (
                std::bind
                (
                    &Operators::Array,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the COUNT operator. */
        { "count", Operator
            (
                std::bind
                (
                    &Operators::Count,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the FLOOR operator. */
        { "floor", Operator
            (
                std::bind
                (
                    &Operators::Floor,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the LOOKUP operator. */
        { "lookup", Operator
            (
                std::bind
                (
                    &Operators::Lookup,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the MAX operator. */
        { "max", Operator
            (
                std::bind
                (
                    &Operators::Max,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the MEAN operator. */
        { "mean", Operator
            (
                std::bind
                (
                    &Operators::Mean,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the MIN operator. */
        { "min", Operator
            (
                std::bind
                (
                    &Operators::Min,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },

        /* Handle for the SUM operator. */
        { "sum", Operator
            (
                std::bind
                (
                    &Operators::Sum,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        },
    };
}
