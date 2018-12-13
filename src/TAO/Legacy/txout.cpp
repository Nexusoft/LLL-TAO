/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLP/include/version.h>

#include <Util/include/string.h>

#include <TAO/Legacy/types/txout.h>
#include <TAO/Legacy/types/script.h>

namespace Legacy
{

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
	uint512_t CTxOut::GetHash() const
	{
		// Most of the time is spent allocating and deallocating CDataStream's
	    // buffer.  If this ever needs to be optimized further, make a CStaticStream
	    // class with its buffer on the stack.
	    CDataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
	    ss.reserve(10000);
	    ss << *this;
	    return LLC::SK512(ss.begin(), ss.end());
	}


	/* Short Hand debug output of the object */
	std::string CTxOut::ToStringShort() const
	{
		return debug::strprintf(" out %s %s", FormatMoney(nValue).c_str(), scriptPubKey.ToString(true).c_str());
	}


	/* Full object debug output */
	std::string CTxOut::ToString() const
	{
		if (IsEmpty()) return "CTxOut(empty)";
		if (scriptPubKey.size() < 6)
			return "CTxOut(error)";
		return debug::strprintf("CTxOut(nValue=%s, scriptPubKey=%s)", FormatMoney(nValue).c_str(), scriptPubKey.ToString().c_str());
	}


	/* Dump the full object to the console (stdout) */
	void CTxOut::print() const
	{
		debug::log(0, "%s", ToString().c_str());
	}
}
