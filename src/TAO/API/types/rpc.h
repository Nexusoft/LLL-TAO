/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_RPC_H
#define NEXUS_TAO_API_TYPES_RPC_H

#include <TAO/API/types/base.h>

namespace TAO::API
{

    /** RPC API Class
     *
     *  Manages the function pointers for all RPC commands.
     *
     **/
    class RPC : public Base
    {
    public:
        RPC() {}

        void Initialize();

        std::string GetName() const
        {
            return "RPC";
        }
    };

    /** The instance of RPC commands. */
    extern RPC* RPCCommands;
}

#endif
