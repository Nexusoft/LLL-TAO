/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_HTTP_H
#define NEXUS_LLP_TYPES_HTTP_H

#include <algorithm>

#include <LLP/include/legacyaddress.h>
#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/http.h>
#include <LLP/templates/connection.h>
#include <Util/include/string.h>

#define HTTPNODE ANSI_COLOR_FUNCTION "HTTPNode" ANSI_COLOR_RESET " : "

namespace LLP
{

    /** HTTPNode
     *
     *  A node that can speak over HTTP protocols.
     *
     *  This overrides process packet as null virtual function to require one more level
     *  of inheritance. This is to allow different interpretations of standard HTTP header.
     *
     *  Such examples would be:
     *  HTTP-JSON-API - Nexus Contracts API
     *  POST <command> HTTP/1.1
     *  {"params":[]}
     *
     *  HTTP-JSON-RPC - Ledger Level API
     *  POST / HTTP/1.1
     *  {"method":"", "params":[]}
     *
     *  This could also be used as the base for a HTTP-LLP server implementation.
     *
     **/
    class HTTPNode : public BaseConnection<HTTPPacket>
    {
        /* Internal Read Buffer. */
        std::vector<int8_t> vchBuffer;


    protected:

        /* The local address of this node. */
        LegacyAddress addrThisNode;


    public:

        /** Default Constructor **/
        HTTPNode()
        : BaseConnection<HTTPPacket>() { }

        /** Constructor **/
        HTTPNode( Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : BaseConnection<HTTPPacket>( SOCKET_IN, DDOS_IN ) { }


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) override = 0;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() override = 0;


        /** DoS
         *
         *  Send the DoS Score to DDOS Filte
         *
         *  @param[in] nDoS The score to add for DoS banning
         *  @param[in] fReturn The value to return (False disconnects this node)
         *
         **/
        inline bool DoS(int nDoS, bool fReturn)
        {
            if(fDDOS)
                DDOS->rSCORE += nDoS;

            return fReturn;
        }


        /** GetAddress
         *
         *  Get the current IP address of this node.
         *
         *  @return The address of this node
         *
         **/
        LegacyAddress GetAddress() const
        {
            return addrThisNode;
        }


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket() final
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
                for(int i = 0; i < 10; i++)
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
                            std::string field = ToLower(strLine.substr(0, pos));

                            /* Parse out the content length field. */
                            if(field == "content-length")
                                INCOMING.nContentLength = stoi(strLine.substr(pos + 2));

                            /* Add line to the headers map. */
                            INCOMING.mapHeaders[field] = strLine.substr(pos + 2);

                        }

                        /* Erase line read from the read buffer. */
                        vchBuffer.erase(vchBuffer.begin(), it + 1);
                    }
                }
            }
        }


        /** PushResponse
         *
         *  Returns an HTTP packet with response code and content.
         *
         *  @param[in] nMsg The status code to respond with.
         *
         *  @param[in] strContent The content to post return with.
         *
         **/
        void PushResponse(const uint16_t nMsg, const std::string& strContent)
        {
            try
            {
                HTTPPacket RESPONSE(nMsg);
                RESPONSE.strContent = strContent;

                this->WritePacket(RESPONSE);
            }
            catch(...)
            {
                throw;
            }
        }

    };

}

#endif
