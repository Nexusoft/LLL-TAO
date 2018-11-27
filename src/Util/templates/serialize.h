/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_TEMPLATES_SERIALIZE_H
#define NEXUS_UTIL_TEMPLATES_SERIALIZE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <ios>
#include <cassert>
#include <limits>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include <type_traits>
#include <tuple>

#include <LLC/types/uint1024.h>
#include <Util/include/allocators.h>
#include <Util/include/debug.h>


/* forward declaration */
namespace Legacy
{
    namespace Types
    {
        class CScript;
    }
}


/** REF
 *
 *  Used to bypass the rule against non-const reference to temporary
 *  where it makes sense with wrappers such as CFlatData or CTxDB
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
 * i.e. anything that supports .read(char*, int) and .write(char*, int)
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

    // modifiers
    SER_SKIPSIG         = (1 << 16),
    SER_BLOCKHEADERONLY = (1 << 17),

    // LLP actions
    SER_LLP_HEADER_ONLY = (1 << 20),

    //Register actions
    SER_REGISTER_PRUNED = (1 << 21),

    // sigchain options
    SER_GENESISHASH     = (1 << 22)
};


/* Use this in the source file to keep dependencies clean */
#define SERIALIZE_SOURCE(classname, statements)                                \
    uint32_t classname::GetSerializeSize(int nSerType, int nSerVersion) const  \
    {                                                                          \
        CSerActionGetSerializeSize ser_action;                                 \
        const bool fGetSize = true;                                            \
        const bool fWrite = false;                                             \
        const bool fRead = false;                                              \
        uint32_t nSerSize = 0;                                                 \
        ser_streamplaceholder s;                                               \
        assert(fGetSize||fWrite||fRead); /* suppress warning */                \
        s.nSerType = nSerType;                                                 \
        s.nSerVersion = nSerVersion;                                           \
        {statements}                                                           \
        return nSerSize;                                                       \
    }                                                                          \
    template<typename Stream>                                                  \
    void classname::Serialize(Stream& s, int nSerType, int nSerVersion) const  \
    {                                                                          \
        CSerActionSerialize ser_action;                                        \
        const bool fGetSize = false;                                           \
        const bool fWrite = true;                                              \
        const bool fRead = false;                                              \
        uint32_t nSerSize = 0;                                                 \
        assert(fGetSize||fWrite||fRead); /* suppress warning */                \
        {statements}                                                           \
    }                                                                          \
    template<typename Stream>                                                  \
    void classname::Unserialize(Stream& s, int nSerType, int nSerVersion)      \
    {                                                                          \
        CSerActionUnserialize ser_action;                                      \
        const bool fGetSize = false;                                           \
        const bool fWrite = false;                                             \
        const bool fRead = true;                                               \
        uint32_t nSerSize = 0;                                                 \
        assert(fGetSize||fWrite||fRead); /* suppress warning */                \
        {statements}                                                           \
    }


/* This should be used in header only files with complete types
 * best to avoid the use of it if not needed, kept for back-compatability */
#define IMPLEMENT_SERIALIZE(statements)                                        \
    uint32_t GetSerializeSize(int nSerType, int nSerVersion) const             \
    {                                                                          \
        CSerActionGetSerializeSize ser_action;                                 \
        const bool fGetSize = true;                                            \
        const bool fWrite = false;                                             \
        const bool fRead = false;                                              \
        uint32_t nSerSize = 0;                                                 \
        ser_streamplaceholder s;                                               \
        assert(fGetSize||fWrite||fRead); /* suppress warning */                \
        s.nSerType = nSerType;                                                 \
        s.nSerVersion = nSerVersion;                                           \
        {statements}                                                           \
        return nSerSize;                                                       \
    }                                                                          \
    template<typename Stream>                                                  \
    void Serialize(Stream& s, int nSerType, int nSerVersion) const             \
    {                                                                          \
        CSerActionSerialize ser_action;                                        \
        const bool fGetSize = false;                                           \
        const bool fWrite = true;                                              \
        const bool fRead = false;                                              \
        uint32_t nSerSize = 0;                                                 \
        assert(fGetSize||fWrite||fRead); /* suppress warning */                \
        {statements}                                                           \
    }                                                                          \
    template<typename Stream>                                                  \
    void Unserialize(Stream& s, int nSerType, int nSerVersion)                 \
    {                                                                          \
        CSerActionUnserialize ser_action;                                      \
        const bool fGetSize = false;                                           \
        const bool fWrite = false;                                             \
        const bool fRead = true;                                               \
        uint32_t nSerSize = 0;                                                 \
        assert(fGetSize||fWrite||fRead); /* suppress warning */                \
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

inline uint32_t GetSerializeSize( int8_t a,                                     int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint8_t a,                                     int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize( int16_t a,                                    int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint16_t a,                                    int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize( int32_t a,                                    int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint32_t a,                                    int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(int64_t a,                                     int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint64_t a,                                    int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(float a,                                       int, int=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(double a,                                      int, int=0) { return sizeof(a); }

template<typename Stream> inline void Serialize(Stream& s,  int8_t  a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint8_t  a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s,  int16_t a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint16_t a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s,  int32_t a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint32_t a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int64_t a,           int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint64_t a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, float a,             int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, double a,            int, int=0) { WRITEDATA(s, a); }

template<typename Stream> inline void Unserialize(Stream& s,  int8_t&  a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint8_t&  a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s,  int16_t& a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint16_t& a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s,  int32_t& a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint32_t& a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, int64_t& a,        int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint64_t& a,       int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, float& a,          int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, double& a,         int, int=0) { READDATA(s, a); }

inline uint32_t GetSerializeSize(bool a, int, int=0)                          { return sizeof(char); }
template<typename Stream> inline void Serialize(Stream& s, bool a, int, int=0)    { char f=a; WRITEDATA(s, f); }
template<typename Stream> inline void Unserialize(Stream& s, bool& a, int, int=0) { char f; READDATA(s, f); a=f; }



#ifndef THROW_WITH_STACKTRACE
#define THROW_WITH_STACKTRACE(exception)  \
{                                         \
    debug::LogStackTrace();               \
    throw (exception);                    \
}
#endif


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
inline uint32_t GetSizeOfCompactSize(uint64_t nSize)
{
    if (nSize < 253)
        return sizeof(uint8_t);
    else if (nSize <= std::numeric_limits<uint16_t>::max())
        return sizeof(uint8_t) + sizeof(uint16_t);
    else if (nSize <= std::numeric_limits<uint32_t>::max())
        return sizeof(uint8_t) + sizeof(uint32_t);
    else
        return sizeof(uint8_t) + sizeof(uint64_t);
}

/** WriteCompactSize
 *
 *  Write the compact size
 *
 *  @param[in] os The output stream
 *
 *  @param[in] nSize The compact size to write
 *
 **/
template<typename Stream>
void WriteCompactSize(Stream& os, uint64_t nSize)
{
    if (nSize < 253)
    {
        uint8_t chSize = nSize;
        WRITEDATA(os, chSize);
    }
    else if (nSize <= std::numeric_limits<uint16_t>::max())
    {
        uint8_t chSize = 253;
        uint16_t xSize = nSize;
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else if (nSize <= std::numeric_limits<uint32_t>::max())
    {
        uint8_t chSize = 254;
        uint32_t xSize = nSize;
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
 *  Read the compact size
 *
 *  @param[in] is The input Stream
 *
 *  @return The compact size
 *
 **/
template<typename Stream>
uint64_t ReadCompactSize(Stream& is)
{
    uint8_t chSize;
    READDATA(is, chSize);
    uint64_t nSizeRet = 0;
    if (chSize < 253)
    {
        nSizeRet = chSize;
    }
    else if (chSize == 253)
    {
        uint16_t xSize;
        READDATA(is, xSize);
        nSizeRet = xSize;
    }
    else if (chSize == 254)
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
    if (nSizeRet > (uint64_t)MAX_SIZE)
        THROW_WITH_STACKTRACE(std::ios_base::failure("ReadCompactSize() : size too large"));
    return nSizeRet;
}


#define FLATDATA(obj)   REF(CFlatData((char*)&(obj), (char*)&(obj) + sizeof(obj)))

/** CFlatData
 *
 *  Wrapper for serializing arrays and POD.
 *  There's a clever template way to make arrays serialize normally, but MSVC6 doesn't support it.
 *
 **/
class CFlatData
{
protected:
    char* pbegin;
    char* pend;
public:


    /** CFlatData
     *
     *  Default constructor, initializes begin and end pointers
     *
     *  @param[in] pbeginIn begin pointer
     *
     * @param[in] pendIn end pointer
     *
     **/
    CFlatData(void* pbeginIn, void* pendIn)
    : pbegin((char*)pbeginIn)
    , pend((char*)pendIn) { }


    /** begin
     *
     *  Get the begin pointer
     *
     * return The begin pointer
     *
     **/
    char* begin() { return pbegin; }


    /** begin
     *
     *  Get the begin pointer
     *
     * return The constant begin pointer
     *
     **/
    const char* begin() const { return pbegin; }


    /** end
     *
     *  Get the end pointer
     *
     * return The end pointer
     *
     **/
    char* end() { return pend; }


    /** end
     *
     *  Get the end pointer
     *
     * return The constant end pointer
     *
     **/
    const char* end() const { return pend; }


    /** GetSerializeSize
     *
     *  The the size, in bytes, of the serialize object
     *
     *  @return Number of bytes
     *
     **/
    uint32_t GetSerializeSize(int, int=0) const
    {
        return pend - pbegin;
    }


    /** Serialize
     *
     *  Use the template stream to write the serialize object
     *
     *  @param[in] s The templated output stream object
     *
     **/
    template<typename Stream>
    void Serialize(Stream& s, int, int=0) const
    {
        s.write(pbegin, pend - pbegin);
    }


    /** Unserialize
     *
     *  Use the template stream to read the serialize object
     *
     *  @param[in] The templated input stream object
     *
     **/
    template<typename Stream>
    void Unserialize(Stream& s, int, int=0)
    {
        s.read(pbegin, pend - pbegin);
    }
};

/**
 *  Function prototypes
 **/

/** string **/
template<typename C> uint32_t GetSerializeSize(const std::basic_string<C>& str, int, int=0);
template<typename Stream, typename C> void Serialize(Stream& os, const std::basic_string<C>& str, int, int=0);
template<typename Stream, typename C> void Unserialize(Stream& is, std::basic_string<C>& str, int, int=0);

/** vector **/
template<typename T, typename A> uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::true_type&);
template<typename T, typename A> uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::false_type&);
template<typename T, typename A> inline uint32_t GetSerializeSize(const std::vector<T, A>& v, int nSerType, int nSerVersion);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::true_type&);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::false_type&);
template<typename Stream, typename T, typename A> inline void Serialize(Stream& os, const std::vector<T, A>& v, int nSerType, int nSerVersion);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nSerType, int nSerVersion, const std::true_type&);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nSerType, int nSerVersion, const std::false_type&);
template<typename Stream, typename T, typename A> inline void Unserialize(Stream& is, std::vector<T, A>& v, int nSerType, int nSerVersion);

/** others derived from vector **/
extern inline uint32_t GetSerializeSize(const Legacy::Types::CScript& v, int nSerType, int nSerVersion);
template<typename Stream> void Serialize(Stream& os, const Legacy::Types::CScript& v, int nSerType, int nSerVersion);
template<typename Stream> void Unserialize(Stream& is, Legacy::Types::CScript& v, int nSerType, int nSerVersion);

/** pair **/
template<typename K, typename T> uint32_t GetSerializeSize(const std::pair<K, T>& item, int nSerType, int nSerVersion);
template<typename Stream, typename K, typename T> void Serialize(Stream& os, const std::pair<K, T>& item, int nSerType, int nSerVersion);
template<typename Stream, typename K, typename T> void Unserialize(Stream& is, std::pair<K, T>& item, int nSerType, int nSerVersion);

/** 3 tuple **/
template<typename T0, typename T1, typename T2> uint32_t GetSerializeSize(const std::tuple<T0, T1, T2>& item, int nSerType, int nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2> void Serialize(Stream& os, const std::tuple<T0, T1, T2>& item, int nSerType, int nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2> void Unserialize(Stream& is, std::tuple<T0, T1, T2>& item, int nSerType, int nSerVersion);

/** 4 tuple **/
template<typename T0, typename T1, typename T2, typename T3> uint32_t GetSerializeSize(const std::tuple<T0, T1, T2, T3>& item, int nSerType, int nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2, typename T3> void Serialize(Stream& os, const std::tuple<T0, T1, T2, T3>& item, int nSerType, int nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2, typename T3> void Unserialize(Stream& is, std::tuple<T0, T1, T2, T3>& item, int nSerType, int nSerVersion);

/** map **/
template<typename K, typename T, typename Pred, typename A> uint32_t GetSerializeSize(const std::map<K, T, Pred, A>& m, int nSerType, int nSerVersion);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, int nSerType, int nSerVersion);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, int nSerType, int nSerVersion);

/** set **/
template<typename K, typename Pred, typename A> uint32_t GetSerializeSize(const std::set<K, Pred, A>& m, int nSerType, int nSerVersion);
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m, int nSerType, int nSerVersion);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m, int nSerType, int nSerVersion);


/**
 *  If none of the specialized versions above matched, default to calling member function.
 *  "int nSerType" is changed to "long nSerType" to keep from getting an ambiguous overload error.
 *  The compiler will only cast int to long if none of the other templates matched.
 **/
template<typename T>
inline uint32_t GetSerializeSize(const T& a, long nSerType, int nSerVersion)
{
    return a.GetSerializeSize((int)nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline void Serialize(Stream& os, const T& a, long nSerType, int nSerVersion)
{
    a.Serialize(os, (int)nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline void Unserialize(Stream& is, T& a, long nSerType, int nSerVersion)
{
    a.Unserialize(is, (int)nSerType, nSerVersion);
}


/**
 * string
 **/
template<typename C>
uint32_t GetSerializeSize(const std::basic_string<C>& str, int, int)
{
    return GetSizeOfCompactSize(str.size()) + str.size() * sizeof(str[0]);
}

template<typename Stream, typename C>
void Serialize(Stream& os, const std::basic_string<C>& str, int, int)
{
    WriteCompactSize(os, str.size());
    if (!str.empty())
        os.write((char*)&str[0], str.size() * sizeof(str[0]));
}

template<typename Stream, typename C>
void Unserialize(Stream& is, std::basic_string<C>& str, int, int)
{
    uint32_t nSize = ReadCompactSize(is);
    str.resize(nSize);
    if (nSize != 0)
        is.read((char*)&str[0], nSize * sizeof(str[0]));
}


/**
 * vector
 **/
template<typename T, typename A>
uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::true_type&)
{
    return (GetSizeOfCompactSize(v.size()) + v.size() * sizeof(T));
}

template<typename T, typename A>
uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::false_type&)
{
    uint32_t nSize = GetSizeOfCompactSize(v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        nSize += GetSerializeSize((*vi), nSerType, nSerVersion);
    return nSize;
}

template<typename T, typename A>
inline uint32_t GetSerializeSize(const std::vector<T, A>& v, int nSerType, int nSerVersion)
{
    return GetSerializeSize_impl(v, nSerType, nSerVersion, std::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::true_type&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write((char*)&v[0], v.size() * sizeof(T));
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nSerType, int nSerVersion, const std::false_type&)
{
    WriteCompactSize(os, v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        ::Serialize(os, (*vi), nSerType, nSerVersion);
}

template<typename Stream, typename T, typename A>
inline void Serialize(Stream& os, const std::vector<T, A>& v, int nSerType, int nSerVersion)
{
    Serialize_impl(os, v, nSerType, nSerVersion, std::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nSerType, int nSerVersion, const std::true_type&)
{
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    uint32_t nSize = ReadCompactSize(is);
    uint32_t i = 0;
    while (i < nSize)
    {
        uint32_t blk = std::min(nSize - i, (uint32_t)(1 + 4999999 / sizeof(T)));
        v.resize(i + blk);
        is.read((char*)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nSerType, int nSerVersion, const std::false_type&)
{
    v.clear();
    uint32_t nSize = ReadCompactSize(is);
    uint32_t i = 0;
    uint32_t nMid = 0;
    while (nMid < nSize)
    {
        nMid += 5000000 / sizeof(T);
        if (nMid > nSize)
            nMid = nSize;
        v.resize(nMid);
        for (; i < nMid; i++)
            Unserialize(is, v[i], nSerType, nSerVersion);
    }
}

template<typename Stream, typename T, typename A>
inline void Unserialize(Stream& is, std::vector<T, A>& v, int nSerType, int nSerVersion)
{
    Unserialize_impl(is, v, nSerType, nSerVersion, std::is_fundamental<T>());
}


/*
 * others derived from vector
 **/
inline uint32_t GetSerializeSize(const Legacy::Types::CScript& v, int nSerType, int nSerVersion)
{
    return GetSerializeSize((const std::vector<uint8_t>&)v, nSerType, nSerVersion);
}

template<typename Stream>
void Serialize(Stream& os, const Legacy::Types::CScript& v, int nSerType, int nSerVersion)
{
    Serialize(os, (const std::vector<uint8_t>&)v, nSerType, nSerVersion);
}

template<typename Stream>
void Unserialize(Stream& is, Legacy::Types::CScript& v, int nSerType, int nSerVersion)
{
    Unserialize(is, (std::vector<uint8_t>&)v, nSerType, nSerVersion);
}


/**
 * pair
 **/
template<typename K, typename T>
uint32_t GetSerializeSize(const std::pair<K, T>& item, int nSerType, int nSerVersion)
{
    return GetSerializeSize(item.first, nSerType, nSerVersion) + GetSerializeSize(item.second, nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T>
void Serialize(Stream& os, const std::pair<K, T>& item, int nSerType, int nSerVersion)
{
    Serialize(os, item.first, nSerType, nSerVersion);
    Serialize(os, item.second, nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T>
void Unserialize(Stream& is, std::pair<K, T>& item, int nSerType, int nSerVersion)
{
    Unserialize(is, item.first, nSerType, nSerVersion);
    Unserialize(is, item.second, nSerType, nSerVersion);
}



/**
 * 3 tuple
 **/
template<typename T0, typename T1, typename T2>
uint32_t GetSerializeSize(const std::tuple<T0, T1, T2>& item, int nSerType, int nSerVersion)
{
    uint32_t nSize = 0;
    nSize += GetSerializeSize(std::get<0>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<1>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<2>(item), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename T0, typename T1, typename T2>
void Serialize(Stream& os, const std::tuple<T0, T1, T2>& item, int nSerType, int nSerVersion)
{
    Serialize(os, std::get<0>(item), nSerType, nSerVersion);
    Serialize(os, std::get<1>(item), nSerType, nSerVersion);
    Serialize(os, std::get<2>(item), nSerType, nSerVersion);
}

template<typename Stream, typename T0, typename T1, typename T2>
void Unserialize(Stream& is, std::tuple<T0, T1, T2>& item, int nSerType, int nSerVersion)
{
    Unserialize(is, std::get<0>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<1>(item), nSerType, nSerVersion);
    Unserialize(is, std::get<2>(item), nSerType, nSerVersion);
}



/**
 * 4 tuple
 **/
template<typename T0, typename T1, typename T2, typename T3>
uint32_t GetSerializeSize(const std::tuple<T0, T1, T2, T3>& item, int nSerType, int nSerVersion)
{
    uint32_t nSize = 0;
    nSize += GetSerializeSize(std::get<0>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<1>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<2>(item), nSerType, nSerVersion);
    nSize += GetSerializeSize(std::get<3>(item), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename T0, typename T1, typename T2, typename T3>
void Serialize(Stream& os, const std::tuple<T0, T1, T2, T3>& item, int nSerType, int nSerVersion)
{
    Serialize(os, std::get<0>(item), nSerType, nSerVersion);
    Serialize(os, std::get<1>(item), nSerType, nSerVersion);
    Serialize(os, std::get<2>(item), nSerType, nSerVersion);
    Serialize(os, std::get<3>(item), nSerType, nSerVersion);
}

template<typename Stream, typename T0, typename T1, typename T2, typename T3>
void Unserialize(Stream& is, std::tuple<T0, T1, T2, T3>& item, int nSerType, int nSerVersion)
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
uint32_t GetSerializeSize(const std::map<K, T, Pred, A>& m, int nSerType, int nSerVersion)
{
    uint32_t nSize = GetSizeOfCompactSize(m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        nSize += GetSerializeSize((*mi), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, int nSerType, int nSerVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        Serialize(os, (*mi), nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, int nSerType, int nSerVersion)
{
    m.clear();
    uint32_t nSize = ReadCompactSize(is);
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for (uint32_t i = 0; i < nSize; i++)
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
uint32_t GetSerializeSize(const std::set<K, Pred, A>& m, int nSerType, int nSerVersion)
{
    uint32_t nSize = GetSizeOfCompactSize(m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        nSize += GetSerializeSize((*it), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m, int nSerType, int nSerVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it), nSerType, nSerVersion);
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m, int nSerType, int nSerVersion)
{
    m.clear();
    uint32_t nSize = ReadCompactSize(is);
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for (uint32_t i = 0; i < nSize; i++)
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
inline uint32_t SerReadWrite(Stream& s, const T& obj, int nSerType, int nSerVersion, CSerActionGetSerializeSize ser_action)
{
    return ::GetSerializeSize(obj, nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline uint32_t SerReadWrite(Stream& s, const T& obj, int nSerType, int nSerVersion, CSerActionSerialize ser_action)
{
    ::Serialize(s, obj, nSerType, nSerVersion);
    return 0;
}

template<typename Stream, typename T>
inline uint32_t SerReadWrite(Stream& s, T& obj, int nSerType, int nSerVersion, CSerActionUnserialize ser_action)
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
    int nSerType;
    int nSerVersion;
};


/** DataStream
 *
 *  Class to handle the serializaing and deserializing of data to disk or over the network
 *
 **/
class DataStream : public std::vector<uint8_t>
{

    /** The current reading position. **/
    uint32_t nReadPos;


    /** The serialization type. **/
    int32_t nSerType;


    /** The serializtion version **/
    int32_t nSerVersion;


public:

    /** Default Constructor. **/
    DataStream(int32_t nSerTypeIn, int32_t nSerVersionIn)
    : nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersion)
    {
        clear();
    }


    /** DataStream
     *
     *  Constructs the DataStream object
     *
     *  @param[in] vchDataIn The byte vector to insert
     *
     *  @param[in] nSerTypeIn The serialize type
     *
     *  @param[in] nSerVersionIn The serialize version
     *
     **/
    DataStream(std::vector<uint8_t> vchDataIn, int32_t nSerTypeIn, int32_t nSerVersionIn)
    : nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersion)
    , std::vector<uint8_t>(vchDataIn) {  }


    /** SetNull
     *
     *  Sets the object into null state.
     *
     **/
    void SetNull()
    {
        nReadPos = 0;
        clear();
    }


    /** IsNull
     *
     *  Returns if object is in null state.
     *
     **/
    bool IsNull()
    {
        return nReadPos == 0 && size() == 0;
    }


    /** Reset
     *
     *  Resets the internal read pointer
     *
     **/
    void Reset()
    {
        nReadPos = 0;
    }


    /** End
     *
     *  Returns if end of stream is found
     *
     **/
    bool End()
    {
        return nReadPos == size();
    }


    /** read
     *
     *  Reads raw data from the stream
     *
     *  @param[in] pch The pointer to beginning of memory to write
     *
     *  @param[in] nSize The total number of bytes to read
     *
     **/
    DataStream& read(char* pch, int nSize)
    {
       /* Copy the bytes into tmp object. */
       std::copy((uint8_t*)&at(nReadPos), (uint8_t*)&at(nReadPos) + nSize, (uint8_t*)pch);

       /* Iterate the read position. */
       nReadPos += nSize;

       return *this;
    }


    /** write
     *
     *  Writes data into the stream
     *
     *  @param[in] pch The pointer to beginning of memory to write
     *
     *  @param[in] nSize The total number of bytes to copy
     *
     **/
    DataStream& write(const char* pch, int nSize)
    {
        /* Push the obj bytes into the vector. */
        insert(end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

        return *this;
    }


    /** Operator Overload <<
     *
     *  Serializes data into vchLedgerData
     *
     *  @param[in] obj The object to serialize into ledger data
     *
     **/
    template<typename Type> DataStream& operator<<(const Type& obj)
    {
        /* Serialize to the stream. */
        ::Serialize(*this, obj, nSerType, nSerVersion); //temp versinos for now

        return (*this);
    }


    /** Operator Overload >>
     *
     *  Serializes data into vchLedgerData
     *
     *  @param[out] obj The object to de-serialize from ledger data
     *
     **/
    template<typename Type> DataStream& operator>>(Type& obj)
    {
        /* Unserialize from the stream. */
        ::Unserialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }
};


/** CDataStream
 *
 *  Double ended buffer combining vector and stream-like interfaces.
 *  >> and << read and write unformatted data using the above serialization templates.
 *  Fills with data in linear time; some stringstream implementations take N^2 time.
 *
 **/
class CDataStream
{
protected:
    typedef std::vector<char, zero_after_free_allocator<char> > vector_type;
    vector_type vch;
    uint32_t nReadPos;
    short state;
    short exceptmask;
public:
    int nSerType;
    int nSerVersion;

    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;


    /** CDataStream
     *
     *  Explicit constructor for default initialization with serialize type and version
     *
     **/
    explicit CDataStream(int nTypeIn, int nVersionIn)
    {
        Init(nTypeIn, nVersionIn);
    }


    /** CDataStream
     *
     *  Default constructor for initialization with serialize data, type and version
     *
     **/
    CDataStream(const_iterator pbegin, const_iterator pend, int nTypeIn, int nVersionIn) : vch(pbegin, pend)
    {
        Init(nTypeIn, nVersionIn);
    }


#if !defined(_MSC_VER) || _MSC_VER >= 1300

    /** CDataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *  (Microsoft compiler compatible)
     *
     **/
    CDataStream(const char* pbegin, const char* pend, int nTypeIn, int nVersionIn) : vch(pbegin, pend)
    {
        Init(nTypeIn, nVersionIn);
    }
#endif


    /** CDataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *
     **/
    CDataStream(const vector_type& vchIn, int nTypeIn, int nVersionIn) : vch(vchIn.begin(), vchIn.end())
    {
        Init(nTypeIn, nVersionIn);
    }


    /** CDataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *
     **/
    CDataStream(const std::vector<char>& vchIn, int nTypeIn, int nVersionIn) : vch(vchIn.begin(), vchIn.end())
    {
        Init(nTypeIn, nVersionIn);
    }


    /** CDataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *
     **/
    CDataStream(const std::vector<uint8_t>& vchIn, int nTypeIn, int nVersionIn) : vch((char*)&vchIn.begin()[0], (char*)&vchIn.end()[0])
    {
        Init(nTypeIn, nVersionIn);
    }


    /** Init
     *
     *  Default initialization with serialize type and versions
     *
     *  @param[in] nTypeIn The serialize type
     *
     *  @param[in] nVersionIn The serialize version
     *
     **/
    void Init(int nTypeIn, int nVersionIn)
    {
        nReadPos = 0;
        nSerType = nTypeIn;
        nSerVersion = nVersionIn;
        state = 0;
        exceptmask = std::ios::badbit | std::ios::failbit;
    }


    /** operator+=
     *
     *  Append one datastream to the other
     *
     **/
    CDataStream& operator+=(const CDataStream& b)
    {
        vch.insert(vch.end(), b.begin(), b.end());
        return *this;
    }


    /** operator+=
     *
     *  Append one datastream to the other
     *
     **/
    friend CDataStream operator+(const CDataStream& a, const CDataStream& b)
    {
        CDataStream ret = a;
        ret += b;
        return (ret);
    }


    /** str
     *
     *  Returns a standard string of the stream data
     *
     **/
    std::string str() const
    {
        return (std::string(begin(), end()));
    }


    /**
     *  Vector subset
     **/
    const_iterator begin() const                     { return vch.begin() + nReadPos; }
    iterator begin()                                 { return vch.begin() + nReadPos; }
    const_iterator end() const                       { return vch.end(); }
    iterator end()                                   { return vch.end(); }
    size_type size() const                           { return vch.size() - nReadPos; }
    bool empty() const                               { return vch.size() == nReadPos; }
    void resize(size_type n, value_type c=0)         { vch.resize(n + nReadPos, c); }
    void reserve(size_type n)                        { vch.reserve(n + nReadPos); }
    const_reference operator[](size_type pos) const  { return vch[pos + nReadPos]; }
    reference operator[](size_type pos)              { return vch[pos + nReadPos]; }
    void clear()                                     { vch.clear(); nReadPos = 0; }
    iterator insert(iterator it, const char& x=char()) { return vch.insert(it, x); }
    void insert(iterator it, size_type n, const char& x) { vch.insert(it, n, x); }
    vector_type data() { return vch; }


    /** insert
     *
     *  Insert data from first to last at the iterator it
     *
     *  @param[in] it The iterator insert point
     *
     *  @param[in] first The start of the insertion data
     *
     *  @param[in] last The end of the insertion data
     *
     **/
    void insert(iterator it, const_iterator first, const_iterator last)
    {
        if (it == vch.begin() + nReadPos && last - first <= nReadPos)
        {
            // special case for inserting at the front when there's room
            nReadPos -= (last - first);
            memcpy(&vch[nReadPos], &first[0], last - first);
        }
        else
            vch.insert(it, first, last);
    }


#if !defined(MAC_OSX)

    /** insert
     *
     *  Insert data from first to last at the iterator it.
     *  Fix for MaxOSX Compatability for XCode and Deployment 10.10.
     *
     *  @param[in] it The iterator insert point
     *
     *  @param[in] first The start of the insertion data
     *
     *  @param[in] last The end of the insertion data
     *
     **/
    void insert(iterator it, std::vector<char>::const_iterator first, std::vector<char>::const_iterator last)
    {
        if (it == vch.begin() + nReadPos && last - first <= nReadPos)
        {
            // special case for inserting at the front when there's room
            nReadPos -= (last - first);
            memcpy(&vch[nReadPos], &first[0], last - first);
        }
        else
            vch.insert(it, first, last);
    }
#endif


#if !defined(_MSC_VER) || _MSC_VER >= 1300

    /** insert
     *
     *  Insert data from first to last at the iterator it.
     *  Support for Microsoft compiler compatability.
     *
     *  @param[in] it The iterator insert point
     *
     *  @param[in] first The start of the insertion data
     *
     *  @param[in] last The end of the insertion data
     *
     **/
    void insert(iterator it, const char* first, const char* last)
    {
        if (it == vch.begin() + nReadPos && last - first <= nReadPos)
        {
            // special case for inserting at the front when there's room
            nReadPos -= (last - first);
            memcpy(&vch[nReadPos], &first[0], last - first);
        }
        else
            vch.insert(it, first, last);
    }
#endif


    /** erase
     *
     *  Erase the data element if it exists and return the erased location
     *
     *  @param[in] it The iterator erase point
     *
     *  @return The location of the erased element or begin if buffer is cleared
     *
     **/
    iterator erase(iterator it)
    {
        if (it == vch.begin() + nReadPos)
        {
            // special case for erasing from the front
            if (++nReadPos >= vch.size())
            {
                // whenever we reach the end, we take the opportunity to clear the buffer
                nReadPos = 0;
                return vch.erase(vch.begin(), vch.end());
            }
            return vch.begin() + nReadPos;
        }
        else
            return vch.erase(it);
    }


    /** erase
     *
     *  Erase the data element if it exists and return the erased location
     *
     *  @param[in] first The begin of iterator erase range
     *
     *  @param[in] last The end of erase range
     *
     *  @return The location of the erased elements or begin if buffer is cleared
     *
     **/
    iterator erase(iterator first, iterator last)
    {
        if (first == vch.begin() + nReadPos)
        {
            // special case for erasing from the front
            if (last == vch.end())
            {
                nReadPos = 0;
                return vch.erase(vch.begin(), vch.end());
            }
            else
            {
                nReadPos = (last - vch.begin());
                return last;
            }
        }
        else
            return vch.erase(first, last);
    }


    /** Compact
     *
     *  Erase the buffer data before the read position to compact the size
     *
     **/
    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + nReadPos);
        nReadPos = 0;
    }


    /** Rewind
     *
     *  Move the read position back to the given position n
     *
     *  @param[in] n The position to move the read position back to
     *
     *  return True if n less than or equal to read position, false otherwise
     *
     **/
    bool Rewind(size_type n)
    {
        // Rewind by n characters if the buffer hasn't been compacted yet
        if (n > nReadPos)
            return false;
        nReadPos -= n;
        return true;
    }


    /**
     *  Stream subset
     **/
    void setstate(short bits, const char* psz)
    {
        state |= bits;
        if (state & exceptmask)
            THROW_WITH_STACKTRACE(std::ios_base::failure(psz));
    }

    bool eof() const             { return size() == 0; }
    bool fail() const            { return state & (std::ios::badbit | std::ios::failbit); }
    bool good() const            { return !eof() && (state == 0); }
    void clear(short n)          { state = n; }  // name conflict with vector clear()
    short exceptions()           { return exceptmask; }
    short exceptions(short mask) { short prev = exceptmask; exceptmask = mask; setstate(0, "CDataStream"); return prev; }
    CDataStream* rdbuf()         { return this; }
    int in_avail()               { return size(); }

    void SetType(int n)          { nSerType = n; }
    int GetType()                { return nSerType; }
    void SetVersion(int n)       { nSerVersion = n; }
    int GetVersion()             { return nSerVersion; }
    void ReadVersion()           { *this >> nSerVersion; }
    void WriteVersion()          { *this << nSerVersion; }


    /** read
     *
     *  Read into a section of raw data out from the stream
     *
     *  @param[in] pch The pointer to the begin of the raw data
     *
     *  @param[in] nSize The size in bytes of the raw data to write
     *
     *  return Returns a reference to the CDataStream object
     *
     **/
    CDataStream& read(char* pch, int nSize)
    {
        // Read from the beginning of the buffer
        assert(nSize >= 0);
        uint32_t nReadPosNext = nReadPos + nSize;
        if (nReadPosNext >= vch.size())
        {
            if (nReadPosNext > vch.size())
            {
                setstate(std::ios::failbit, "CDataStream::read() : end of data");
                memset(pch, 0, nSize);
                nSize = vch.size() - nReadPos;
            }
            memcpy(pch, &vch[nReadPos], nSize);
            nReadPos = 0;
            vch.clear();
            return (*this);
        }
        memcpy(pch, &vch[nReadPos], nSize);
        nReadPos = nReadPosNext;
        return (*this);
    }


    /** ignore
     *
     *  ignore a section of data inside the CDataStream
     *
     *  @param[in] nSize The size in bytes of the data to ignore
     *
     *  return Returns a reference to the CDataStream object
     *
     **/
    CDataStream& ignore(int nSize)
    {
        // Ignore from the beginning of the buffer
        assert(nSize >= 0);
        uint32_t nReadPosNext = nReadPos + nSize;
        if (nReadPosNext >= vch.size())
        {
            if (nReadPosNext > vch.size())
            {
                setstate(std::ios::failbit, "CDataStream::ignore() : end of data");
                nSize = vch.size() - nReadPos;
            }
            nReadPos = 0;
            vch.clear();
            return (*this);
        }
        nReadPos = nReadPosNext;
        return (*this);
    }


    /** write
     *
     *  Write a section of raw data to the end of the buffer
     *
     *  @param[in] pch The pointer to the begin of the raw data
     *
     *  @param[in] nSize The size in bytes of the raw data to read
     *
     *  return Returns a reference to the CDataStream object
     *
     **/
    CDataStream& write(const char* pch, int nSize)
    {
        // Write to the end of the buffer
        assert(nSize >= 0);
        vch.insert(vch.end(), pch, pch + nSize);
        return (*this);
    }


    /** Serialize
     *
     *  Serialize an object into this CDataStram
     *
     *  @param[in] s The stream to write with
     *
     *  @param[in] nSerType The serialize type
     *
     *  @param[in] nSerVersion The serialize version
     *
     **/
    template<typename Stream>
    void Serialize(Stream& s, int nSerType, int nSerVersion) const
    {
        // Special case: stream << stream concatenates like stream += stream
        if (!vch.empty())
            s.write((char*)&vch[0], vch.size() * sizeof(vch[0]));
    }


    /** GetSerializeSize
     *
     *  Get the size in bytes of the serialize object
     *
     *  @param[in] obj The object to get the size of
     *
     *  @return The size in bytes
     *
     **/
    template<typename T>
    uint32_t GetSerializeSize(const T& obj)
    {
        // Tells the size of the object if serialized to this stream
        return ::GetSerializeSize(obj, nSerType, nSerVersion);
    }


    /** operator<<
     *
     *  Serialize to this stream
     *
     **/
    template<typename T>
    CDataStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }

    /** operator>>
     *
     *  Unserialize from this stream
     *
     **/
    template<typename T>
    CDataStream& operator>>(T& obj)
    {
        ::Unserialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }
};


/** CAutoFile
 *
 *  RAII wrapper for FILE*. Will automatically close the file when it goes out of
 *  scope if not null.
 *  If you're returning the file pointer, return file.release().
 *  If you need to close the file early, use file.fclose() instead of fclose(file).
 *
 */
class CAutoFile
{
protected:
    FILE* file;
    short state;
    short exceptmask;
public:
    int nSerType;
    int nSerVersion;


    /** CAutoFile
     *
     *  Default construtor
     *
     *  @param[in] filenew The new file pointer to associate with
     *
     *  @param[in] nTypeIn The serialize type
     *
     *  @param[in] nVersionIn The serialize version
     *
     **/
    CAutoFile(FILE* filenew, int nTypeIn, int nVersionIn)
    {
        file = filenew;
        nSerType = nTypeIn;
        nSerVersion = nVersionIn;
        state = 0;
        exceptmask = std::ios::badbit | std::ios::failbit;
    }


    /** ~CAutoFile
     *
     *  Destructor, closes the file on destruction
     *
     **/
    ~CAutoFile()
    {
        fclose();
    }


    /** fclose
     *
     *  Closes the file if it's not null or a std stream (stdin, stdout, stderr)
     *
     **/
    void fclose()
    {
        if (file != NULL && file != stdin && file != stdout && file != stderr)
            ::fclose(file);
        file = NULL;
    }


    /** release
     *
     *  Gets the file pointer then sets file pointer to null. Returns the file pointer
     *
     *  @return The file pointer
     *
     **/
    FILE* release()
    {
        FILE* ret = file;
        file = NULL;
        return ret;
    }


    /* operator overloads */
    operator FILE*()            { return file; }
    FILE* operator->()          { return file; }
    FILE& operator*()           { return *file; }
    FILE** operator&()          { return &file; }
    FILE* operator=(FILE* pnew) { return file = pnew; }
    bool operator!()            { return (file == NULL); }


    /**
     *  Stream subset
     **/
    void setstate(short bits, const char* psz)
    {
        state |= bits;
        if (state & exceptmask)
            THROW_WITH_STACKTRACE(std::ios_base::failure(psz));
    }

    bool fail() const            { return state & (std::ios::badbit | std::ios::failbit); }
    bool good() const            { return state == 0; }
    void clear(short n = 0)      { state = n; }
    short exceptions()           { return exceptmask; }
    short exceptions(short mask) { short prev = exceptmask; exceptmask = mask; setstate(0, "CAutoFile"); return prev; }

    void SetType(int n)          { nSerType = n; }
    int GetType()                { return nSerType; }
    void SetVersion(int n)       { nSerVersion = n; }
    int GetVersion()             { return nSerVersion; }
    void ReadVersion()           { *this >> nSerVersion; }
    void WriteVersion()          { *this << nSerVersion; }


    /** read
     *
     *  Reads from file into the raw data pointer
     *
     *  @param[in] pch The pointer to beginning of memory to read into
     *
     *  @param[in] nSize The total number of bytes to read
     *
     *  @return Reference to the CAutoFile
     *
     **/
    CAutoFile& read(char* pch, size_t nSize)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::read : file handle is NULL");
        if (fread(pch, 1, nSize, file) != nSize)
            setstate(std::ios::failbit, feof(file) ? "CAutoFile::read : end of file" : "CAutoFile::read : fread failed");
        return (*this);
    }


    /** write
     *
     *  Writes data into the file from the raw data pointer
     *
     *  @param[in] pch The pointer to beginning of memory to write from
     *
     *  @param[in] nSize The total number of bytes to write
     *
     *  @return Reference to the CAutoFile
     *
     **/
    CAutoFile& write(const char* pch, size_t nSize)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::write : file handle is NULL");
        if (fwrite(pch, 1, nSize, file) != nSize)
            setstate(std::ios::failbit, "CAutoFile::write : write failed");
        return (*this);
    }


    /** GetSerializeSize
     *
     *  The the size, in bytes, of the serialize object
     *
     *  @param[in] reference to the object to get the size of
     *
     *  @return Number of bytes
     *
     **/
    template<typename T>
    uint32_t GetSerializeSize(const T& obj)
    {
        // Tells the size of the object if serialized to this stream
        return ::GetSerializeSize(obj, nSerType, nSerVersion);
    }


    /** operator<<
     *
     *  Serialize to this stream
     *
     **/
    template<typename T>
    CAutoFile& operator<<(const T& obj)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::operator<< : file handle is NULL");
        ::Serialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }


    /** operator>>
     *
     *  Unserialize from this stream
     *
     **/
    template<typename T>
    CAutoFile& operator>>(T& obj)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::operator>> : file handle is NULL");
        ::Unserialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }
};

#endif
