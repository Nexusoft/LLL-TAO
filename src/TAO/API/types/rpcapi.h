/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_RPCAPI_H
#define NEXUS_TAO_API_TYPES_RPCAPI_H

#include <functional>
#include <Util/include/json.h>
#include <TAO/API/types/jsonapibase.h>

namespace TAO::API::RPC
{
    class RPCAPI : public JSONAPIBase
    {
    public:
        /** Abstract method implementations**/
        virtual void Initialize() ;
        virtual std::string GetAPIName() const {return "RPC";};
        virtual nlohmann::json HandleJSONAPIMethod(std::string strMethod, nlohmann::json jsonParameters);

        /* API commands*/
        nlohmann::json TestFunc(bool fHelp, nlohmann::json jsonParameters);


    };
}


#endif
