/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once


#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** Objects
         *
         *  Objects API Class.
         *  Manages the function pointers for all Objects API commands.
         *
         **/
        class Objects : public Base
        {
        public:

            /** Default Constructor. **/
            Objects() { Initialize(); }


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
                return "Objects";
            }



            /******************* STATIC HELPER METHODS BELOW THIS LINE **********************/

            /** Transfer
             *
             *  Generic method to transfer an object register .
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] nType The object type to transfer.
             *  @param[in] strType The name of the object type, used to format error messages
             *
             *  @return The return object in JSON.
             *
             **/
            static json::json Transfer(const json::json& params, uint8_t nType, const std::string& strType);


            /** Claim
             *
             *  Generic method to claim a transferred object regiseter .
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] nType The object type to claim
             *  @param[in] strType The name of the object type, used to format error messages
             *
             *  @return The return object in JSON.
             *
             **/
            static json::json Claim(const json::json& params, uint8_t nType, const std::string& strType);


            /** History
             *
             *  Generic method to output the history of an object register and its ownership
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] nType The object type to show history for
             *  @param[in] strType The name of the object type, used to format error messages
             *
             *  @return The return object in JSON.
             *
             **/
            static json::json History(const json::json& params, uint8_t nType, const std::string& strType);


            /** List
             *
             *  Generic method to list object registers by sig chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] nType The object type to list
             *
             *  @return The return object in JSON.
             *
             **/
            static json::json List(const json::json& params, uint8_t nType);


        };
    }
}
