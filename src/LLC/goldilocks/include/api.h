/**
 * @file api.h
 * @brief Generic api helper header.
 * @copyright
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 */

#ifndef __API_H__
#define __API_H__

#define API_NAME "goldilocks_448"
#define API_NS(_id) goldilocks_448_##_id

#define COFACTOR 4

#define SCALAR_BITS GOLDILOCKS_448_SCALAR_BITS
#define SCALAR_SER_BYTES GOLDILOCKS_448_SCALAR_BYTES
#define SCALAR_LIMBS GOLDILOCKS_448_SCALAR_LIMBS

#define scalar_p API_NS(scalar_p)

#define WBITS GOLDILOCKS_WORD_BITS /* NB this may be different from ARCH_WORD_BITS */


#endif // __API_H__
