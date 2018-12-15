/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_PACKETS_HTTP_H
#define NEXUS_LLP_PACKETS_HTTP_H


#include <Util/include/runtime.h>
#include <Util/include/debug.h>
#include <vector>
#include <map>

namespace LLP
{

    /* Class to handle sending and receiving of LLP Packets. */
    class HTTPPacket
    {
    public:
        HTTPPacket() { SetNull(); }
        HTTPPacket(uint32_t nStatus)
        {
            SetNull();
            SetStatus(nStatus);
        }


        /* HTTP Request Type. */
        std::string strType;


        /* HTTP Request URL. */
        std::string strRequest;


        /* HTTP Request Version. */
        std::string strVersion; //HTTP version


        /* HTTP Status Headers. */
        std::map<std::string, std::string> mapHeaders;


        /* The content length. */
        uint32_t nContentLength;


        /* HTTP Body or Post content. */
        std::string strContent;


        /* Flag for knowing when reading HTML. */
        bool fHeader;


        /* Set the Packet Null Flags. */
        void SetNull()
        {
            strType = "";
            strRequest = "";
            strVersion = "";

            mapHeaders.clear();
            strContent = "";
            nContentLength = -1; // Set this to -1 initially so that we can determine when it has been set to a valid value, even if that is 0

            fHeader = false;
        }


        /* Packet Null Flag. Status of 0 */
        bool IsNull()
        {
            return strType == "" && strRequest == "" && strVersion == "" && mapHeaders.empty() && strContent == "" && !fHeader;
        }


        /* Determine if a packet is fully read. */
        bool Complete()
        {
            if(strType == "GET" && fHeader)
                return true;

            return fHeader && (nContentLength == 0 || nContentLength > 0 && nContentLength == strContent.size());
        }


        /* Set the response status on a HTTP Reply. */
        void SetStatus(uint32_t nStatus)
        {
            switch(nStatus)
            {
                case 200:
                    strType = "200 OK";
                    break;

                case 400:
                    strType = "400 Bad Request";
                    break;

                case 401:
                    strType = "401 Authorization Required";
                    break;

                case 403:
                    strType = "403 Forbidden";
                    break;

                case 404:
                    strType = "404 Not Found";
                    break;

                case 500:
                    strType = "500 Internal Server Error";
                    break;
            }
        }

        /* Serializes class into a byte buffer. Used to write Packet to Sockets. */
        std::vector<uint8_t> GetBytes()
        {
            //TODO: use constant format (not ...) -> ostringstream
            //TODO: add headers map to build more complex response rather than const as follows
            std::string strReply = debug::strprintf(
                    "HTTP/1.1 %s\r\n"
                    "Date: %s\r\n"
                    "Connection: close\r\n"
                    "Content-Length: %d\r\n"
                    "Content-Type: application/json\r\n"
                    "Server: Nexus-JSON-RPC\r\n"
                    "\r\n"
                    "%s",
                strType.c_str(),
                runtime::rfc1123Time().c_str(),
                strContent.size(),
                strContent.c_str());

            //get the bytes to submit over socket
            std::vector<uint8_t> vBytes(strReply.begin(), strReply.end());

            return vBytes;
        }
    };
}

#endif
