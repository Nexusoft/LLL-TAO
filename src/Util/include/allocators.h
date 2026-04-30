/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_ALLOCATORS_H
#define NEXUS_UTIL_INCLUDE_ALLOCATORS_H

#include <cstring>
#include <string>

#ifdef WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600    //targeting minimum Windows Vista version for winsock2, etc.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1  //prevents windows.h from including winsock.h and messing up winsock2.h definitions we use
#endif

#ifndef NOMINMAX
#define NOMINMAX //prevents windows.h from including min/max and potentially interfering with std::min/std::max
#endif

#include <winsock2.h> //ensure winsock2 included before windows even if not needed in this file 
#include <windows.h>

/** This is used to attempt to keep keying material out of swap
 *  Note that VirtualLock does not provide this as a guarantee on Windows,
 *  but, in practice, memory that has been VirtualLock'd almost never gets written to
 *  the pagefile except in rare circumstances where memory is extremely low.
 **/
#define mlock(p, n) VirtualLock((p), (n));
#define munlock(p, n) VirtualUnlock((p), (n));
#else
#include <sys/mman.h>
#include <climits>
/* This comes from limits.h if it's not defined there set a sane default */
#ifndef PAGESIZE
#include <unistd.h>
#define PAGESIZE sysconf(_SC_PAGESIZE)
#endif
#define mlock(a,b) \
mlock(((void *)(((size_t)(a)) & (~((PAGESIZE)-1)))),\
(((((size_t)(a)) + (b) - 1) | ((PAGESIZE) - 1)) + 1) - (((size_t)(a)) & (~((PAGESIZE) - 1))))
#define munlock(a,b) \
munlock(((void *)(((size_t)(a)) & (~((PAGESIZE)-1)))),\
(((((size_t)(a)) + (b) - 1) | ((PAGESIZE) - 1)) + 1) - (((size_t)(a)) & (~((PAGESIZE) - 1))))
#endif


/** secure_allocator
 *
 * Allocator that locks its contents from being paged
 * out of memory and clears its contents before deletion.
 *
 **/
template<typename T>
struct secure_allocator
{
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    /* Stateless allocators propagate trivially on container ops. */
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;
    using is_always_equal                        = std::true_type;

    secure_allocator() noexcept = default;
    secure_allocator(const secure_allocator&) noexcept = default;
    template<typename U>
    secure_allocator(const secure_allocator<U>&) noexcept {}
    ~secure_allocator() noexcept = default;


    /** allocate
     *
     *  allocates n elements of type T
     *
     **/
    T* allocate(std::size_t n)
    {
        T* p = std::allocator<T>{}.allocate(n);
        if(p != nullptr)
            mlock(p, sizeof(T) * n);
        return p;
    }


    /** deallocate
     *
     *  frees n elements of type T from pointer p. clears contents before deletion
     *
     **/
    void deallocate(T* p, std::size_t n) noexcept
    {
        if(p != nullptr)
        {
            memset(p, 0, sizeof(T) * n);
            munlock(p, sizeof(T) * n);
        }
        std::allocator<T>{}.deallocate(p, n);
    }
};

template<typename T, typename U>
inline bool operator==(const secure_allocator<T>&, const secure_allocator<U>&) noexcept { return true; }
template<typename T, typename U>
inline bool operator!=(const secure_allocator<T>&, const secure_allocator<U>&) noexcept { return false; }


/**
 *
 * Allocator that clears its contents before deletion.
 *
 **/
template<typename T>
struct zero_after_free_allocator
{
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;
    using is_always_equal                        = std::true_type;

    zero_after_free_allocator() noexcept = default;
    zero_after_free_allocator(const zero_after_free_allocator&) noexcept = default;
    template<typename U>
    zero_after_free_allocator(const zero_after_free_allocator<U>&) noexcept {}
    ~zero_after_free_allocator() noexcept = default;


    /** allocate
     *
     *  allocates n elements of type T
     *
     **/
    T* allocate(std::size_t n)
    {
        return std::allocator<T>{}.allocate(n);
    }


    /** deallocate
     *
     *  frees n elements of type T from pointer p. Clears contents before deletion.
     *
     **/
    void deallocate(T* p, std::size_t n) noexcept
    {
        if(p != nullptr)
            memset(p, 0, sizeof(T) * n);
        std::allocator<T>{}.deallocate(p, n);
    }
};

template<typename T, typename U>
inline bool operator==(const zero_after_free_allocator<T>&, const zero_after_free_allocator<U>&) noexcept { return true; }
template<typename T, typename U>
inline bool operator!=(const zero_after_free_allocator<T>&, const zero_after_free_allocator<U>&) noexcept { return false; }

/* This is exactly like std::string, but with a custom allocator. */
typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char> > SecureString;

#endif
