/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
    class CScript;
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


/* Use this in the source file to keep dependencies clean */
#define SERIALIZE_SOURCE(classname, statements)                                \
    uint32_t classname::GetSerializeSize(uint32_t nSerType, uint32_t nSerVersion) const  \
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
    void classname::Serialize(Stream& s, uint32_t nSerType, uint32_t nSerVersion) const  \
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
    void classname::Unserialize(Stream& s, uint32_t nSerType, uint32_t nSerVersion)      \
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
    uint32_t GetSerializeSize(uint32_t nSerType, uint32_t nSerVersion) const             \
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
    void Serialize(Stream& s, uint32_t nSerType, uint32_t nSerVersion) const             \
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
    void Unserialize(Stream& s, uint32_t nSerType, uint32_t nSerVersion)                 \
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

inline uint32_t GetSerializeSize( int8_t a,                                     uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint8_t a,                                     uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize( int16_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint16_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize( int32_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint32_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(int64_t a,                                     uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(uint64_t a,                                    uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(float a,                                       uint32_t, uint32_t=0) { return sizeof(a); }
inline uint32_t GetSerializeSize(double a,                                      uint32_t, uint32_t=0) { return sizeof(a); }

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

inline uint32_t GetSerializeSize(bool a, uint32_t, uint32_t=0)                          { return sizeof(char); }
template<typename Stream> inline void Serialize(Stream& s, bool a, uint32_t, uint32_t=0)    { char f=a; WRITEDATA(s, f); }
template<typename Stream> inline void Unserialize(Stream& s, bool& a, uint32_t, uint32_t=0) { char f; READDATA(s, f); a=f; }



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
 *  Write the compact size.
 *
 *  @param[in] os The output stream.
 *  @param[in] nSize The compact size to write.
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
     *  Default constructor, initializes begin and end pointers.
     *
     *  @param[in] pbeginIn begin pointer.
     *  @param[in] pendIn end pointer.
     *
     **/
    CFlatData(void* pbeginIn, void* pendIn)
    : pbegin((char*)pbeginIn)
    , pend((char*)pendIn) { }


    /** begin
     *
     *  Get the begin pointer.
     *
     * return The begin pointer.
     *
     **/
    char* begin() { return pbegin; }


    /** begin
     *
     *  Get the begin pointer.
     *
     * return The constant begin pointer.
     *
     **/
    const char* begin() const { return pbegin; }


    /** end
     *
     *  Get the end pointer.
     *
     * return The end pointer.
     *
     **/
    char* end() { return pend; }


    /** end
     *
     *  Get the end pointer.
     *
     * return The constant end pointer.
     *
     **/
    const char* end() const { return pend; }


    /** GetSerializeSize
     *
     *  The the size, in bytes, of the serialize object.
     *
     *  @return The number of bytes.
     *
     **/
    uint32_t GetSerializeSize(uint32_t, uint32_t=0) const
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
    void Serialize(Stream& s, uint32_t, uint32_t=0) const
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
    void Unserialize(Stream& s, uint32_t, uint32_t=0)
    {
        s.read(pbegin, pend - pbegin);
    }
};

/**
 *  Function prototypes
 **/

/** string **/
template<typename C> uint32_t GetSerializeSize(const std::basic_string<C>& str, uint32_t, uint32_t=0);
template<typename Stream, typename C> void Serialize(Stream& os, const std::basic_string<C>& str, uint32_t, uint32_t=0);
template<typename Stream, typename C> void Unserialize(Stream& is, std::basic_string<C>& str, uint32_t, uint32_t=0);

/** vector **/
template<typename T, typename A> uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&);
template<typename T, typename A> uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&);
template<typename T, typename A> inline uint32_t GetSerializeSize(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&);
template<typename Stream, typename T, typename A> inline void Serialize(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&);
template<typename Stream, typename T, typename A> inline void Unserialize(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion);

/** others derived from vector **/
extern inline uint32_t GetSerializeSize(const Legacy::CScript& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream> void Serialize(Stream& os, const Legacy::CScript& v, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream> void Unserialize(Stream& is, Legacy::CScript& v, uint32_t nSerType, uint32_t nSerVersion);

/** pair **/
template<typename K, typename T> uint32_t GetSerializeSize(const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T> void Serialize(Stream& os, const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T> void Unserialize(Stream& is, std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion);

/** 3 tuple **/
template<typename T0, typename T1, typename T2> uint32_t GetSerializeSize(const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2> void Serialize(Stream& os, const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2> void Unserialize(Stream& is, std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion);

/** 4 tuple **/
template<typename T0, typename T1, typename T2, typename T3> uint32_t GetSerializeSize(const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2, typename T3> void Serialize(Stream& os, const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename T0, typename T1, typename T2, typename T3> void Unserialize(Stream& is, std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion);

/** map **/
template<typename K, typename T, typename Pred, typename A> uint32_t GetSerializeSize(const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);

/** set **/
template<typename K, typename Pred, typename A> uint32_t GetSerializeSize(const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion);


/**
 *  If none of the specialized versions above matched, default to calling member function.
 *  "uint32_t nSerType" is changed to "long nSerType" to keep from getting an ambiguous overload error.
 *  The compiler will only cast uint32_t to long if none of the other templates matched.
 **/
template<typename T>
inline uint32_t GetSerializeSize(const T& a, long nSerType, uint32_t nSerVersion)
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
uint32_t GetSerializeSize(const std::basic_string<C>& str, uint32_t, uint32_t)
{
    return GetSizeOfCompactSize(str.size()) + str.size() * sizeof(str[0]);
}

template<typename Stream, typename C>
void Serialize(Stream& os, const std::basic_string<C>& str, uint32_t, uint32_t)
{
    WriteCompactSize(os, str.size());
    if (!str.empty())
        os.write((char*)&str[0], str.size() * sizeof(str[0]));
}

template<typename Stream, typename C>
void Unserialize(Stream& is, std::basic_string<C>& str, uint32_t, uint32_t)
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
uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&)
{
    return (GetSizeOfCompactSize(v.size()) + v.size() * sizeof(T));
}

template<typename T, typename A>
uint32_t GetSerializeSize_impl(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&)
{
    uint32_t nSize = GetSizeOfCompactSize(v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        nSize += GetSerializeSize((*vi), nSerType, nSerVersion);
    return nSize;
}

template<typename T, typename A>
inline uint32_t GetSerializeSize(const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion)
{
    return GetSerializeSize_impl(v, nSerType, nSerVersion, std::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::true_type&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write((char*)&v[0], v.size() * sizeof(T));
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&)
{
    WriteCompactSize(os, v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
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
void Unserialize_impl(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion, const std::false_type&)
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
inline void Unserialize(Stream& is, std::vector<T, A>& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize_impl(is, v, nSerType, nSerVersion, std::is_fundamental<T>());
}


/*
 * others derived from vector
 **/
inline uint32_t GetSerializeSize(const Legacy::CScript& v, uint32_t nSerType, uint32_t nSerVersion)
{
    return GetSerializeSize((const std::vector<uint8_t>&)v, nSerType, nSerVersion);
}

template<typename Stream>
void Serialize(Stream& os, const Legacy::CScript& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Serialize(os, (const std::vector<uint8_t>&)v, nSerType, nSerVersion);
}

template<typename Stream>
void Unserialize(Stream& is, Legacy::CScript& v, uint32_t nSerType, uint32_t nSerVersion)
{
    Unserialize(is, (std::vector<uint8_t>&)v, nSerType, nSerVersion);
}


/**
 * pair
 **/
template<typename K, typename T>
uint32_t GetSerializeSize(const std::pair<K, T>& item, uint32_t nSerType, uint32_t nSerVersion)
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
uint32_t GetSerializeSize(const std::tuple<T0, T1, T2>& item, uint32_t nSerType, uint32_t nSerVersion)
{
    uint32_t nSize = 0;
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
uint32_t GetSerializeSize(const std::tuple<T0, T1, T2, T3>& item, uint32_t nSerType, uint32_t nSerVersion)
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
uint32_t GetSerializeSize(const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    uint32_t nSize = GetSizeOfCompactSize(m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        nSize += GetSerializeSize((*mi), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        Serialize(os, (*mi), nSerType, nSerVersion);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
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
uint32_t GetSerializeSize(const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    uint32_t nSize = GetSizeOfCompactSize(m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        nSize += GetSerializeSize((*it), nSerType, nSerVersion);
    return nSize;
}

template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it), nSerType, nSerVersion);
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m, uint32_t nSerType, uint32_t nSerVersion)
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
inline uint32_t SerReadWrite(Stream& s, const T& obj, uint32_t nSerType, uint32_t nSerVersion, CSerActionGetSerializeSize ser_action)
{
    return ::GetSerializeSize(obj, nSerType, nSerVersion);
}

template<typename Stream, typename T>
inline uint32_t SerReadWrite(Stream& s, const T& obj, uint32_t nSerType, uint32_t nSerVersion, CSerActionSerialize ser_action)
{
    ::Serialize(s, obj, nSerType, nSerVersion);
    return 0;
}

template<typename Stream, typename T>
inline uint32_t SerReadWrite(Stream& s, T& obj, uint32_t nSerType, uint32_t nSerVersion, CSerActionUnserialize ser_action)
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


/** DataStream
 *
 *  Class to handle the serializaing and deserializing of data to disk or over the network
 *
 **/
class DataStream
{
    /** The byte vector of data . **/
    std::vector<uint8_t> vData;


    /** The current reading position. **/
    mutable uint32_t nReadPos;


    /** The serialization type. **/
    uint32_t nSerType;


    /** The serializtion version **/
    uint32_t nSerVersion;


public:

    /** Default Constructor. **/
    DataStream(uint32_t nSerTypeIn, uint32_t nSerVersionIn)
    : vData()
    , nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersionIn)
    {
    }


    /** DataStream
     *
     *  Constructs the DataStream object.
     *
     *  @param[in] vchDataIn The byte vector to insert.
     *  @param[in] nSerTypeIn The serialize type.
     *  @param[in] nSerVersionIn The serialize version.
     *
     **/
    DataStream(const std::vector<uint8_t>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
    : vData(vchDataIn)
    , nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersionIn)
    {
    }


    /** DataStream
     *
     *  Default constructor for initialization with serialize data, type and version
     *
     **/
    DataStream( const std::vector<uint8_t>::const_iterator pbegin,
                const std::vector<uint8_t>::const_iterator pend,
                const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
    : vData(pbegin, pend)
    , nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersionIn)
    {
    }


#if !defined(_MSC_VER) || _MSC_VER >= 1300

    /** DataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *  (Microsoft compiler compatible)
     *
     **/
    DataStream(const char* pbegin, const char* pend, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
    : vData((uint8_t*)pbegin, (uint8_t*)pend)
    , nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersionIn)
    {
    }
#endif


    /** DataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *
     **/
    DataStream(const std::vector<char>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
    : vData((uint8_t*)&vchDataIn.begin()[0], (uint8_t*)&vchDataIn.end()[0])
    , nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersionIn)
    {
    }


    /** SetType
     *
     *  Sets the type of stream.
     *
     *  @param[in] nSerTypeIn The serialize type to set.
     *
     **/
    void SetType(uint8_t nSerTypeIn)
    {
        nSerType = nSerTypeIn;
    }


    /** SetPos
     *
     *  Sets the position in the stream.
     *
     *  @param[in] nNewPos The position to set to in the stream.
     *
     **/
    void SetPos(uint32_t nNewPos) const
    {
        /* Check size constraints. */
        if(nNewPos > size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "cannot set at end of stream ", nNewPos));

        /* Set the new read pos. */
        nReadPos = nNewPos;
    }


    /** SetNull
     *
     *  Sets the object into null state.
     *
     **/
    void SetNull()
    {
        nReadPos = 0;
        vData.clear();
    }


    /** IsNull
     *
     *  Returns if object is in null state.
     *
     **/
    bool IsNull() const
    {
        return nReadPos == 0 && size() == 0;
    }


    /** Reset
     *
     *  Resets the internal read pointer.
     *
     **/
    void Reset() const
    {
        nReadPos = 0;
    }


    /** End
     *
     *  Returns if end of stream is found.
     *
     **/
    bool End() const
    {
        return nReadPos >= size();
    }


    /** read
     *
     *  Reads raw data from the stream.
     *
     *  @param[in] pch The pointer to beginning of memory to write.
     *  @param[in] nSize The total number of bytes to read.
     *
     *  @return Returns a reference to the DataStream object.
     *
     **/
    const DataStream& read(char* pch, uint32_t nSize) const
    {
        /* Check size constraints. */
        if(nReadPos + nSize > size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "reached end of stream ", nReadPos));

        /* Copy the bytes into tmp object. */
        std::copy((uint8_t*)&vData.at(nReadPos), (uint8_t*)&vData.at(nReadPos) + nSize, (uint8_t*)pch);

        /* Iterate the read position. */
        nReadPos += nSize;

        return *this;
    }


    /** Bytes
     *
     *  Get the data stream from the object.
     *
     **/
    const std::vector<uint8_t>& Bytes()
    {
        return vData;
    }


    /** reserve
     *
     *  Implement the same reserve functionality to vector.
     *
     **/
    void reserve(const uint32_t nSize)
    {
        vData.reserve(nSize);
    }


    /** begin
     *
     *  Wrapper around the vector iterator.
     *
     **/
    std::vector<uint8_t>::const_iterator begin() const
    {
        return vData.begin();
    }


    /** end
     *
     *  Wrapper around the vector iterator.
     *
     **/
    std::vector<uint8_t>::const_iterator end() const
    {
        return vData.end();
    }


    /** begin
     *
     *  Wrapper around the vector iterator.
     *
     **/
    std::vector<uint8_t>::iterator begin()
    {
        return vData.begin();
    }


    /** end
     *
     *  Wrapper around the vector iterator.
     *
     **/
    std::vector<uint8_t>::iterator end()
    {
        return vData.end();
    }


    /** data
     *
     *  Wrapper around data to get the start of vector.
     *
     **/
     uint8_t* data()
     {
         return vData.data();
     }


    /** clear
     *
     *  Wrapper around the vector clear.
     *
     **/
    void clear()
    {
        return vData.clear();
    }


    /** size
     *
     *  Get the size of the data stream.
     *
     **/
    size_t size() const
    {
        return vData.size();
    }


    /** write
     *
     *  Writes data into the stream.
     *
     *  @param[in] pch The pointer to beginning of memory to write.
     *  @param[in] nSize The total number of bytes to copy.
     *
     **/
    DataStream& write(const char* pch, uint32_t nSize)
    {
        /* Push the obj bytes into the vector. */
        vData.insert(vData.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

        return *this;
    }


    /** Operator Overload <<
     *
     *  Serializes data into vchOperations
     *
     *  @param[in] obj The object to serialize into ledger data.
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
     *  Serializes data into vchOperations.
     *
     *  @param[out] obj The object to de-serialize from ledger data.
     *
     **/
    template<typename Type> const DataStream& operator>>(Type& obj) const
    {
        /* Unserialize from the stream. */
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
    uint32_t nSerType;
    uint32_t nSerVersion;


    /** CAutoFile
     *
     *  Default construtor
     *
     *  @param[in] filenew The new file pointer to associate with
     *  @param[in] nTypeIn The serialize type
     *  @param[in] nVersionIn The serialize version
     *
     **/
    CAutoFile(FILE* filenew, uint32_t nTypeIn, uint32_t nVersionIn)
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
        if (file != nullptr && file != stdin && file != stdout && file != stderr)
            ::fclose(file);
        file = nullptr;
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
        file = nullptr;
        return ret;
    }


    /* operator overloads */
    operator FILE*()            { return file; }
    FILE* operator->()          { return file; }
    FILE& operator*()           { return *file; }
    FILE** operator&()          { return &file; }
    FILE* operator=(FILE* pnew) { return file = pnew; }
    bool operator!()            { return (file == nullptr); }


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

    void SetType(uint32_t n)          { nSerType = n; }
    uint32_t GetType()                { return nSerType; }
    void SetVersion(uint32_t n)       { nSerVersion = n; }
    uint32_t GetVersion()             { return nSerVersion; }
    void ReadVersion()           { *this >> nSerVersion; }
    void WriteVersion()          { *this << nSerVersion; }


    /** read
     *
     *  Reads from file into the raw data pointer.
     *
     *  @param[in] pch The pointer to beginning of memory to read into.
     *  @param[in] nSize The total number of bytes to read.
     *
     *  @return Reference to the CAutoFile.
     *
     **/
    CAutoFile& read(char* pch, size_t nSize)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::read : file handle is nullptr");
        if (fread(pch, 1, nSize, file) != nSize)
            setstate(std::ios::failbit, feof(file) ? "CAutoFile::read : end of file" : "CAutoFile::read : fread failed");
        return (*this);
    }


    /** write
     *
     *  Writes data into the file from the raw data pointer.
     *
     *  @param[in] pch The pointer to beginning of memory to write from.
     *  @param[in] nSize The total number of bytes to write.
     *
     *  @return Reference to the CAutoFile.
     *
     **/
    CAutoFile& write(const char* pch, size_t nSize)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::write : file handle is nullptr");
        if (fwrite(pch, 1, nSize, file) != nSize)
            setstate(std::ios::failbit, "CAutoFile::write : write failed");
        return (*this);
    }


    /** GetSerializeSize
     *
     *  The the size, in bytes, of the serialize object.
     *
     *  @param[in] obj A reference to the object to get the size of.
     *
     *  @return Number of bytes.
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
            throw std::ios_base::failure("CAutoFile::operator<< : file handle is nullptr");
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
            throw std::ios_base::failure("CAutoFile::operator>> : file handle is nullptr");
        ::Unserialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }
};

#endif
