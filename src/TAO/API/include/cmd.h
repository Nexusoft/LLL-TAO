/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_INCLUDE_CMD_H
#define NEXUS_TAO_API_INCLUDE_CMD_H

namespace TAO
{
    namespace API
    {
        /** CommandLine
         *
         *  Executes an API call from the commandline
         *
         *  @param[in] argc The total input arguments.
         *  @param[in] argv The argument characters.
         *
         **/
        int CommandLine(int argc, char** argv, int argn);
    }

}

#endif
