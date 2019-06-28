/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/outpoint.h>

#include <Util/include/debug.h>

namespace Legacy
{

	/* Full object debug output */
	std::string OutPoint::ToString() const
	{
		return debug::safe_printstr("OutPoint(", hash.SubString(), ", ", n, ")");
	}


	/* Dump the full object to the console (stdout) */
	void OutPoint::print() const
	{
		debug::log(0, ToString());
	}
}
