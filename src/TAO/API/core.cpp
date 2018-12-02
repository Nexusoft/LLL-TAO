/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/core.h>

namespace TAO::API
{
    std::map<std::string, std::map<std::string, std::function<nlohmann::json(bool, nlohmann::json)> > > mapFunctions;

    /* Custom Events for Core API */
    void CoreNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        //no events for now
        //TODO: see if at all possible to call from down in inheritance heirarchy
    }


    /** Main message handler once a packet is recieved. **/
    bool CoreNode::ProcessPacket()
    {
        /* Parse the packet request. */
        std::string::size_type npos = INCOMING.strRequest.find('/', 1);

        /* Extract the API requested. */
        std::string API = INCOMING.strRequest.substr(1, npos - 1);

        /* Extract the method to invoke. */
        std::string METHOD = INCOMING.strRequest.substr(npos + 1);


        nlohmann::json ret;

        nlohmann::json parameters;// = nlohmann::json::parse(INCOMING.strContent);
        if(mapFunctions.count(API))
        {

            if(mapFunctions[API].count(METHOD))
            {
                ret = mapFunctions[API][METHOD](false, parameters); //TODO: add help support as param[0]
            }
            else
            {
                ret = { {"result", ""}, {"errors","method not found"} };
            }
        }
        else
        {
            ret = { {"result", ""}, {"errors","API not found"} };
        }


        PushResponse(200, ret.dump(4));

        /* Handle a connection close header. */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "close")
            return false;

        return true;
    }

}
