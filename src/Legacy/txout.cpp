/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>


#include <Util/templates/datastream.h>
#include <LLP/include/version.h>

#include <Util/include/string.h>

#include <Legacy/types/txout.h>
#include <Legacy/types/script.h>

namespace Legacy
{

	/* Set the object to null state. */
	void TxOut::SetNull()
	{
		nValue = -1;
		scriptPubKey.clear();
	}


	/* Determine if the object is in a null state. */
	bool TxOut::IsNull()
	{
		return (nValue == -1);
	}


	/* Clear the object and reset value to 0. */
	void TxOut::SetEmpty()
	{
		nValue = 0;
		scriptPubKey.clear();
	}


	/* Determine if the object is in an empty state. */
	bool TxOut::IsEmpty() const
	{
		return (nValue == 0 && scriptPubKey.empty());
	}


	/* Get the hash of the object. */
	uint512_t TxOut::GetHash() const
	{
		// Most of the time is spent allocating and deallocating DataStream's
	    // buffer.  If this ever needs to be optimized further, make a CStaticStream
	    // class with its buffer on the stack.
	    DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
	    ss.reserve(8192);
	    ss << *this;
	    return LLC::SK512(ss.begin(), ss.end());
	}


	/* Short Hand debug output of the object */
	std::string TxOut::ToStringShort() const
	{
		return debug::safe_printstr(" out ", FormatMoney(nValue), " ", scriptPubKey.ToString(true));
	}


	/* Full object debug output */
	std::string TxOut::ToString() const
	{
		if (IsEmpty()) return "TxOut(empty)";
		if (scriptPubKey.size() < 6)
			return "TxOut(error)";
		return debug::safe_printstr("TxOut(nValue=", FormatMoney(nValue), ", scriptPubKey=", scriptPubKey.ToString(), ")");
	}


	/* Dump the full object to the console (stdout) */
	void TxOut::print() const
	{
		debug::log(0, ToString());
	}
}
