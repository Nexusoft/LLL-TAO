/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#include "SK.h"

#include "../../LLP/include/version.h"

#include "../../Util/templates/serialize.h"


/** Namespace LLC (Lower Level Crypto) **/
namespace LLC
{

	namespace HASH
	{

		/** Serialize Hash: Used to Serialize a CTransaction class in order to obtain the Tx Hash. Utilizes CDataStream to serialize the class. **/
		template<typename T>
		LLC::uint512 SerializeHash(const T& obj)
		{
			// Most of the time is spent allocating and deallocating CDataStream's
			// buffer.  If this ever needs to be optimized further, make a CStaticStream
			// class with its buffer on the stack.
			CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
			ss.reserve(10000);
			ss << obj;
			return LLC::HASH::SK512(ss.begin(), ss.end());
		}

	}
}
