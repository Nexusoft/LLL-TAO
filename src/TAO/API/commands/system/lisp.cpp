/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/system.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <LLP/types/apinode.h>
#include <LLP/include/base_address.h>

#include <LLP/include/lisp.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Queries the lisp api and returns the EID's for this node. */
        encoding::json System::LispEIDs(const encoding::json& params, const bool fHelp)
        {
            encoding::json jsonEIDs = encoding::json::array();

            std::map<std::string, LLP::EID> mapEIDs = LLP::GetEIDs();
            if(mapEIDs.size() > 0)
            {
                for(const auto& eid : mapEIDs)
                {
                    encoding::json jsonEID;

                    jsonEID["instance-id"] = eid.second.strInstanceID;
                    jsonEID["eid"] = eid.second.strAddress;

                    encoding::json jsonRLOCs = encoding::json::array();

                    for(const auto& rloc : eid.second.vRLOCs)
                    {
                        encoding::json jsonRLOC;

                        jsonRLOC["interface"] = rloc.strInterface;
                        jsonRLOC["rloc-name"] = rloc.strRLOCName;
                        jsonRLOC["rloc"] = rloc.strTranslatedRLOC;

                        jsonRLOCs.push_back(jsonRLOC);
                    }

                    jsonEID["rlocs"] = jsonRLOCs;

                    jsonEIDs.push_back(jsonEID);
                }
            }

            return jsonEIDs;

        }


    }

}
