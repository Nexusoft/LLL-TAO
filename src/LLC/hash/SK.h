/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_HASH_SK_H
#define NEXUS_LLC_HASH_SK_H

#include "../types/uint1024.h"
#include "SK/skein.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "SK/KeccakHash.h"

#if defined(__cplusplus)
}
#endif

/** Namespace LLC (Lower Level Crypto) **/
namespace LLC
{

	namespace HASH
	{
		static unsigned char pblank[1];


		/* Hashing Template to Serialize Object into a 512-bit hash **/
		template<typename T> uint512 SerializeHash(const T& obj);


        /* Hashing template for Checksums */
		inline unsigned int SK32(const std::vector<unsigned char>& vch)
		{
			unsigned int skein;
			Skein_256_Ctxt_t ctx;
			Skein_256_Init(&ctx, 32);
			Skein_256_Update(&ctx, (unsigned char *)&vch[0], vch.size());
			Skein_256_Final(&ctx, (unsigned char *)&skein);

			unsigned int keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 1344, 256, 32, 0x06);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 32);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}

		/* Hashing template for Checksums */
		template<typename T1>
		inline unsigned int SK32(const T1 pbegin, const T1 pend)
		{
			unsigned int skein;
			Skein_256_Ctxt_t ctx;
			Skein_256_Init  (&ctx, 32);
			Skein_256_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_256_Final (&ctx, (unsigned char *)&skein);

			unsigned int keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 1344, 256, 32, 0x06);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 32);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for Checksums */
		template<typename T1>
		inline uint64_t SK64(const T1 pbegin, const T1 pend)
		{
			uint64_t skein;
			Skein_256_Ctxt_t ctx;
			Skein_256_Init  (&ctx, 64);
			Skein_256_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_256_Final (&ctx, (unsigned char *)&skein);

			uint64_t keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 1344, 256, 64, 0x06);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 64);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for Address Generation */
		inline uint64_t SK64(const std::vector<unsigned char>& vch)
		{
			uint64_t skein;
			Skein_256_Ctxt_t ctx;
			Skein_256_Init(&ctx, 64);
			Skein_256_Update(&ctx, (unsigned char *)&vch[0], vch.size());
			Skein_256_Final(&ctx, (unsigned char *)&skein);

			uint64_t keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 1344, 256, 64, 0x06);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 64);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for Address Generation */
		inline uint256 SK256(const std::vector<unsigned char>& vch)
		{
			uint256 skein;
			Skein_256_Ctxt_t ctx;
			Skein_256_Init(&ctx, 256);
			Skein_256_Update(&ctx, (unsigned char *)&vch[0], vch.size());
			Skein_256_Final(&ctx, (unsigned char *)&skein);

			uint256 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_256(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 256);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for Address Generation */
		template<typename T1>
		inline uint256 SK256(const T1 pbegin, const T1 pend)
		{
			uint256 skein;
			Skein_256_Ctxt_t ctx;
			Skein_256_Init  (&ctx, 256);
			Skein_256_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_256_Final (&ctx, (unsigned char *)&skein);

			uint256 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_256(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 256);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


        /* Hashing template for Next Hash */
        inline uint512 SK512(const std::vector<unsigned char>& vch)
		{
			uint512 skein;
			Skein_512_Ctxt_t ctx;
			Skein_512_Init(&ctx, 512);
			Skein_512_Update(&ctx, (unsigned char *)&vch[0], vch.size());
			Skein_512_Final(&ctx, (unsigned char *)&skein);

			uint512 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_512(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 512);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for Trust Key Hash */
		template<typename T1>
		inline uint512 SK512(const std::vector<unsigned char>& vch, const T1 pbegin, const T1 pend)
		{
			uint512 skein;
			Skein_512_Ctxt_t ctx;
			Skein_512_Init(&ctx, 512);
			Skein_512_Update(&ctx, (unsigned char *)&vch[0], vch.size());
			Skein_512_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_512_Final(&ctx, (unsigned char *)&skein);

			uint512 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_512(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 512);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for TX hash */
		template<typename T1>
		inline uint512 SK512(const T1 pbegin, const T1 pend)
		{
			uint512 skein;
			Skein_512_Ctxt_t ctx;
			Skein_512_Init  (&ctx, 512);
			Skein_512_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein_512_Final (&ctx, (unsigned char *)&skein);

			uint512 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_512(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 512);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for TX hash */
		template<typename T1, typename T2>
		inline uint512 SK512(const T1 p1begin, const T1 p1end,
							const T2 p2begin, const T2 p2end)
		{
			uint512 skein;
			Skein_512_Ctxt_t ctx;
			Skein_512_Init  (&ctx, 512);
			Skein_512_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
			Skein_512_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
			Skein_512_Final (&ctx, (unsigned char *)&skein);

			uint512 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_512(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 512);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template for TX hash */
		template<typename T1, typename T2, typename T3>
		inline uint512 SK512(const T1 p1begin, const T1 p1end,
							const T2 p2begin, const T2 p2end,
							const T3 p3begin, const T3 p3end)
		{
			uint512 skein;
			Skein_512_Ctxt_t ctx;
			Skein_512_Init  (&ctx, 512);
			Skein_512_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
			Skein_512_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
			Skein_512_Update(&ctx, (p3begin == p3end ? pblank : (unsigned char*)&p3begin[0]), (p3end - p3begin) * sizeof(p3begin[0]));
			Skein_512_Final (&ctx, (unsigned char *)&skein);

			uint512 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize_SHA3_512(&ctx_keccak);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 512);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template used for Private Keys */
		template<typename T1>
		inline uint576 SK576(const T1 pbegin, const T1 pend)
		{
			uint576 skein;
			Skein1024_Ctxt_t ctx;
			Skein1024_Init(&ctx, 576);
			Skein1024_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein1024_Final(&ctx, (unsigned char *)&skein);

			uint576 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 1024, 576, 576, 0x06);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 576);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}

        /* Hashing template used to Build Sig Chains */
        inline uint1024 SK1024(const std::vector<unsigned char>& vch)
		{
			uint1024 skein;
			Skein1024_Ctxt_t ctx;
			Skein1024_Init(&ctx, 1024);
			Skein1024_Update(&ctx, (unsigned char *)&vch[0], vch.size());
			Skein1024_Final(&ctx, (unsigned char *)&skein);

			uint1024 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 576, 1024, 1024, 0x05);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 1024);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}


		/* Hashing template used to build Block Hashes */
		template<typename T1>
		inline uint1024 SK1024(const T1 pbegin, const T1 pend)
		{
			uint1024 skein;
			Skein1024_Ctxt_t ctx;
			Skein1024_Init(&ctx, 1024);
			Skein1024_Update(&ctx, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
			Skein1024_Final(&ctx, (unsigned char *)&skein);

			uint1024 keccak;
			Keccak_HashInstance ctx_keccak;
			Keccak_HashInitialize(&ctx_keccak, 576, 1024, 1024, 0x05);
			Keccak_HashUpdate(&ctx_keccak, (unsigned char *)&skein, 1024);
			Keccak_HashFinal(&ctx_keccak, (unsigned char *)&keccak);

			return keccak;
		}
	}
}

#endif
