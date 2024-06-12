/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifdef WIN32 //TODO: use GetFullPathNameW in system_complete if getcwd not supported

#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <cstdio> //remove(), rename()
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>

#include <Util/include/debug.h>
#include <Util/include/filesystem.h>
#include <Util/include/mutex.h>

#include <sys/stat.h>

#ifdef WIN32
#include <shlwapi.h>
#include <direct.h>

/* Set up defs properly before including windows.h */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600    //minimum Windoze Vista version for winsock2, etc.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1  //prevents windows.h from including winsock.h and messing with winsock2.h definitions we use
#endif

#ifndef NOMINMAX
#define NOMINMAX //prevents windows.h from including min/max and potentially interfering with std::min/std::max
#endif

#include <windows.h>
#endif

namespace filesystem
{

    /* Open a file handle by current path. */
    file_t open_file(const std::string& strPath)
    {
        /* Check that the file exists. */
        if(!exists(strPath))
            return INVALID_HANDLE_VALUE;

    #ifdef _WIN32

        /* Create file with windoze API. */
        const file_t hFile = ::CreateFileA(strPath.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                0);
    #else // POSIX

        /* Create file with POSIX API. */
        const file_t hFile = ::open(strPath.c_str(), O_RDWR);
    #endif

        /* Check for failures. */
        if(hFile == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;

        return hFile;
    }


    /*  Determines the operating system's page allocation granularity.  */
    size_t page_size()
    {
        static const size_t nPageSize = []
        {
    #ifdef _WIN32
            SYSTEM_INFO SystemInfo;
            GetSystemInfo(&SystemInfo);
            return SystemInfo.dwAllocationGranularity;
    #else
            return sysconf(_SC_PAGE_SIZE);
    #endif
        }();
        return nPageSize;
    }

    /* Alligns `nOffset` to the operating's system page size */
    size_t make_offset_page_aligned(const size_t nOffset) noexcept
    {
        const size_t nPageSize = page_size();

        // Use integer division to round down to the nearest page alignment.
        return nOffset / nPageSize * nPageSize;
    }


    /* Get the filesize from the OS level API.*/
    int64_t query_file_size(const file_t hFile)
    {
    #ifdef _WIN32

        /* Query filesize with Windoze API. */
        LARGE_INTEGER nSize;
        if(::GetFileSizeEx(hFile, &nSize) == 0)
            return INVALID_HANDLE_VALUE;

    	return static_cast<int64_t>(nSize.QuadPart);

    #else // POSIX

        /* Query filesize with POSIX API. */
        struct stat sbuf;
        if(::fstat(hFile, &sbuf) == -1)
            return INVALID_HANDLE_VALUE;

        return sbuf.st_size;
    #endif
    }


    /* Get the size of a current file. */
    int64_t size(const std::string& strPath)
    {
        /* We are using the stream method to make sure it is portable. */
        std::fstream stream = std::fstream(strPath, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
        if(!stream.is_open())
            return -1;

        /* Get our filesize from stream position. */
        const uint64_t nFileSize = stream.tellp();

        /* Close the stream mnow. */
        stream.close();

        return nFileSize;
    }


    /* Removes a directory from the specified strPath. */
    bool remove_directories(const std::string& strPath)
    {
        if(!exists(strPath))
            return false;

    #ifdef WIN32
        /* Windows cannot do -rf type directory removal, have to delete all directory content manually first */
        std::string strFiles(strPath);
        strFiles.append("\\*");

        WIN32_FIND_DATA fileData;
        HANDLE hFind;
        bool fHasFile = true;

        hFind = FindFirstFile(strFiles.c_str(), &fileData);
        if(hFind == INVALID_HANDLE_VALUE)
        {
            if(GetLastError() == ERROR_FILE_NOT_FOUND)
                fHasFile = false;
            else
                return debug::error(FUNCTION, "Unable to remove directory: ", strPath, "\nReason: Invalid file handle");
        }

        while(fHasFile)
        {
            std::string fileName(fileData.cFileName);

            /* Don't process . or .. in the Windows file listing */
            if(!(fileName == "." || fileName == ".."))
            {
                if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                /* If content includes another directory, recursively remove both it and its contents */
                    if(!remove_directories(strPath + "\\" + fileName))
                        return false;
                }
                else
                {
                    std::string filePath(strPath + "\\" + fileName);

                    if(!DeleteFile(filePath.c_str()))
                        return debug::error(FUNCTION, "Unable to remove directory: ", strPath,
                                                      "\nReason: Unable to delete file", fileData.cFileName);
                }
            }

            if(!FindNextFile(hFind, &fileData))
            {
                if(GetLastError() == ERROR_NO_MORE_FILES)
                    fHasFile = false;
                else
                    return debug::error(FUNCTION, "Unable to remove directory: ", strPath, "\nReason: Error reading next file");
            }
        }

        FindClose(hFind);

        debug::log(2, FUNCTION, "Removing directory ", strPath);
        if(!RemoveDirectory(strPath.c_str()))
            return debug::error(FUNCTION, "Unable to remove directory: ", strPath);

        return true;

    #elif defined(MAC_OSX)
        debug::log(2, FUNCTION, "Removing directory ", strPath);
        if(system(debug::safe_printstr("sudo rm -rf '", strPath, "'").c_str()) == 0) //OSX requires sudo and special chars for strPath
            return true;

    #elif defined(IPHONE)
        return false;
    #elif defined(ANDROID)
        return false;
    #else
        debug::log(2, FUNCTION, "Removing directory ", strPath);
        if(system(debug::safe_printstr("rm -rf '", strPath, "'").c_str()) == 0)
            return true;

    #endif

        return false;
    }


    /* Removes a file or folder from the specified strPath. */
    bool remove(const std::string& strPath)
    {
        if(!exists(strPath))
            return false;

        if(std::remove(strPath.c_str()) != 0)
            return false;

        return true;
    }


    /*  Renames a file or folder from the specified old strPath to a new strPath. */
    bool rename(const std::string& strPathOld, const std::string& strPathNew)
    {
        if(!exists(strPathOld))
            return false;

        if(std::rename(strPathOld.c_str(), strPathNew.c_str()) != 0)
            return false;

        return true;
    }


    /* Determines if the file or folder from the specified strPath exists. */
    bool exists(const std::string& strPath)
    {
        struct stat statbuf;

        return stat(strPath.c_str(), &statbuf) == 0;
    }


    /* Copy a file. */
    bool copy_file(const std::string& strPathSource, const std::string& strPathDest)
    {
        try
        {
            /* Make sure destination is a file, not a directory. */
            if(exists(strPathDest) && is_directory(strPathDest))
                return false;

            /* If destination file exists, remove it (ie, we overwrite the file) */
            if(exists(strPathDest))
                filesystem::remove(strPathDest);

            /* Get the input stream of source file. */
            std::ifstream sourceFile(strPathSource, std::ios::binary);
            sourceFile.exceptions(std::ios::badbit);

            /* Get the output stream of destination file. */
            std::ofstream destFile(strPathDest, std::ios::binary);
            destFile.exceptions(std::ios::badbit);

            /* Copy the bytes. */
            destFile << sourceFile.rdbuf();

            /* Close the file handles and flush buffers. */
            sourceFile.close();
            destFile.close();

#ifndef WIN32
            /* Set destination file permissions (read/write by owner only for data files) */
            mode_t m = S_IRUSR | S_IWUSR;
            int file_des = open(strPathDest.c_str(), O_RDWR);

            if(file_des < 0)
                return false;

            fchmod(file_des, m);
#endif

        }
        catch(const std::ios_base::failure &e)
        {
            return debug::error(FUNCTION, " failed to write ", e.what());
        }

        return true;
    }

    /* Determines if the specified strPath is a folder. */
    bool is_directory(const std::string& strPath)
    {
        struct stat statbuf;

        if(stat(strPath.c_str(), &statbuf) != 0)
            return false;

        if(S_ISDIR(statbuf.st_mode))
            return true;

        return false;
    }


    /*  Recursively create directories along the strPath if they don't exist. */
    bool create_directories(const std::string& strPath)
    {
        /* Start loop at 1. Allows for root (Linux) or relative strPath (Windows) separator at 0 */
        for(uint32_t i = 1; i < strPath.length(); i++)
        {
            bool isSeparator = false;
            const char& currentChar = strPath.at(i);

        #ifdef WIN32
            /* For Windows, support both / and \ directory separators.
             * Ignore separator after drive designation, as in C:\
             */
            if((currentChar == '/' || currentChar == '\\')  && strPath.at(i-1) != ':')
                isSeparator = true;
        #else
            if(currentChar == '/')
                isSeparator = true;
        #endif

            if(isSeparator)
            {
                std::string createPath(strPath, 0, i);
                if(!create_directory(createPath))
                    return false;
            }
        }

        return true;
    }


    /* Create a single directory at the strPath if it doesn't already exist. */
    bool create_directory(const std::string& strPath)
    {
        if(exists(strPath)) //if the directory exists, don't attempt to create it
            return true;

        /* Set directory with read/write/search permissions for owner/group/other */
    #ifdef WIN32
        int status = _mkdir(strPath.c_str());
    #else
        mode_t m = S_IRWXU | S_IRWXG | S_IRWXO;
        int status = mkdir(strPath.c_str(), m);
    #endif

        /* Handle failures. */
        if(status < 0)
        {
            return debug::error(FUNCTION, "Failed to create directory: ", strPath, "\nReason: ", strerror(errno));
        }

        return true;
    }


    /* Obtain the system complete strPath from a given relative strPath. */
    std::string system_complete(const std::string& strPath)
    {
        char buffer[MAX_PATH] = {0};

        std::string fullPath;

    #ifdef WIN32

        /* Use Windows API for strPath generation. */
        if(!PathIsRelativeA(strPath.c_str()))
        {
            /* strPath is absolute */
            fullPath = strPath;
        }
        else
        {
            /* strPath is relative. */
            char *pTemp = nullptr;
            GetFullPathNameA(strPath.c_str(), MAX_PATH, buffer, &pTemp);
            fullPath = std::string(buffer);
        }

        if(fullPath.at(fullPath.length()-1) != '\\')
            fullPath += "\\";

    #else

        /* Non-Windows. If begins with / it is an absolute strPath, otherwise a relative strPath */
        if(strPath.at(0) == '/')
            fullPath = strPath;
        else
        {
            std::string currentDir(getcwd(buffer, MAX_PATH));
            fullPath = currentDir + "/" + strPath;
        }

        if(fullPath.at(fullPath.length()-1) != '/')
            fullPath += "/";

    #endif

        return fullPath;
    }


    /* Returns the full pathname of the PID file */
    std::string GetPidFile()
    {
        std::string pathPidFile(config::GetArg("-pid", "Nexus.pid"));
        return config::GetDataDir() + "/" +pathPidFile;
    }


    /* Creates a PID file on disk for the provided PID */
    void CreatePidFile(const std::string& strPath, pid_t pid)
    {
        FILE* file = fopen(strPath.c_str(), "w");
        if(file)
        {
        #ifndef WIN32
            fprintf(file, "%d", pid);
        #else
            /* For some reason, PRI64d fails with warning here because %I non-ANSI compliant,
               but it doesn't give this warning in other places except for config.cpp
               perhaps because this is fprintf (debug::log for example does not use printf).

               Consider re-writing this to use << operator

               If we just change to llu, then Linux gives warnings, so instead use a
               conditional compile and get warnings out of both */
            fprintf(file, "%llu", pid);
        #endif

            fclose(file);
        }
    }

}
