/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_FILESYSTEM_H
#define NEXUS_UTIL_INCLUDE_FILESYSTEM_H

#include <string>

namespace filesystem
{

    /** Remove
     *
     *  @param[in] path The path to remove.
     *
     *  @return true if path was deleted
     *
     **/
    bool remove(const std::string &path);


    /** Exists
     *
     *  @param[in] path The path to check
     *
     *  @return true if path exists
     *
     **/
    bool exists(const std::string &path);


    /** is_directory
     *
     *  @param[in] path The path to check
     *
     *  @return true if path is a directory
     *
     **/
    bool is_directory(const std::string &path);


    /** create_directories
     *
     *  @param[in] path The path to generate recrusively
     *
     *  @return true if path was created
     *
     **/
    bool create_directories(const std::string &path);


    /** create_directory
     *
     *  @param[in] path The path to create
     *
     *  @return true if path was created
     *
     **/
    bool create_directory(const std::string &path);


    /** system_complete
     *
     *  @param[in] path The path to append to
     *
     *  @return the system complete path from root directory
     *
     **/
    std::string system_complete(const std::string &path);
    
}
#endif
