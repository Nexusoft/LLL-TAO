/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_HTTP_H
#define NEXUS_LLP_INCLUDE_HTTP_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/http.h>
#include <LLP/templates/connection.h>

namespace LLP
{

    /** HTTP Node
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
     **/
    class HTTPNode : public BaseConnection<HTTPPacket>
    {
        /* Internal Read Buffer. */
        std::vector<int8_t> vchBuffer;


    protected:
        CAddress addrThisNode;


    public:

        /* Constructors for Message LLP Class. */
        HTTPNode() : BaseConnection<HTTPPacket>() {}
        HTTPNode( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false ) : BaseConnection<HTTPPacket>( SOCKET_IN, DDOS_IN ) { }


        /** Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type
         *  @param[in[ LENGTH The size of bytes read on packet read events
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) = 0;


        /** Main message handler once a packet is recieved. **/
        bool ProcessPacket() = 0;


        /** DoS
         *
         *  Send the DoS Score to DDOS Filte
         *
         *  @param[in] nDoS The score to add for DoS banning
         *  @param[in] fReturn The value to return (False disconnects this node)
         *
         */
        inline bool DoS(int nDoS, bool fReturn)
        {
            if(fDDOS)
                DDOS->rSCORE += nDoS;

            return fReturn;
        }


        /** Get the current IP address of this node. **/
        CAddress GetAddress() const
        {
            return addrThisNode;
        }


        /** Read Packet
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         */
        void ReadPacket()
        {
            if(!INCOMING.Complete())
            {
                /* Handle Reading Data into Buffer. */
                uint32_t nAvailable = SOCKET.Available();
                if(nAvailable > 0)
                {
                    std::vector<int8_t> vchData(nAvailable);
                    uint32_t nRead = SOCKET.Read(vchData, nAvailable);
                    if(nRead > 0)
                        vchBuffer.insert(vchBuffer.end(), vchData.begin(), vchData.begin() + nRead);
                }

                /* If waiting for buffer data, don't try to parse. */
                if(vchBuffer.size() == 0)
                    return;

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
                    std::transform(strLine.begin(), strLine.end(), strLine.begin(), ::tolower);

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
                         //TODO: check for spaces here.
                        if(strLine.substr(0, pos) == "content-length")
                            INCOMING.nContentLength = stoi(strLine.substr(pos + 2));

                        INCOMING.mapHeaders[strLine.substr(0, pos)] = strLine.substr(pos + 2);
                    }

                    /* Erase line read from the read buffer. */
                    vchBuffer.erase(vchBuffer.begin(), it + 1);
                }
            }
        }


        /** Push Response
         *
         *  Returns an HTTP packet with response code and content.
         *
         *  @param[in] nMsg The status code to respond with.
         *  @param[in] strContent The content to post return with.
         *
         **/
        void PushResponse(const uint16_t nMsg, std::string strContent)
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
