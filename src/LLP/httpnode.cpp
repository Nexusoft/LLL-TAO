/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/httpnode.h>
#include <LLP/templates/ddos.h>

#include <Util/include/string.h>

#include <algorithm>

namespace LLP
{

    /** Default Constructor **/
    HTTPNode::HTTPNode()
    : BaseConnection<HTTPPacket> ( )
    , vchBuffer                  ( )
    , strOrigin                  ( )
    {
    }


    /** Constructor **/
    HTTPNode::HTTPNode(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<HTTPPacket> (SOCKET_IN, DDOS_IN, isDDOS)
    , vchBuffer                  ( )
    , strOrigin                  ( )
    {
    }


    /** Constructor **/
    HTTPNode::HTTPNode(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<HTTPPacket> (DDOS_IN, isDDOS)
    , vchBuffer                  ( )
    , strOrigin                  ( )
    {
    }


    /** Default Destructor **/
    HTTPNode::~HTTPNode()
    {
    }


    /* Send the DoS Score to DDOS Filter */
    bool HTTPNode::DoS(uint32_t nDoS, bool fReturn)
    {
        if(DDOS)
            DDOS->rSCORE += nDoS;

        return fReturn;
    }


    /*  Non-Blocking Packet reader to build a packet from TCP Connection. */
    void HTTPNode::ReadPacket()
    {
        if(!INCOMING.Complete())
        {
            /* Handle Reading Data into Buffer. */
            uint32_t nAvailable = Available();
            if(nAvailable > 0)
            {
                std::vector<int8_t> vchData(nAvailable);
                uint32_t nRead = Read(vchData, nAvailable);
                if(nRead > 0)
                    vchBuffer.insert(vchBuffer.end(), vchData.begin(), vchData.begin() + nRead);
            }

            /* If waiting for buffer data, don't try to parse. */
            if(vchBuffer.size() == 0)
                return;

            /* Allow up to 10 iterations to parse the header. */
            for(int i = 0; i < 10; ++i)
            {
                /* Read content if there is some. */
                if(INCOMING.fHeader)
                {
                    INCOMING.strContent += std::string(vchBuffer.begin(), vchBuffer.end());
                    vchBuffer.clear();
                    return;
                }

                /* Break out the lines by the input buffer. */
                auto it = std::find(vchBuffer.begin(), vchBuffer.end(), '\n');

                /* Return if a full line hasn't been read yet. */
                if(it == vchBuffer.end())
                    return;

                /* Check for the end of header with double CLRF. */
                if(vchBuffer.begin() == (it - 1))
                {
                    INCOMING.fHeader = true;

                    vchBuffer.erase(vchBuffer.begin(), it + 1); //erase the CLRF
                    //this->Event()
                    //TODO: assess the events code and calling virutal method from lower class in the inheritance heirarchy
                }

                /* Read all the headers. */
                else if(!INCOMING.fHeader)
                {
                    /* Extract the line from the buffer. */
                    std::string strLine = std::string(vchBuffer.begin(), it - 1);

                    /* Dump the header if requested on read. */
                    if(config::GetBoolArg("-httpheader"))
                        debug::log(0, strLine);

                    /* Find the delimiter to split. */
                    std::string::size_type pos = strLine.find(':', 0);

                    /* Handle the request types. */
                    if(INCOMING.strType == "")
                    {
                        /* Find the end of request type. */
                        std::string::size_type npos = strLine.find(' ', 0);
                        INCOMING.strType = strLine.substr(0, npos);

                        /* Find the start of version. */
                        std::string::size_type npos2 = strLine.find(' ', npos + 1);
                        INCOMING.strVersion = strLine.substr(npos2 + 1);

                        /* Parse request from between the two. */
                        INCOMING.strRequest = strLine.substr(npos + 1, npos2 - INCOMING.strType.length() - 1);

                    }

                    /* Handle normal headers. */
                    else if(pos != std::string::npos)
                    {
                        /* Set the field value to lowercase. */
                        std::string strField = ToLower(strLine.substr(0, pos));

                        /* Parse out the content length field. */
                        if(strField == "content-length")
                            INCOMING.nContentLength = std::stoul(strLine.substr(pos + 2));

                        /* Parse out origin. */
                        if(strField == "origin")
                            strOrigin = strLine.substr(pos + 2);

                        /* Add line to the headers map. */
                        INCOMING.mapHeaders[strField] = strLine.substr(pos + 2);

                    }

                    /* Erase line read from the read buffer. */
                    vchBuffer.erase(vchBuffer.begin(), it + 1);
                }
            }
        }
    }


    /* Returns an HTTP packet with response code and content. */
    void HTTPNode::PushResponse(const uint16_t nMsg, const std::string& strContent)
    {
        try
        {
            /* Build packet. */
            HTTPPacket RESPONSE(nMsg);

            /* Check for origin. */
            if(strOrigin != "")
                RESPONSE.mapHeaders["Origin"] = strOrigin;

            /* Add content. */
            RESPONSE.strContent = strContent;
            this->WritePacket(RESPONSE);
        }
        catch(...)
        {
            throw;
        }
    }

}
