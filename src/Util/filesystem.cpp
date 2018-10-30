/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/filesystem.h>

#ifdef WIN32 //GetFullPathNameW

#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <stdio.h> //remove()
#include <errno.h>
#include <string.h>

#define MAX_PATH 256

extern int errno;

namespace filesystem
{
    bool remove(const std::string &path)
    {
        if(exists(path) == false)
            return false;

        if(remove(path.c_str()) == 0)
            return true;

        return false;
    }

    bool exists(const std::string &path)
    {
        if(access(path.c_str(), F_OK) != -1)
            return true;

        return false;
    }

    bool is_directory(const std::string &path)
    {
        struct stat statbuf;
        if(stat(path.c_str(), &statbuf) != 0)
            return false;

        if(S_ISDIR(statbuf.st_mode))
            return true;

        return false;
    }

    bool create_directory(const std::string &path)
    {
        if(exists(path)) //if the directory exists, don't attempt to create it
            return true;

          /* set directory with read/write/search permissions for owner/group/other */
        mode_t m = S_IRWXU | S_IRWXG | S_IRWXO;
        int status = mkdir(path.c_str(), m);

        if(status < 0)
        {
            printf("Failed to create directory: %s\nReason: %s\n", path.c_str(), strerror(errno));
            return false;
        }
        return true;
    }

    bool create_directories(const std::string &path)
    {
        for(std::string::const_iterator it = path.begin(); it != path.end(); ++it)
        {
            if(*it == '/' && it != path.begin())
            {
                if(!create_directory(std::string(path.begin(), it)))
                    return false;
            }
        }
        return true;
    }

    std::string system_complete(const std::string &path)
    {
        char buffer[MAX_PATH] = {0};
        std::string abs_path;

          //get the path of the current directory and append path name to that
        abs_path = getcwd(buffer, MAX_PATH);
    #ifdef WIN32
        abs_path += "\\";
    #else
        abs_path += "/";
    #endif
        abs_path += path;

        return abs_path;
    }

}
