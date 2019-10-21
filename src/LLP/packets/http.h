/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_PACKETS_HTTP_H
#define NEXUS_LLP_PACKETS_HTTP_H


#include <Util/include/runtime.h>
#include <Util/include/debug.h>
#include <vector>
#include <map>

namespace LLP
{

    /** HTTPPacket
     *
     *  Class to handle sending and receiving of LLP Packets.
     *
     **/
    class HTTPPacket
    {
    public:

        /** Default Constructor **/
        HTTPPacket()
        : strType        ("")
        , strRequest     ("")
        , strVersion     ("")
        , mapHeaders     ( )
        , nContentLength (0)
        , strContent     ("")
        , fHeader        (false)
        {
        }


        /** Copy Constructor. **/
        HTTPPacket(const HTTPPacket& packet)
        : strType        (packet.strType)
        , strRequest     (packet.strRequest)
        , strVersion     (packet.strVersion)
        , mapHeaders     (packet.mapHeaders)
        , nContentLength (packet.nContentLength)
        , strContent     (packet.strContent)
        , fHeader        (packet.fHeader)
        {
        }


        /** Move Constructor. **/
        HTTPPacket(HTTPPacket&& packet) noexcept
        : strType        (std::move(packet.strType))
        , strRequest     (std::move(packet.strRequest))
        , strVersion     (std::move(packet.strVersion))
        , mapHeaders     (std::move(packet.mapHeaders))
        , nContentLength (std::move(packet.nContentLength))
        , strContent     (std::move(packet.strContent))
        , fHeader        (std::move(packet.fHeader))
        {
        }


        /** Copy assignment. **/
        HTTPPacket& operator=(const HTTPPacket& packet)
        {
            strType        = packet.strType;
            strRequest     = packet.strRequest;
            strVersion     = packet.strVersion;
            mapHeaders     = packet.mapHeaders;
            nContentLength = packet.nContentLength;
            strContent     = packet.strContent;
            fHeader        = packet.fHeader;

            return *this;
        }


        /** Move assignment. **/
        HTTPPacket& operator=(HTTPPacket&& packet) noexcept
        {
            strType        = std::move(packet.strType);
            strRequest     = std::move(packet.strRequest);
            strVersion     = std::move(packet.strVersion);
            mapHeaders     = std::move(packet.mapHeaders);
            nContentLength = std::move(packet.nContentLength);
            strContent     = std::move(packet.strContent);
            fHeader        = std::move(packet.fHeader);

            return *this;
        }


        /** Destructor. **/
        ~HTTPPacket()
        {
        }


        /** Constructor **/
        HTTPPacket(const uint32_t nStatus)
        : strType        ("")
        , strRequest     ("")
        , strVersion     ("")
        , mapHeaders     ( )
        , nContentLength (0)
        , strContent     ("")
        , fHeader        (false)
        {
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


        /** SetNull
         *
         *  Set the Packet Null Flags.
         *
         **/
        void SetNull()
        {
            strType = "";
            strRequest = "";
            strVersion = "";

            mapHeaders.clear();
            strContent = "";
            nContentLength = 0;

            fHeader = false;
        }


        /** IsNull
         *
         *  Packet Null Flag. Status of 0
         *
         **/
        bool IsNull() const
        {
            return strType == "" && strRequest == "" && strVersion == "" && mapHeaders.empty() && strContent == "" && !fHeader;
        }


        /** Complete
         *
         *  Determine if a packet is fully read.
         *
         **/
        bool Complete() const
        {
            if(strType == "GET" && fHeader)
                return true;

            return fHeader && (nContentLength == 0 || (nContentLength > 0 && nContentLength == strContent.size()));
        }


        /** SetStatus
         *
         *  Set the response status on a HTTP Reply.
         *
         *  @param[in] nStatus The status to set.
         *
         **/
        void SetStatus(uint32_t nStatus)
        {
            switch(nStatus)
            {
                case 200:
                    strType = "200 OK";
                    break;

                case 204:
                    strType = "204 No Content";
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

            /* Set connection header. */
            mapHeaders["Connection"] = "close";
        }


        /** GetBytes
         *
         *  Serializes class into a byte buffer. Used to write Packet to
         *  Sockets.
         *
         *  @return Returns a byte buffer.
         *
         **/
        std::vector<uint8_t> GetBytes() const
        {
            //TODO: use constant format (not ...) -> ostringstream
            std::string strReply = debug::safe_printstr
            (
                "HTTP/1.1 ", strType, "\r\n",
                "Date: ", debug::rfc1123Time(), "\r\n",
                "Server: Tritium HTTP\r\n"
            );

            /* Check for content. */
            if(strContent.size() > 0)
            {
                strReply += debug::safe_printstr
                (
                    "Content-Length: ", strContent.size(), "\r\n",
                    "Content-Type: application/json\r\n"
                );
            }

            /* Add custom header fields. */
            for(const auto& header : mapHeaders)
                strReply += debug::safe_printstr(header.first, ": ", header.second, "\r\n");;

            /* Add end of header and content. */
            strReply += debug::safe_printstr("\r\n", strContent);

            //get the bytes to submit over socket
            std::vector<uint8_t> vBytes(strReply.begin(), strReply.end());

            return vBytes;
        }
    };
}

#endif
