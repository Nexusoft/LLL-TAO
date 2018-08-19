/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "SK.h"

#include "../../LLP/include/version.h"

#include "../../Util/templates/serialize.h"


/** Namespace LLC (Lower Level Crypto) **/
namespace LLC
{

	/** Serialize Hash: Used to Serialize a CTransaction class in order to obtain the Tx Hash. Utilizes CDataStream to serialize the class. **/
	template<typename T>
	uint512_t SerializeHash(const T& obj)
	{
		// Most of the time is spent allocating and deallocating CDataStream's
		// buffer.  If this ever needs to be optimized further, make a CStaticStream
		// class with its buffer on the stack.
		CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
		ss.reserve(10000);
		ss << obj;
		return SK512(ss.begin(), ss.end());
	}

}
