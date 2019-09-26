/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_TEMPLATES_AUTOFILE_H
#define NEXUS_UTIL_TEMPLATES_AUTOFILE_H

#include <Util/templates/serialize.h>
#include <cstdio>
#include <cstdint>
#include <ios>


/** AutoFile
 *
 *  RAII wrapper for FILE*. Will automatically close the file when it goes out of
 *  scope if not null.
 *  If you're returning the file pointer, return file.release().
 *  If you need to close the file early, use file.fclose() instead of fclose(file).
 *
 */
class AutoFile
{
protected:
    FILE* file;
    short state;
    short exceptmask;
public:
    uint32_t nSerType;
    uint32_t nSerVersion;


    /** AutoFile
     *
     *  Default construtor
     *
     *  @param[in] filenew The new file pointer to associate with
     *  @param[in] nTypeIn The serialize type
     *  @param[in] nVersionIn The serialize version
     *
     **/
    AutoFile(FILE* filenew, uint32_t nTypeIn, uint32_t nVersionIn)
    {
        file = filenew;
        nSerType = nTypeIn;
        nSerVersion = nVersionIn;
        state = 0;
        exceptmask = std::ios::badbit | std::ios::failbit;
    }


    /** ~AutoFile
     *
     *  Destructor, closes the file on destruction
     *
     **/
    ~AutoFile()
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
            throw std::runtime_error("setstate() : exception occured");
    }


    bool fail() const            { return state & (std::ios::badbit | std::ios::failbit); }
    bool good() const            { return state == 0; }
    void clear(short n = 0)      { state = n; }
    short exceptions()           { return exceptmask; }
    short exceptions(short mask) { short prev = exceptmask; exceptmask = mask; setstate(0, "AutoFile"); return prev; }

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
     *  @return Reference to the AutoFile.
     *
     **/
    AutoFile& read(char* pch, size_t nSize)
    {
        if (!file)
            throw std::ios_base::failure("AutoFile::read : file handle is nullptr");
        if (fread(pch, 1, nSize, file) != nSize)
            setstate(std::ios::failbit, feof(file) ? "AutoFile::read : end of file" : "AutoFile::read : fread failed");
        return (*this);
    }


    /** write
     *
     *  Writes data into the file from the raw data pointer.
     *
     *  @param[in] pch The pointer to beginning of memory to write from.
     *  @param[in] nSize The total number of bytes to write.
     *
     *  @return Reference to the AutoFile.
     *
     **/
    AutoFile& write(const char* pch, size_t nSize)
    {
        if (!file)
            throw std::ios_base::failure("AutoFile::write : file handle is nullptr");
        if (fwrite(pch, 1, nSize, file) != nSize)
            setstate(std::ios::failbit, "AutoFile::write : write failed");
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
    AutoFile& operator<<(const T& obj)
    {
        if (!file)
            throw std::ios_base::failure("AutoFile::operator<< : file handle is nullptr");
        ::Serialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }


    /** operator>>
     *
     *  Unserialize from this stream
     *
     **/
    template<typename T>
    AutoFile& operator>>(T& obj)
    {
        if (!file)
            throw std::ios_base::failure("AutoFile::operator>> : file handle is nullptr");
        ::Unserialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }
};

#endif
