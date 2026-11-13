/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_DAEMON_H
#define NEXUS_UTIL_INCLUDE_DAEMON_H

#if !defined(WIN32) && !defined(QT_GUI) && !defined(NO_DAEMON)
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif


/** Daemonize
 *
 *  Daemonize by forking the parent process
 *
 **/
void Daemonize()
{
 #if !defined(WIN32) && !defined(QT_GUI) && !defined(NO_DAEMON)


    pid_t pid = fork();
    if(pid < 0)
    {
        debug::error(FUNCTION, "fork() returned ", pid, " errno ", errno);
        exit(EXIT_FAILURE);
    }
    if(pid > 0)
    {
        /* generate a pid file so that we can keep track of the forked process */
        filesystem::CreatePidFile(filesystem::GetPidFile(), pid);

        /* Success: Let the parent terminate */
        exit(EXIT_SUCCESS);
    }

    pid_t sid = setsid();
    if(sid < 0)
    {
        debug::error(FUNCTION, "setsid() returned ", sid, " errno ", errno);
        exit(EXIT_FAILURE);
    }

    debug::log(0, "Nexus server starting");

    /* Set new file permissions */
    umask(077);

    /* Redirect the standard streams away from the tty. */
    if(int fdnull = open("/dev/null", O_RDWR | O_CLOEXEC); fdnull >= 0)
    {
        for(int fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd)
        {
            if(fd == fdnull)
            {
                if(fcntl(fd, F_SETFD, 0) < 0)
                {
                    debug::error(FUNCTION, "Failed to clear FD_CLOEXEC for standard descriptor ", fd, " errno ", errno);
                    _exit(EXIT_FAILURE);
                }
            }
            else if(dup2(fdnull, fd) < 0)
            {
                debug::error(FUNCTION, "Failed to redirect standard descriptor ", fd, " errno ", errno);
                _exit(EXIT_FAILURE);
            }
        }
        if(fdnull > STDERR_FILENO)
            close(fdnull);
    }
    else
    {
        debug::error(FUNCTION, "Failed to open /dev/null");
        exit(EXIT_FAILURE);
    }
#endif
}

#endif
