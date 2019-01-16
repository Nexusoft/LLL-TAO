/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_INCLUDE_ENUM_H
#define NEXUS_LEGACY_INCLUDE_ENUM_H

/* Global Legacy namespace. */
namespace Legacy
{
    /* Flags for connect inputs. */
    struct FLAGS
    {
        enum
        {
            MEMPOOL = (1 << 1),
            BLOCK   = (1 << 2)
        };
    };
}

#endif
