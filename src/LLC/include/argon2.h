/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>
#include <LLC/hash/argon2.h>

/** Namespace LLC (Lower Level Crypto) **/
namespace LLC
{

	/** Argon2_256
	 *
	 * 256-bit hashing function.
	 * 
	 * @param vchData  Data to be hashed
	 * @param vchSalt  Optional salt to use
	 * @param vchSecret  Optional secret to use
	 * @param nCost  The computational cost to use
	 * @param nMemory  The memory cost to use
	 * 
	 * @return  The hashed data
	 *
	 **/
	uint256_t Argon2_256(const std::vector<uint8_t>& vchData, 
						const std::vector<uint8_t>& vchSalt = std::vector<uint8_t>(16), 
						const std::vector<uint8_t>& vchSecret = std::vector<uint8_t>(),
						uint32_t nCost = 64, uint32_t nMemory = (1 << 16));



	/** Argon2_512
	 *
	 * 512-bit hashing function.
	 *
	 * @param vchData  Data to be hashed
	 * @param vchSalt  Optional salt to use
	 * @param vchSecret  Optional secret to use
	 * @param nCost  The computational cost to use
	 * @param nMemory  The memory cost to use
	 * 
	 * @return  The hashed data
	 **/
	uint512_t Argon2_512(const std::vector<uint8_t>& vchData, 
						const std::vector<uint8_t>& vchSalt = std::vector<uint8_t>(16), 
						const std::vector<uint8_t>& vchSecret = std::vector<uint8_t>(),
						uint32_t nCost = 64, uint32_t nMemory = (1 << 16));

	
}
