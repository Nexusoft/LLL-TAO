/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_TEMPLATES_SERIALIZE_H
#define NEXUS_UTIL_TEMPLATES_SERIALIZE_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <ios>
#include <cassert>
#include <limits>
#include <cstring>
#include <cstdio>

#include <type_traits>
#include <tuple>

#include <LLC/types/uint1024.h>
#include <Util/include/allocators.h>
#include <Util/include/debug.h>


/* forward declaration */
namespace Legacy
{
    class Script;
}


/** REF
 *
 *  Used to bypass the rule against non-const reference to temporary
 *  where it makes sense with wrappers such as FlatData or CTxDB
 *
 */
template<typename T>
inline T& REF(const T& val)
{
    return const_cast<T&>(val);
}

/**
 *
 *  Templates for serializing to anything that looks like a stream,
 * i.e. anything that supports .read(char*, uint32_t) and .write(char*, uint32_t)
 *
 **/
enum
{
    // primary actions
    SER_NETWORK         = (1 << 0),
    SER_DISK            = (1 << 1),
    SER_GETHASH         = (1 << 2),

    // LLD actions
    SER_LLD             = (1 << 3),
    SER_LLD_KEY_HEADER  = (1 << 4),

    // layers
    SER_REGISTER        = (1 << 5),
    SER_OPERATIONS      = (1 << 6),


    // mining
    SER_MINER           = (1 << 10),

    // modifiers
    SER_SKIPSIG         = (1 << 16),
    SER_BLOCKHEADERONLY = (1 << 17),
    SER_STATEHEADERONLY = (1 << 18),

    // LLP actions
    SER_LLP_HEADER_ONLY = (1 << 20),

    //Register actions
    SER_REGISTER_PRUNED = (1 << 21),

    // sigchain options
    SER_GENESISHASH     = (1 << 22)
};

/* Used to suppress unused variable warnings. */
#define _unused(x) ((void)(x))

/* This should be used in header only files with complete types
 * best to avoid the use of it if not needed, kept for back-compatability */
#define IMPLEMENT_SERIALIZE(statements)                                        \
    uint64_t GetSerializeSize(uint32_t nSerType, uint32_t nSerVersion) const   \
    {                                                                          \
        CSerActionGetSerializeSize ser_action;                                 \
        const bool fGetSize = true;                                            \
        const bool fWrite = false;                                             \
        const bool fRead = false;                                              \
        uint64_t nSerSize = 0;                                                 \
        ser_streamplaceholder s;                                               \
        _unused(fGetSize); /* suppress warning */                              \
        _unused(fWrite);   /* suppress warning */                              \
        _unused(fRead);    /* suppress warning */                              \
        s.nSerType = nSerType;                                                 \
        s.nSerVersion = nSerVersion;                                           \
        {statements}                                                           \
        return nSerSize;                                                       \
    }                                                                          \
    template<typename Stream>                                                  \
    void Serialize(Stream& s, uint32_t nSerType, uint32_t nSerVersion) const   \
    {                                                                          \
        CSerActionSerialize ser_action;                                        \
        const bool fGetSize = false;                                           \
        const bool fWrite = true;                                              \
        const bool fRead = false;                                              \
        uint64_t nSerSize = 0;                                                 \
        _unused(fGetSize); /* suppress warning */                              \
        _unused(fWrite);   /* suppress warning */                              \
        _unused(fRead);    /* suppress warning */                              \
        {statements}                                                           \
    }                                                                          \
    template<typename Stream>                                                  \
    void Unserialize(Stream& s, uint32_t nSerType, uint32_t nSerVersion)       \
    {                                                                          \
        CSerActionUnserialize ser_action;                                      \
        const bool fGetSize = false;                                           \
        const bool fWrite = false;                                             \
        const bool fRead = true;                                               \
        uint64_t nSerSize = 0;                                                 \
        _unused(fGetSize); /* suppress warning */                              \
        _unused(fWrite);   /* suppress warning */                              \
        _unused(fRead);    /* suppress warning */                              \
        {statements}                                                           \
    }


#define READWRITE(obj) (nSerSize += ::SerReadWrite(s, (obj), nSerType, nSerVersion, ser_action))

/**
 *
 * Basic types
 *
 **/
#define WRITEDATA(s, obj)   s.write((char*)&(obj), sizeof(obj))
#define READDATA(s, obj)    s.read((char*)&(obj), sizeof(obj))

inline uint64_t GetSerializeSize( int8_t a,                                     uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(uint8_t a,                                     uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize( int16_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(uint16_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize( int32_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(uint32_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(int64_t a,                                     uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(uint64_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(float a,                                       uint32_t, uint32_t=0) { return sizeof(a); }
inline uint64_t GetSerializeSize(double a,                                      uint32_t, uint32_t=0) { return sizeof(a); }

template<typename Stream> inline void Serialize(Stream& s,  int8_t  a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint8_t  a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s,  int16_t a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint16_t a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s,  int32_t a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint32_t a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int64_t a,           uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint64_t a,          uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, float a,             uint32_t, uint32_t=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, double a,            uint32_t, uint32_t=0) { WRITEDATA(s, a); }

template<typename Stream> inline void Unserialize(Stream& s,  int8_t&  a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint8_t&  a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s,  int16_t& a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint16_t& a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s,  int32_t& a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint32_t& a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, int64_t& a,        uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint64_t& a,       uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, float& a,          uint32_t, uint32_t=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, double& a,         uint32_t, uint32_t=0) { READDATA(s, a); }

inline uint64_t GetSerializeSize(bool a, uint32_t, uint32_t=0)                          { return sizeof(char); }
template<typename Stream> inline void Serialize(Stream& s, bool a, uint32_t, uint32_t=0)    { char f=a; WRITEDATA(s, f); }
template<typename Stream> inline void Unserialize(Stream& s, bool& a, uint32_t, uint32_t=0) { char f; READDATA(s, f); a=f; }


static const uint32_t MAX_SIZE = 0x02000000;


/** GetSizeOfCompactSize
 *
 * Compact size
 *  size <  253        -- 1 byte
 *  size <= USHRT_MAX  -- 3 bytes  (253 + 2 bytes)
 *  size <= UINT_MAX   -- 5 bytes  (254 + 4 bytes)
 *  size >  UINT_MAX   -- 9 bytes  (255 + 8 bytes)
 *
 *  param[in] nSize the compact size
 *
 *  return the size of the compact size
 *
 **/
inline uint64_t GetSizeOfCompactSize(uint64_t nSize)
{
    if(nSize < 253)
        return sizeof(uint8_t);
    else if(nSize <= std::numeric_limits<uint16_t>::max())
        return sizeof(uint8_t) + sizeof(uint16_t);
    else if(nSize <= std::numeric_limits<uint32_t>::max())
        return sizeof(uint8_t) + sizeof(uint32_t);
    else
        return sizeof(uint8_t) + sizeof(uint64_t);
}


/** WriteCompactSize
 *
 *  Write the compact size.
 *
 *  @param[in] os The output stream.
 *  @param[in] nSize The compact size to write.
 *
 **/
template<typename Stream>
void WriteCompactSize(Stream& os, uint64_t nSize)
{
    if(nSize < 253)
    {
        uint8_t chSize = static_cast<uint8_t>(nSize);
        WRITEDATA(os, chSize);
    }
    else if(nSize <= std::numeric_limits<uint16_t>::max())
    {
        uint8_t chSize = 253;
        uint16_t xSize = static_cast<uint16_t>(nSize);
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else if(nSize <= std::numeric_limits<uint32_t>::max())
    {
        uint8_t chSize = 254;
        uint32_t xSize = static_cast<uint32_t>(nSize);
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else
    {
        uint8_t chSize = 255;
        uint64_t xSize = nSize;
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    return;
}


/** ReadCompactSize
 *
 *  Read the compact size.
 *
 *  @param[in] is The input Stream.
 *
 *  @return The compact size.
 *
 **/
template<typename Stream>
uint64_t ReadCompactSize(Stream& is)
{
    uint8_t chSize;
    READDATA(is, chSize);
    uint64_t nSizeRet = 0;
    if(chSize < 253)
    {
        nSizeRet = chSize;
    }
    else if(chSize == 253)
    {
        uint16_t xSize;
        READDATA(is, xSize);
        nSizeRet = xSize;
    }
    else if(chSize == 254)
    {
        uint32_t xSize;
        READDATA(is, xSize);
        nSizeRet = xSize;
    }
    else
    {
        uint64_t xSize;
        READDATA(is, xSize);
        nSizeRet = xSize;
    }
    if(nSizeRet > (uint64_t)MAX_SIZE)
        throw std::runtime_error("ReadCompactSize() : size too large");

    return nSizeRet;
}

/**
 *  Function prototypes
 **/

/** string **/
template<typename C> uint64_t GetSerializeSize(const std::basic_string<C>& str, uint32_t, uint32_t=0);
template<typename Stream, typename C> void Serialize(Stream& os, const std::basic_string<C>& str, uint32_t, uint32_t=0);
template<typename Stream, typename C> void Unserialize(Stream& is, std::basic_string<C>& str, uint32_t, uint32_t=0);

/** vector **/
template<typename T, typename A> uint64_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&);
template<typename T, typename A> uint64_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&);
template<typename T, typename A> inline uint64_t GetSerializeSize(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&);
template<typename Stream, typename T, typename A> inline void Serialize(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&);
template<typename Stream, typename T, typename A> inline void Unserialize(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion);

/** others derived from vector **/
extern inline uint64_t GetSerializeSize(const Legacy::Script& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream> void Serialize(Stream& os, const Legacy::Script& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream> void Unserialize(Stream& is, Legacy::Script& v, uint32_t nSerType, uint32_t nSerVersion);

/** pair **/
template<typename K, typename T> uint64_t GetSerializeSize(const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T> void Serialize(Stream& os, const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T> void Unserialize(Stream& is, std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion);

/** 3 tuple **/
template<typename T0, typename T1, typename T2> uint64_t GetSerializeSize(const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2> void Serialize(Stream& os, const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2> void Unserialize(Stream& is, std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion);

/** 4 tuple **/
template<typename T0, typename T1, typename T2, typename T3> uint64_t GetSerializeSize(const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2, typename T3> void Serialize(Stream& os, const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2, typename T3> void Unserialize(Stream& is, std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion);

/** map **/
template<typename K, typename T, typename Pred, typename A> uint64_t GetSerializeSize(const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);

/** set **/
template<typename K, typename Pred, typename A> uint64_t GetSerializeSize(const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);


/**
 *  If none of the specialized versions above matched, default to calling member function.
 *  "uint32_t nSerType" is changed to "long nSerType" to keep from getting an ambiguous overload error.
 *  The compiler will only cast uint32_t to long if none of the other templates matched.
 **/
template<typename T>
inline uint64_t GetSerializeSize(const T& a, long nSerType, uint32_t nSerVersion)
{
    return a.GetSerializeSize((uint32_t)nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline void Serialize(Stream& os, const T& a, long nSerType, uint32_t nSerVersion)
{
    a.Serialize(os, (uint32_t)nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline void Unserialize(Stream& is, T& a, long nSerType, uint32_t nSerVersion)
{
    a.Unserialize(is, (uint32_t)nSerType, nSerVersion);
}


/**
 * string
 **/
template<typename C>
uint64_t GetSerializeSize(const std::basic_string<C>& str, uint32_t, uint32_t)
{
    return GetSizeOfCompactSize(str.size()) + str.size() * sizeof(str[0]);
}

template<typename Stream, typename C>
void Serialize(Stream& os, const std::basic_string<C>& str, uint32_t, uint32_t)
{
    WriteCompactSize(os, str.size());
    if(!str.empty())
        os.write((char*)&str[0], str.size() * sizeof(str[0]));
}

template<typename Stream, typename C>
void Unserialize(Stream& is, std::basic_string<C>& str, uint32_t, uint32_t)
{
    uint64_t nSize = ReadCompactSize(is);
    str.resize(nSize);
    if(nSize != 0)
        is.read((char*)&str[0], nSize * sizeof(str[0]));
}


/**
 * vector
 **/
template<typename T, typename A>
uint64_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&)
{
    return (GetSizeOfCompactSize(v.size()) + v.size() * sizeof(T));
}

template<typename T, typename A>
uint64_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&)
{
    uint64_t nSize = GetSizeOfCompactSize(v.size());
    for(typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        nSize += GetSerializeSize((*vi), nSerType, nSerVersion);
    return nSize;
}

template<typename T, typename A>
inline uint64_t GetSerializeSize(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion)
{
    return GetSerializeSize_impl(v, nSerType, nSerVersion, std::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&)
{
    WriteCompactSize(os, v.size());
    if(!v.empty())
        os.write((char*)&v[0], v.size() * sizeof(T));
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&)
{
    WriteCompactSize(os, v.size());
    for(typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        ::Serialize(os, (*vi), nSerType, nSerVersion);
}

template<typename Stream, typename T, typename A>
inline void Serialize(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Serialize_impl(os, v, nSerType, nSerVersion, std::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&)
{
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    uint64_t nSize = ReadCompactSize(is);
    uint64_t i = 0;
    while(i < nSize)
    {
        uint64_t blk = std::min(nSize - i, (1 + static_cast<uint64_t>(4999999) / sizeof(T)));
        v.resize(i + blk);
        is.read((char*)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&)
{
    v.clear();
    uint64_t nSize = ReadCompactSize(is);
    uint64_t i = 0;
    uint64_t nMid = 0;
    while(nMid < nSize)
    {
        nMid += static_cast<uint64_t>(5000000) / sizeof(T);
        if(nMid > nSize)
            nMid = nSize;
        v.resize(nMid);
        for(; i < nMid; ++i)
            Unserialize(is, v[i], nSerType, nSerVersion);
    }
}

template<typename Stream, typename T, typename A>
inline void Unserialize(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize_impl(is, v, nSerType, nSerVersion, std::is_fundamental<T>());
}


/*
 * others derived from vector
 **/
inline uint64_t GetSerializeSize(const Legacy::Script& v, uint32_t nSerType, uint32_t nSerVersion)
{
    return GetSerializeSize((const std::vector<uint8_t>&)v, nSerType, nSerVersion);
}

template<typename Stream>
void Serialize(Stream& os, const Legacy::Script& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Serialize(os, (const std::vector<uint8_t>&)v, nSerType, nSerVersion);
}

template<typename Stream>
void Unserialize(Stream& is, Legacy::Script& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize(is, (std::vector<uint8_t>&)v, nSerType, nSerVersion);
}


/**
 * pair
 **/
template<typename K, typename T>
uint64_t GetSerializeSize(const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    return GetSerializeSize(item.first, nSerType, nSerVersion) + GetSerializeSize(item.second, nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T>
void Serialize(Stream& os, const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    Serialize(os, item.first, nSerType, nSerVersion);
    Serialize(os, item.second, nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T>
void Unserialize(Stream& is, std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize(is, item.first, nSerType, nSerVersion);
    Unserialize(is, item.second, nSerType, nSerVersion);
}



/**
 * 3 tuple
 **/
template<typename T0, typename T1, typename T2>
uint64_t GetSerializeSize(const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    uint64_t nSize = 0;
    nSize += GetSerializeSize(std::get<0>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<1>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<2>(item), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename T0, typename T1, typename T2>
void Serialize(Stream& os, const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    Serialize(os, std::get<0>(item), nSerType, nSerVersion);
    Serialize(os, std::get<1>(item), nSerType, nSerVersion);
    Serialize(os, std::get<2>(item), nSerType, nSerVersion);
}

template<typename Stream, typename T0, typename T1, typename T2>
void Unserialize(Stream& is, std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize(is, std::get<0>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<1>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<2>(item), nSerType, nSerVersion);
}



/**
 * 4 tuple
 **/
template<typename T0, typename T1, typename T2, typename T3>
uint64_t GetSerializeSize(const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    uint32_t nSize = 0;
    nSize += GetSerializeSize(std::get<0>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<1>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<2>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<3>(item), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename T0, typename T1, typename T2, typename T3>
void Serialize(Stream& os, const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    Serialize(os, std::get<0>(item), nSerType, nSerVersion);
    Serialize(os, std::get<1>(item), nSerType, nSerVersion);
    Serialize(os, std::get<2>(item), nSerType, nSerVersion);
    Serialize(os, std::get<3>(item), nSerType, nSerVersion);
}

template<typename Stream, typename T0, typename T1, typename T2, typename T3>
void Unserialize(Stream& is, std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize(is, std::get<0>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<1>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<2>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<3>(item), nSerType, nSerVersion);
}


/**
 * map
 **/
template<typename K, typename T, typename Pred, typename A>
uint64_t GetSerializeSize(const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    uint64_t nSize = GetSizeOfCompactSize(m.size());
    for(typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        nSize += GetSerializeSize((*mi), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    WriteCompactSize(os, m.size());
    for(typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        Serialize(os, (*mi), nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    m.clear();
    uint64_t nSize = ReadCompactSize(is);
    uint64_t i = 0;
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for(; i < nSize; ++i)
    {
        std::pair<K, T> item;
        Unserialize(is, item, nSerType, nSerVersion);
        mi = m.insert(mi, item);
    }
}


/**
 * set
 **/
template<typename K, typename Pred, typename A>
uint64_t GetSerializeSize(const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    uint32_t nSize = GetSizeOfCompactSize(m.size());
    for(typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        nSize += GetSerializeSize((*it), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    WriteCompactSize(os, m.size());
    for(typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it), nSerType, nSerVersion);
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    m.clear();
    uint64_t nSize = ReadCompactSize(is);
    uint64_t i = 0;
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for(; i < nSize; ++i)
    {
        K key;
        Unserialize(is, key, nSerType, nSerVersion);
        it = m.insert(it, key);
    }
}


/**
 * Support for IMPLEMENT_SERIALIZE and READWRITE macro
 **/
class CSerActionGetSerializeSize { };
class CSerActionSerialize { };
class CSerActionUnserialize { };

template<typename Stream, typename T>
inline uint64_t SerReadWrite(Stream& s, const T& obj, uint32_t nSerType, uint32_t nSerVersion, CSerActionGetSerializeSize ser_action)
{
    return ::GetSerializeSize(obj, nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline uint64_t SerReadWrite(Stream& s, const T& obj, uint32_t nSerType, uint32_t nSerVersion, CSerActionSerialize ser_action)
{
    ::Serialize(s, obj, nSerType, nSerVersion);
    return 0;
}

template<typename Stream, typename T>
inline uint64_t SerReadWrite(Stream& s, T& obj, uint32_t nSerType, uint32_t nSerVersion, CSerActionUnserialize ser_action)
{
    ::Unserialize(s, obj, nSerType, nSerVersion);
    return 0;
}


/** ser_streamplaceholder
 *
 *  Struct to hold serialize information such as type and version
 *
 **/
struct ser_streamplaceholder
{
    uint32_t nSerType;
    uint32_t nSerVersion;
};


#endif
