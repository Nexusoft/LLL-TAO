/**
 * @file goldilocks.hxx
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * Master header for Goldilocks library, C++ version.
 */

#ifndef __GOLDILOCKS_HXX__
#define __GOLDILOCKS_HXX__ 1

#include <goldilocks/ed448.hxx>

/** Namespace for all C++ goldilocks objects. */
namespace goldilocks {
    /** Given a template with a "run" function, run it for all curves */
    template <template<typename Group> class Run>
    void run_for_all_curves() {
        Run<Ed448Goldilocks>::run();
    }
}

#endif /* __GOLDILOCKS_HXX__ */
