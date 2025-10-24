/**
 * @file goldilocks/spongerng.hxx
 * @copyright
 *   Based on CC0 code by David Leon Gil, 2015 \n
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief Sponge RNG instances, C++ wrapper.
 * @warning The guts of this are subject to change.  Please don't implement
 * anything that depends on the deterministic RNG being stable across versions
 * of this library.
 */

#ifndef __GOLDILOCKS_SPONGERNG_HXX__
#define __GOLDILOCKS_SPONGERNG_HXX__

#include <goldilocks/spongerng.h>

#include <string>
#include <sys/types.h>
#include <errno.h>

/** @cond internal */
#if __cplusplus >= 201103L
#define GOLDILOCKS_NOEXCEPT noexcept
#define GOLDILOCKS_DELETE = delete
#else
#define GOLDILOCKS_NOEXCEPT throw()
#define GOLDILOCKS_DELETE
#endif
/** @endcond */

namespace goldilocks {

/** Sponge-based random-number generator */
class SpongeRng : public Rng {
private:
    /** C wrapped object */
    goldilocks_keccak_prng_p sp;

public:
    /** Deterministic flag.
     * The idea is that DETERMINISTIC is used for testing or for lockstep computations,
     * and NONDETERMINISTIC is used in production.
     */
    enum Deterministic { RANDOM = 0, DETERMINISTIC = 1 };

    /** Exception thrown when The RNG fails (to seed itself) */
    class RngException : public std::exception {
    private:
        /** @cond internal */
        const char *const what_;
        /** @endcond */
    public:
        const int err_code; /**< errno that caused the reseed to fail. */
        const char *what() const GOLDILOCKS_NOEXCEPT { return what_; } /**< Description of exception. */
        RngException(int err_code, const char *what_) GOLDILOCKS_NOEXCEPT : what_(what_), err_code(err_code) {} /**< Construct */
    };

    /** Initialize, deterministically by default, from block */
    inline SpongeRng( const Block &in, Deterministic det ) {
        goldilocks_spongerng_init_from_buffer(sp,in.data(),in.size(),(int)det);
    }

    /** Initialize, non-deterministically by default, from C/C++ filename */
    inline SpongeRng( const std::string &in = "/dev/urandom", size_t len = 32, Deterministic det = RANDOM )
        /*throw(RngException)*/ {
        goldilocks_error_t ret = goldilocks_spongerng_init_from_file(sp,in.c_str(),len,det);
        if (!goldilocks_successful(ret)) {
            throw RngException(errno, "Couldn't load from file");
        }
    }

    /** Stir in new data */
    inline void stir( const Block &data ) GOLDILOCKS_NOEXCEPT {
        goldilocks_spongerng_stir(sp,data.data(),data.size());
    }

    /** Securely destroy by overwriting state. */
    inline ~SpongeRng() GOLDILOCKS_NOEXCEPT { goldilocks_spongerng_destroy(sp); }

    using Rng::read;

    /** Read data to a buffer. */
    virtual inline void read(Buffer buffer) GOLDILOCKS_NOEXCEPT
#if __cplusplus >= 201103L
        final
#endif
        { goldilocks_spongerng_next(sp,buffer.data(),buffer.size()); }

private:
    SpongeRng(const SpongeRng &) GOLDILOCKS_DELETE;
    SpongeRng &operator=(const SpongeRng &) GOLDILOCKS_DELETE;
};
/**@endcond*/

} /* namespace goldilocks */

#undef GOLDILOCKS_NOEXCEPT
#undef GOLDILOCKS_DELETE

#endif /* __GOLDILOCKS_SPONGERNG_HXX__ */
