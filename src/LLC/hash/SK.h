/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_HASH_SK_H
#define NEXUS_LLC_HASH_SK_H

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK/skein.h>
#include <LLC/hash/SK/KeccakHash.h>

#include <LLD/cache/template_lru.h>

/** Namespace LLC (Lower Level Crypto) **/
namespace LLC
{

	static uint8_t pblank[1];

	/* LRU cache of hashed values  */
	extern LLD::TemplateLRU<std::vector<uint8_t>, uint64_t> cache64;
	extern LLD::TemplateLRU<std::vector<uint8_t>, uint256_t> cache256;
	extern LLD::TemplateLRU<std::vector<uint8_t>, uint512_t> cache512;
	extern LLD::TemplateLRU<std::vector<uint8_t>, uint1024_t> cache1024;


    /** SK32
     *
     * 32-bit hashing template for Checksums.
     *
     **/
	inline uint32_t SK32(const std::vector<uint8_t> vch)
	{
		uint32_t hashSkein;
		Skein_256_Ctxt_t ctxSkein;
		Skein_256_Init(&ctxSkein, 32);
		Skein_256_Update(&ctxSkein, (uint8_t *)&vch[0], vch.size());
		Skein_256_Final(&ctxSkein, (uint8_t *)&hashSkein);

		uint32_t hashKeccak;
		Keccak_HashInstance ctxKeccak;
		Keccak_HashInitialize(&ctxKeccak, 1344, 256, 32, 0x06);
		Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 32);
		Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

		return hashKeccak;
	}


	/** SK32
     *
     *  32-bit hashing template for Checksums.
     *
     **/
	template<typename T1>
	inline uint32_t SK32(const T1 pbegin, const T1 pend)
	{
		uint32_t hashSkein;
		Skein_256_Ctxt_t ctxSkein;
		Skein_256_Init  (&ctxSkein, 32);
		Skein_256_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
		Skein_256_Final (&ctxSkein, (uint8_t *)&hashSkein);

		uint32_t hashKeccak;
		Keccak_HashInstance ctxKeccak;
		Keccak_HashInitialize(&ctxKeccak, 1344, 256, 32, 0x06);
		Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 32);
		Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

		return hashKeccak;
	}


	/** SK64
     *
     *  64-bit hashing template for Checksums.
     *
     **/
	template<typename T1>
	inline uint64_t SK64(const T1 pbegin, const T1 pend)
	{
		/* Copy the data into a vector of bytes so we can cache the hashed value */
		std::vector<uint8_t> vData(pbegin, pend);

		/* Check the cache for this data */
		uint64_t hashKeccak = 0;
		if(!cache64.Get(vData, hashKeccak))
		{
			uint64_t hashSkein = 0;
			Skein_256_Ctxt_t ctxSkein;
			Skein_256_Init  (&ctxSkein, 64);
			Skein_256_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_256_Final (&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize(&ctxKeccak, 1344, 256, 64, 0x06);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 64);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache64.Put(vData, hashKeccak);
		}

		return hashKeccak;
	}


	/** SK64
     *
     *  64-bit hashing template for Address Generation.
     *
     **/
	inline uint64_t SK64(const std::vector<uint8_t>& vch)
	{
		/* Check the cache for this data */
		uint64_t hashKeccak = 0;
		if(!cache64.Get(vch, hashKeccak))
		{
			uint64_t hashSkein = 0;
			Skein_256_Ctxt_t ctxSkein;
			Skein_256_Init(&ctxSkein, 64);
			Skein_256_Update(&ctxSkein, (uint8_t *)&vch[0], vch.size());
			Skein_256_Final(&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize(&ctxKeccak, 1344, 256, 64, 0x06);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 64);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache64.Put(vch, hashKeccak);
		}

		return hashKeccak;
	}


	/** SK256
     *
     *  256-bit hashing template for Address Generation .
     *
     **/
	inline uint256_t SK256(const std::vector<uint8_t>& vch)
	{
		/* Check the cache for this data */
		uint256_t hashKeccak;
		if(!cache256.Get(vch, hashKeccak))
		{
			uint256_t hashSkein = 0;
			Skein_256_Ctxt_t ctxSkein;
			Skein_256_Init(&ctxSkein, 256);
			Skein_256_Update(&ctxSkein, (uint8_t *)&vch[0], vch.size());
			Skein_256_Final(&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize_SHA3_256(&ctxKeccak);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 256);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache256.Put(vch, hashKeccak);
		}

		return hashKeccak;
	}


	/** SK256
     *
     *  256-bit hashing template for Address Generation.
     *
     **/
	template<typename T1>
	inline uint256_t SK256(const T1 pbegin, const T1 pend)
	{
		/* Copy the data into a vector of bytes so we can cache the hashed value */
		std::vector<uint8_t> vData(pbegin, pend);

		/* Check the cache for this data */
		uint256_t hashKeccak;
		if(!cache256.Get(vData, hashKeccak))
		{
			uint256_t hashSkein;
			Skein_256_Ctxt_t ctxSkein;
			Skein_256_Init  (&ctxSkein, 256);
			Skein_256_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_256_Final (&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize_SHA3_256(&ctxKeccak);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 256);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache256.Put(vData, hashKeccak);
		}

		return hashKeccak;
	}


	/** SK256
	 *
	 *  256-bit hashing template for strings.
	 *
	 *  @param[in] str The string to hash
	 *
	 *  @return uint256_t hashSkein/hashKeccak hash of the string
	 *
	 **/

	inline uint256_t SK256(const std::string& str)
	{
		return SK256(str.begin(), str.end());
	}


    /** SK512
     *
     *  512-bit hashing template for Next Hash.
     *
     **/
    inline uint512_t SK512(const std::vector<uint8_t>& vch)
	{
		/* Check the cache for this data */
		uint512_t hashKeccak;
		if(!cache512.Get(vch, hashKeccak))
		{
			uint512_t hashSkein;
			Skein_512_Ctxt_t ctxSkein;
			Skein_512_Init(&ctxSkein, 512);
			Skein_512_Update(&ctxSkein, (uint8_t *)&vch[0], vch.size());
			Skein_512_Final(&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize_SHA3_512(&ctxKeccak);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 512);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache512.Put(vch, hashKeccak);
		}

		return hashKeccak;
	}


	/** SK512
     *
     *  512-bit hashing template for Trust Key Hash.
     *
     **/
	template<typename T1>
	inline uint512_t SK512(const std::vector<uint8_t>& vch, const T1 pbegin, const T1 pend)
	{
		uint512_t hashSkein;
		Skein_512_Ctxt_t ctxSkein;
		Skein_512_Init(&ctxSkein, 512);
		Skein_512_Update(&ctxSkein, (uint8_t *)&vch[0], vch.size());
		Skein_512_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
		Skein_512_Final(&ctxSkein, (uint8_t *)&hashSkein);

		uint512_t hashKeccak;
		Keccak_HashInstance ctxKeccak;
		Keccak_HashInitialize_SHA3_512(&ctxKeccak);
		Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 512);
		Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

		return hashKeccak;
	}


	/** SK512
     *
     *  512-bit hashing template for TX hash.
     *
     **/
	template<typename T1>
	inline uint512_t SK512(const T1 pbegin, const T1 pend)
	{
		/* Copy the data into a vector of bytes so we can cache the hashed value */
		std::vector<uint8_t> vData(pbegin, pend);

		/* Check the cache for this data */
		uint512_t hashKeccak;
		if(!cache512.Get(vData, hashKeccak))
		{
			uint512_t hashSkein;
			Skein_512_Ctxt_t ctxSkein;
			Skein_512_Init  (&ctxSkein, 512);
			Skein_512_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_512_Final (&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize_SHA3_512(&ctxKeccak);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 512);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache512.Put(vData, hashKeccak);
		}

		return hashKeccak;
	}


	/** SK512
     *
     * 512-bit hashing template for TX hash.
     *
     **/
	template<typename T1, typename T2>
	inline uint512_t SK512(const T1 p1begin, const T1 p1end,
						   const T2 p2begin, const T2 p2end)
	{
		uint512_t hashSkein;
		Skein_512_Ctxt_t ctxSkein;
		Skein_512_Init  (&ctxSkein, 512);
		Skein_512_Update(&ctxSkein, (p1begin == p1end ? pblank : (uint8_t*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
		Skein_512_Update(&ctxSkein, (p2begin == p2end ? pblank : (uint8_t*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
		Skein_512_Final (&ctxSkein, (uint8_t *)&hashSkein);

		uint512_t hashKeccak;
		Keccak_HashInstance ctxKeccak;
		Keccak_HashInitialize_SHA3_512(&ctxKeccak);
		Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 512);
		Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

		return hashKeccak;
	}


	/** SK512
     *
     *  512-bit hashing template for TX hash.
     *
     **/
	template<typename T1, typename T2, typename T3>
	inline uint512_t SK512(const T1 p1begin, const T1 p1end,
						   const T2 p2begin, const T2 p2end,
						   const T3 p3begin, const T3 p3end)
	{
		uint512_t hashSkein;
		Skein_512_Ctxt_t ctxSkein;
		Skein_512_Init  (&ctxSkein, 512);
		Skein_512_Update(&ctxSkein, (p1begin == p1end ? pblank : (uint8_t*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
		Skein_512_Update(&ctxSkein, (p2begin == p2end ? pblank : (uint8_t*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
		Skein_512_Update(&ctxSkein, (p3begin == p3end ? pblank : (uint8_t*)&p3begin[0]), (p3end - p3begin) * sizeof(p3begin[0]));
		Skein_512_Final (&ctxSkein, (uint8_t *)&hashSkein);

		uint512_t hashKeccak;
		Keccak_HashInstance ctxKeccak;
		Keccak_HashInitialize_SHA3_512(&ctxKeccak);
		Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 512);
		Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

		return hashKeccak;
	}


	/** SK576
     *
     * 576-bit hashing template used for Private Keys.
     *
     **/
	template<typename T1>
	inline uint576_t SK576(const T1 pbegin, const T1 pend)
	{
		uint576_t hashSkein;
		Skein1024_Ctxt_t ctxSkein;
		Skein1024_Init(&ctxSkein, 576);
		Skein1024_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
		Skein1024_Final(&ctxSkein, (uint8_t *)&hashSkein);

		uint576_t hashKeccak;
		Keccak_HashInstance ctxKeccak;
		Keccak_HashInitialize(&ctxKeccak, 1024, 576, 576, 0x06);
		Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 576);
		Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

		return hashKeccak;
	}


	/** SK1024
     *
     *  1024-bit hashing template used to build Block Hashes.
     *
     **/
	template<typename T1>
	inline uint1024_t SK1024(const T1 pbegin, const T1 pend)
	{
		/* Copy the data into a vector of bytes so we can cache the hashed value */
		std::vector<uint8_t> vData(pbegin, pend);

		/* Check the cache for this data */
		uint1024_t hashKeccak;
		if(!cache1024.Get(vData, hashKeccak))
		{
			uint1024_t hashSkein;
			Skein1024_Ctxt_t ctxSkein;
			Skein1024_Init(&ctxSkein, 1024);
			Skein1024_Update(&ctxSkein, (pbegin == pend ? pblank : (uint8_t*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein1024_Final(&ctxSkein, (uint8_t *)&hashSkein);

			Keccak_HashInstance ctxKeccak;
			Keccak_HashInitialize(&ctxKeccak, 576, 1024, 1024, 0x05);
			Keccak_HashUpdate(&ctxKeccak, (uint8_t *)&hashSkein, 1024);
			Keccak_HashFinal(&ctxKeccak, (uint8_t *)&hashKeccak);

			/* Cache the hashed value */
			cache1024.Put(vData, hashKeccak);
		}

		return hashKeccak;
	}
}

#endif
