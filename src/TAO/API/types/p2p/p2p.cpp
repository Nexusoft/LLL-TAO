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
                                memory::atomic_ptr<LLP::P2PNode>& connection)
        {
            for(uint16_t nThread = 0; nThread < LLP::P2P_SERVER->MAX_THREADS; ++nThread)
            {
                /* Get the data threads. */
                LLP::DataThread<LLP::P2PNode>* dt = LLP::P2P_SERVER->DATA_THREADS[nThread];

                /* Lock the data thread. */
                uint16_t nSize = static_cast<uint16_t>(dt->CONNECTIONS->size());

                /* Loop through connections in data thread. */
                for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    /* Get the connection */
                    auto& conn = dt->CONNECTIONS->at(nIndex);

                    /* Skip over inactive connections. */
                    if(conn != nullptr && conn->Connected())
                    {
                        /* Check that the connection matches th criteria  */
                        if(conn->Matches(strAppID, hashGenesis, hashPeer))
                        {
                            /* Copy the internal pointer to the atomic to return */
                            connection.store(conn.load());
                            return true;

                        }
                    }
                }
            }

            /* Not found */
            return false;
        }

    }
}
