/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_FUNCTION_H
#define NEXUS_TAO_API_TYPES_FUNCTION_H

#include <Util/include/json.h>
#include <functional>
#include <memory>

namespace TAO::API
{

    /** Base class for all JSON based API methods
     *
     *  Encapsulates the function pointer to the method to process the API request
     *
     **/
    class Function
    {
        /** The function pointer to be called. */
        std::function<nlohmann::json(bool, nlohmann::json)> function;


        /** The state being enabled or not. **/
        bool fEnabled;


    public:
        /** Default Constructor. **/
        Function() { };


        /** Function input **/
        Function(std::function<nlohmann::json(bool, nlohmann::json)> functionIn) :
        function(functionIn),
        fEnabled(true)
        {
        }


        /** Execute
         *
         *  Executes the function pointer.
         *
         *  @param[in] fHelp Flag if help is invoked
         *  @param[in] params The json formatted parameters
         *
         *  @return The json formatted response.
         *
         **/
        nlohmann::json Execute(bool fHelp, nlohmann::json params)
        {
            if(!fEnabled)
                return nlohmann::json::object({"error", "method disabled"});

            return function(fHelp, params);
        }


        /** Disable
         *
         *  Disables the method from executing.
         *
         **/
        void Disable()
        {
            fEnabled = false;
        }
    };
}

#endif
