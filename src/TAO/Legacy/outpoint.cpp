/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Legacy/types/outpoint.h>

#include <LLC/types/uint1024.h>
#include <Util/templates/serialize.h>
#include <Util/include/debug.h>

namespace Legacy
{

	/* Full object debug output */
	std::string COutPoint::ToString() const
	{
		return debug::strprintf("COutPoint(%s, %d)", hash.ToString().substr(0,10).c_str(), n);
	}


	/* Dump the full object to the console (stdout) */
	void COutPoint::print() const
	{
		debug::log(0, "%s", ToString().c_str());
	}
}
