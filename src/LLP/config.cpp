/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/config.h>

#include <limits>

namespace LLP
{

    /** Constructor  **/
    ServerConfig::ServerConfig()
    : nPort         ()
    , nSSLPort      ()
    , fListen       (true)
    , fRemote       (false)
    , fSSL          (false)
    , fSSLRequired   (false)
    , nMaxIncoming  (std::numeric_limits<uint32_t>::max())
    , nMaxConnections(std::numeric_limits<uint32_t>::max())
    , nMaxThreads   (1)
    , nTimeout      (30)
    , fMeter        (false)
    , fDDOS         (false)
    , nDDOSCScore   (0)
    , nDDOSRScore   (0)
    , nDDOSTimespan (60)
    , fManager      (0)
    , nManagerInterval(1000)
    {

    }

}
