/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/x509_cert.h>

#include <Util/include/debug.h>
#include <Util/include/config.h>

#include <openssl/ssl.h>
#include <openssl/err.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <ifaddrs.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>

void PrintCert(X509 *cert)
{
    if(cert != nullptr)
    {
        char *str = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        debug::log(0, "subject: ", str);
        OPENSSL_free(str);
        str = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        debug::log(0, "issuer: ", str);
        OPENSSL_free(str);
    }
}


void PeerCertificateInfo(SSL *ssl)
{
    X509 *peer_cert = SSL_get_peer_certificate(ssl);
    if(peer_cert != nullptr)
    {
        debug::log(0, "Peer certificate:");

        PrintCert(peer_cert);

        X509_free(peer_cert);
    }
    else
        debug::log(0, "Peer does not have certificate.");
}


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    config::ParseParameters(argc, argv);
    LLC::X509Cert certificate;

    certificate.Write();

    debug::log(0, "My certificate: ");
    certificate.Print();

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    SSL_CTX *ssl_ctx = SSL_CTX_new(SSLv23_method());
    SSL *ssl = SSL_new(ssl_ctx);

    certificate.Init_SSL(ssl);

    if(SSL_check_private_key(ssl) != 1)
        debug::error("Private key does not match the certificate public key.");

    int sd = -1;
    int err = 0;

    char buf[4096];

    if(config::GetBoolArg("-listen"))
    {
        int listen_sd = socket(AF_INET, SOCK_STREAM, 0);
        if(listen_sd == -1)
        debug::error("socket");

        struct sockaddr_in sa_serv;
        memset(&sa_serv, 0, sizeof(sa_serv));
        sa_serv.sin_family      = AF_INET;
        sa_serv.sin_addr.s_addr = INADDR_ANY;
        sa_serv.sin_port        = htons(1111);          /* Server Port number */

        err = bind(listen_sd, (struct sockaddr*)&sa_serv, sizeof(sa_serv));
        if(err == -1)
        debug::error("bind");

        /* Receive a TCP connection. */
        err = listen(listen_sd, 5);
        if(err == -1)
        debug::error("listen");

        struct sockaddr_in sa_cli;
        socklen_t client_len = sizeof(sa_cli);
        sd = accept(listen_sd, (struct sockaddr*) &sa_cli, &client_len);
        if(sd == -1)
            debug::error("accept");


        close(listen_sd);

        printf ("Connection from %x, port %u\n", sa_cli.sin_addr.s_addr, sa_cli.sin_port);

        SSL_set_fd (ssl, sd);
        err = SSL_accept(ssl);
        if(err == -1)
        {
            debug::error("SSL_accept: ", SSL_get_error(ssl, err));
            ERR_print_errors_fp(stderr);
        }

        debug::log(0, "SSL connection using ", SSL_get_cipher(ssl));

        //PeerCertificateInfo(ssl);

        err = recv(sd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
        if(err != -1)
            buf[err] = '\0';

        debug::log(0, "Got ", err, " chars: ", buf);

        err = SSL_read(ssl, buf, sizeof(buf) - 1);
        if(err == -1)
            debug::error("SSL_read");

        buf[err] = '\0';

        debug::log(0, "SSL: Got ", err, " chars: ", buf);

        err = SSL_write(ssl, "I hear you.", strlen("I hear you."));
        if(err == -1)
            debug::error("SSL_write");


    }
    else
    {

        sd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family      = AF_INET;
        sa.sin_addr.s_addr = inet_addr("127.0.0.1");   /* Server IP */
        sa.sin_port        = htons(1111);          /* Server Port number */

        err = connect(sd, (struct sockaddr*) &sa,  sizeof(sa));

        if(err == -1)
            debug::error("connect");

        SSL_set_fd(ssl, sd);
        err = SSL_connect(ssl);
        if(err == -1)
            debug::error("SSL_connect");

        debug::log(0, "SSL connection using ", SSL_get_cipher(ssl));

        PeerCertificateInfo(ssl);

        err = SSL_write(ssl, "Hello World!", strlen("Hello World!"));
        if(err == -1)
            debug::error("SSL_write");

        err = recv(sd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
        if(err != -1)
            buf[err] = '\0';
        debug::log(0, "Got ", err, " chars: ", buf);

        err = SSL_read(ssl, buf, sizeof(buf) - 1);
        buf[err] = '\0';

        debug::log(0, "SSL: Got ", err, " chars: ", buf);

    }

    SSL_shutdown(ssl); /* send SSL/TLS close_notify */

    close(sd);
    SSL_free(ssl);
    SSL_CTX_free(ssl_ctx);

    return 0;
}
