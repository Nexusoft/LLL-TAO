/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <LLP/include/global.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Returns a connection for the specified app / genesis / peer. */
        bool P2P::get_connection(const std::string& strAppID, 
                                const uint256_t& hashGenesis, 
                                const uint256_t& hashPeer,
                                std::shared_ptr<LLP::P2PNode> &connection)
        {
            /* GEt the matching connection from the server */
            std::shared_ptr<LLP::P2PNode>& pConnection = LLP::P2P_SERVER->GetSpecificConnection(strAppID, hashGenesis, hashPeer);

            /* Check it is valid */
            if(!pConnection)
                return false;

            else
            {
                /* Copy the internal pointer to the atomic to return */
                connection = pConnection;
                return true;
            }
        }
    }
}
