/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/lisp.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <LLP/types/corenode.h>
#include <LLP/include/base_address.h>

#include <LLP/include/lisp.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** The LISP API instance. **/
        Lisp lisp;

        /* Standard initialization function. */
        void Lisp::Initialize()
        {
            mapFunctions["eids"]              = Function(std::bind(&Lisp::EIDs,    this, std::placeholders::_1, std::placeholders::_2));
        }

        

        /* Queries the lisp api and returns the EID's for this node. */
        json::json Lisp::EIDs(const json::json& params, bool fHelp)
        {
            json::json jsonRet;

            if( LLP::EIDS.size() > 0)
            {
                json::json jsonEIDs = json::json::array();

                for(const auto& eid : LLP::EIDS)
                {
                    json::json jsonEID;

                    jsonEID["instance-id"] = eid.second.strInstanceID;
                    jsonEID["eid"] = eid.second.strAddress;

                    json::json jsonRLOCs = json::json::array();

                    for(const auto& rloc : eid.second.vRLOCs)
                    {
                        json::json jsonRLOC;

                        jsonRLOC["interface"] = rloc.strInterface;
                        jsonRLOC["rloc-name"] = rloc.strRLOCName;
                        jsonRLOC["rloc"] = rloc.strTranslatedRLOC;

                        jsonRLOCs.push_back(jsonRLOC);
                    }

                    jsonEID["rlocs"] = jsonRLOCs;

                    jsonEIDs.push_back(jsonEID);
                }

                jsonRet["eids"] = jsonEIDs;
            }

            return jsonRet;

        }

    
    }

}
