/**
 * @file goldilocks/eddsa.hxx
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * EdDSA crypto routines, metaheader.
 */

#ifndef __GOLDILOCKS_EDDSA_HXX__
#define __GOLDILOCKS_EDDSA_HXX__ 1

/** Namespace for all libgoldilocks C++ objects. */
namespace goldilocks {
    /** How signatures handle hashing. */
    enum Prehashed {
        PURE,     /**< Sign the message itself.  This can't be done in one pass. */
        PREHASHED /**< Sign the hash of the message. */
    };
}

#include <goldilocks/ed448.hxx>

#endif /* __GOLDILOCKS_EDDSA_HXX__ */
