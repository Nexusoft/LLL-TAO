/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once


#include <TAO/API/types/base.h>

#include <LLP/types/p2p.h>

#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** P2P
         *
         *  P2P API Class.
         *  Manages the function pointers for all P2P API commands.
         *
         **/
        class P2P : public Base
        {
        public:

            /** Default Constructor. **/
            P2P()
            : Base()
            {
                Initialize();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;



            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "P2P";
            }


            /** List
             *
             *  Returns a list of all active P2P connections for the logged in user including the number of outstanding messages
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json List(const json::json& params, bool fHelp);


            /** Get
             *
             *  Returns information about a single connection for the logged in user including the number of outstanding messages
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Get(const json::json& params, bool fHelp);


            /** ListRequests
             *
             *  Returns a list of all pending connection requests for the logged in user, either outgoing (you have made) 
             *  or incoming (made to you) 
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json ListRequests(const json::json& params, bool fHelp);


            /** Request
             *
             *  Makes a new outgoing connection request to a peer.  
             *  This method can optionally block (for up to 30s) waiting for an answer 
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Request(const json::json& params, bool fHelp);


            /** Accept
             *
             *  Accepts an incoming connection request from a peer and starts a messaging session
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Accept(const json::json& params, bool fHelp);


            /** Ignore
             *
             *  Removes an incoming connection request from the pending list
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Ignore(const json::json& params, bool fHelp);


            /** Terminate
             *
             *  Closes an existing P2P connection 
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Terminate(const json::json& params, bool fHelp);


            /** Send
             *
             *  Sends a data message to a connected peer
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Send(const json::json& params, bool fHelp);


            /** Peek
             *
             *  Returns the oldest message in the message queue from a peer without removing it from the queue.  
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Peek(const json::json& params, bool fHelp);


            /** Pop
             *
             *  Returns the oldest message in the message queue from a peer and removes it from the queue.  
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Pop(const json::json& params, bool fHelp);
            


        /* private helper methods */
        private:

            /** get_connection
             *
             *  Returns a connection for the specified app / genesis / peer.
             *
             *  @param[in] strAppID The app ID to check for
             *  @param[in] hashGenesis The genesis hash to search for
             *  @param[in] hashGenesis The peer genesis hash to search for 
             *  @param[out] connection The connection pointer to set
             *
             *  @return true if the connection was found.
             *
             **/
            bool get_connection(const std::string& strAppID, 
                                const uint256_t& hashGenesis, 
                                const uint256_t& hashPeer,
                                std::shared_ptr<LLP::P2PNode> &connection);
        };
    }
}
