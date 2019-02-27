/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_CMD_H
#define NEXUS_TAO_API_INCLUDE_CMD_H

namespace TAO
{
    namespace API
    {
        /** CommandLineAPI
         *
         *  Executes an API call from the commandline.
         *
         *  @param[in] argc The total input arguments.
         *  @param[in] argv The argument characters.
         *
         **/
        int CommandLineAPI(int argc, char** argv, int argn);


        /** CommandLineRPC
         *
         *  Executes an RPC call from the commandline.
         *
         *  @param[in] argc The total input arguments.
         *  @param[in] argv The argument characters.
         *
         **/
        int CommandLineRPC(int argc, char** argv, int argn);
    }

}

#endif
