/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/x509_cert.h>

#include <LLP/include/network.h>

#include <Util/include/debug.h>

#include <openssl/ssl.h>

namespace LLP
{

    /* The global SSL Context for the LLP */
    SSL_CTX *pSSL_CTX = nullptr;

    /* Perform any necessary processing to initialize the underlying network
     * resources such as sockets, etc.
     */
    bool NetworkInitialize()
    {

    #ifdef WIN32
        /* Initialize Windows Sockets */
        WSADATA wsaData;
        int32_t ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if(ret != NO_ERROR)
        {
            debug::error(FUNCTION, "TCP/IP socket library failed to start (WSAStartup returned error ", ret, ") ");
            return false;
        }
        else if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
        {
            /* Winsock version incorrect */
            debug::error(FUNCTION, "Windows sockets does not support requested version 2.2");
            WSACleanup();
            return false;
        }

        debug::log(3, FUNCTION, "Windows sockets initialized for Winsock version 2.2");

    #else
    {
        struct rlimit lim;
        lim.rlim_cur = 4096;
        lim.rlim_max = 4096;
        if(setrlimit(RLIMIT_NOFILE, &lim) == -1)
            debug::error(FUNCTION, "Failed to set max file descriptors");
    }

    {
        struct rlimit lim;
        getrlimit(RLIMIT_NOFILE, &lim);

        debug::log(2, FUNCTION "File descriptor limit set to ", lim.rlim_cur, " and maximum ", lim.rlim_max);
    }

    #endif



        /* OpenSSL initialization. */
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();

        /* Create the global network SSL object. */
        pSSL_CTX = SSL_CTX_new(SSLv23_method());

        /* Set the verification callback to always true. */
        SSL_CTX_set_verify(pSSL_CTX, SSL_VERIFY_PEER, LLC::always_true_callback);

        /* Instantiate a certificate for use with SSL context */
        LLC::X509Cert cert;

        cert.Generate();
        cert.Verify();

        if(!cert.Init_SSL(pSSL_CTX))
            return debug::error(FUNCTION, "Certificate Init Failed for SSL Context");
        if(!cert.Verify(pSSL_CTX))
            return debug::error(FUNCTION, "Certificate Verify Failed for SSL Context");


        debug::log(3, FUNCTION, "SSL context and certificate creation complete.");
        debug::log(2, FUNCTION, "Network resource initialization complete");

        return true;
    }


    /* Perform any necessary processing to shutdown and release underlying network resources.*/
    bool NetworkShutdown()
    {

    #ifdef WIN32
        /* Clean up Windows Sockets */
        int32_t ret = WSACleanup();

        if(ret != NO_ERROR)
        {
            debug::error("Windows socket cleanup failed (WSACleanup returned error ", ret, ") ");
            return false;
        }

    #endif

        /* Free the SSL context. */
        SSL_CTX_free(pSSL_CTX);

        debug::log(2, FUNCTION, "Network resource cleanup complete");

        return true;
    }


    /* Asynchronously invokes the lispers.net API to cache the EIDs and RLOCs used by this node
    *  and caches them for future use */
    void CacheEIDsAndRLOCs()
    {

    }

}
