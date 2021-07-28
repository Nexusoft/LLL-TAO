/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/system.h>

#include <Util/include/json.h>

#include <LLP/include/lisp.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Queries the lisp api and returns the EID's for this node. */
    encoding::json System::LispEIDs(const encoding::json& jParams, const bool fHelp)
    {
        /* Build our return object. */
        encoding::json jRet = encoding::json::array();

        /* Query the LISP API. */
        std::map<std::string, LLP::EID> mapEIDs = LLP::GetEIDs();
        for(const auto& rEID : mapEIDs)
        {
            /* Build our EID object. */
            encoding::json jEID =
            {
                { "instance-id", rEID.second.strInstanceID },
                { "eid"        , rEID.second.strAddress    }
            };

            /* Add our according RLOC's. */
            encoding::json jRLOCs = encoding::json::array();
            for(const auto& rRLOC : rEID.second.vRLOCs)
            {
                /* Add our RLOC data to ret. */
                const encoding::json jRLOC =
                {
                    { "interface", rRLOC.strInterface      },
                    { "rloc-name", rRLOC.strRLOCName       },
                    { "rloc"     , rRLOC.strTranslatedRLOC }
                };

                jRLOCs.push_back(jRLOC);
            }

            /* Add RLOC's to EID object. */
            jEID["rlocs"] = jRLOCs;
            jRet.push_back(jEID);
        }

        return jRet;
    }
}
