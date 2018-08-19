/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#include "include/txout.h"
#include "../"

namespace Legacy
{

	namespace Types
	{

		//the serialization methods
		SERIALIZE_SOURCE
		(
			CTxOut,

			READWRITE(nValue);
			READWRITE(scriptPubKey);
		)


		/* Set the object to null state. */
		void CTxOut::SetNull()
		{
			nValue = -1;
			scriptPubKey.clear();
		}


		/* Determine if the object is in a null state. */
		bool CTxOut::IsNull()
		{
			return (nValue == -1);
		}


		/* Clear the object and reset value to 0. */
		void CTxOut::SetEmpty()
		{
			nValue = 0;
			scriptPubKey.clear();
		}


		/* Determine if the object is in an empty state. */
		bool CTxOut::IsEmpty() const
		{
			return (nValue == 0 && scriptPubKey.empty());
		}


		/* Get the hash of the object. */
		LLC::uint512 CTxOut::GetHash() const
		{
			return SerializeHash(*this);
		}


		/* Short Hand debug output of the object */
		std::string CTxOut::ToStringShort() const
		{
			return strprintf(" out %s %s", FormatMoney(nValue).c_str(), scriptPubKey.ToString(true).c_str());
		}


		/* Full object debug output */
		std::string CTxOut::ToString() const
		{
			if (IsEmpty()) return "CTxOut(empty)";
			if (scriptPubKey.size() < 6)
				return "CTxOut(error)";
			return strprintf("CTxOut(nValue=%s, scriptPubKey=%s)", FormatMoney(nValue).c_str(), scriptPubKey.ToString().c_str());
		}


		/* Dump the full object to the console (stdout) */
		void CTxOut::print() const
		{
			printf("%s\n", ToString().c_str());
		}
	}
}
