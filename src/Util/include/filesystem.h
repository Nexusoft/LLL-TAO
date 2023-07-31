/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_FILESYSTEM_H
#define NEXUS_UTIL_INCLUDE_FILESYSTEM_H

#include <string>

#ifndef MAX_PATH
#ifdef WIN32
#define MAX_PATH 260
#else
#define MAX_PATH 4096
#endif
#endif

namespace filesystem
{

    /** size
     *
     *  Gets the size of the given file.
     *
     *  @param[in] strPath The path to check.
     *
     *  @return Returns the filesize from fstat command.
     *
     **/
    int64_t size(const std::string& strPath);


    /** remove_directories
     *
     *  Recursively remove directories along the path if they exist.
     *
     *  @param[in] strPath The path to remove.
     *
     *  @return Returns true if path was deleted, false otherwise.
     *
     **/
    bool remove_directories(const std::string& path);


    /** remove
     *
     *  Removes a file or folder from the specified path.
     *
     *  @param[in] strPath The path to remove.
     *
     *  @return Returns true if path was deleted, false otherwise.
     *
     **/
    bool remove(const std::string& strPath);


    /** rename
     *
     *  Renames a file or folder from the specified old path to a new path.
     *
     *  @param[in] strPathOld The path to rename.
     *  @param[in] strPathNew The renamed path.
     *
     *  @return Returns true if the file or folder was renamed, false otherwise.
     *
     **/
    bool rename(const std::string& strPathOld, const std::string& strPathNew);


    /** exists
     *
     *  Determines if the file or folder from the specified path exists.
     *
     *  @param[in] strPath The path to check
     *
     *  @return Returns true if the file or folder exists, false otherwise.
     *
     **/
    bool exists(const std::string& strPath);


    /** copy_file
     *
     *  Copies a data file. If the destination file exists, it is overwritten.
     *
     *  @param[in] strPathSource Fully qualified path name of source file.
     *  @param[in] strPathDest Fully qualified path name of destination file.
     *
     *  @return Returns true if the file was copied, false otherwise.
     *
     **/
    bool copy_file(const std::string& strPathSource, const std::string& strPathDest);


    /** set_permissions
     *
     *  Determines if the file or folder from the specified path exists.
     *
     *  @param[in] strPath The path to check.
     *
     *  @return Returns true if permissions were set, false otherwise.
     *
     **/
    bool set_permissions(const int fileMode);


    /** is_directory
     *
     *  Determines if the specified path is a folder.
     *
     *  @param[in] strPath The path to check.
     *
     *  @return Returns true if the path is a directory, false otherwise.
     *
     **/
    bool is_directory(const std::string& strPath);


    /** create_directories
     *
     *  Recursively create directories along the path if they don't exist.
     *
     *  @param[in] strPath The path to generate recursively.
     *
     *  @return Returns true if the directory was created, false otherwise.
     *
     **/
    bool create_directories(const std::string& strPath);


    /** create_directory
     *
     *  Create a single directory at the path if it doesn't already exist.
     *
     *  @param[in] strPath The path to create.
     *
     *  @return Returns true if the directory was created, false otherwise.
     *
     **/
    bool create_directory(const std::string& strPath);


    /** system_complete
     *
     *  Obtain the system complete path from a given relative path.
     *
     *  @param[in] strPath The path to append to.
     *
     *  @return Returns the system complete path from root directory.
     *
     **/
    std::string system_complete(const std::string& strPath);


    /** GetPidFile
    *
    *  Returns the full pathname of the PID file.
    *
    *  @return The complete path name of the PID file.
    *
    **/
    std::string GetPidFile();


    /** CreatePidFile
    *
    *  Creates a PID file on disk for the provided PID.
    *
    *  @param[in] strPath The path of the PID file.
    *  @param[in] pid The PID to create the PID file for.
    *
    **/
    void CreatePidFile(const std::string& strPath, pid_t pid);

}
#endif
