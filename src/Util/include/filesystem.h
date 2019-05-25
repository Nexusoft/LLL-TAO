/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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

    /** remove_directories
     *
     *  Recursively remove directories along the path if they exist.
     *
     *  @param[in] path The path to remove.
     *
     *  @return Returns true if path was deleted, false otherwise.
     *
     **/
    bool remove_directories(const std::string& path);


    /** remove
     *
     *  Removes a file or folder from the specified path.
     *
     *  @param[in] path The path to remove.
     *
     *  @return Returns true if path was deleted, false otherwise.
     *
     **/
    bool remove(const std::string &path);


    /** rename
     * 
     *  Renames a file or folder from the specified old path to a new path.
     * 
     *  @param[in] pathOld The path to rename. 
     *  @param[in] pathNew The renamed path.
     * 
     *  @return Returns true if the file or folder was renamed, false otherwise.
     * 
     **/
    bool rename(const std::string &pathOld, const std::string &pathNew); 


    /** exists
     *
     *  Determines if the file or folder from the specified path exists.
     *
     *  @param[in] path The path to check
     *
     *  @return Returns true if the file or folder exists, false otherwise.
     *
     **/
    bool exists(const std::string &path);


    /** copy_file
     *
     *  Copies a data file. If the destination file exists, it is overwritten.
     *
     *  @param[in] pathSource Fully qualified path name of source file.
     *  @param[in] pathDest Fully qualified path name of destination file.
     *
     *  @return Returns true if the file was copied, false otherwise.
     *
     **/
    bool copy_file(const std::string &pathSource, const std::string &pathDest);


    /** set_permissions
     *
     *  Determines if the file or folder from the specified path exists.
     *
     *  @param[in] path The path to check.
     *
     *  @return Returns true if permissions were set, false otherwise.
     *
     **/
    bool set_permissions(const int fileMode);


    /** is_directory
     *
     *  Determines if the specified path is a folder.
     *
     *  @param[in] path The path to check.
     *
     *  @return Returns true if the path is a directory, false otherwise.
     *
     **/
    bool is_directory(const std::string &path);


    /** create_directories
     *
     *  Recursively create directories along the path if they don't exist.
     *
     *  @param[in] path The path to generate recrusively.
     *
     *  @return Returns true if the directory was created, false otherwise.
     *
     **/
    bool create_directories(const std::string &path);


    /** create_directory
     *
     *  Create a single directory at the path if it doesn't already exist.
     *
     *  @param[in] path The path to create.
     *
     *  @return Returns true if the directory was created, false otherwise.
     *
     **/
    bool create_directory(const std::string &path);


    /** system_complete
     *
     *  Obtain the system complete path from a given relative path.
     *
     *  @param[in] path The path to append to.
     *
     *  @return Returns the system complete path from root directory.
     *
     **/
    std::string system_complete(const std::string &path);


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
    *  @param[in] path The path of the PID file.
    *  @param[in] pid The PID to create the PID file for.
    *
    **/
    void CreatePidFile(const std::string &path, pid_t pid);

}
#endif
