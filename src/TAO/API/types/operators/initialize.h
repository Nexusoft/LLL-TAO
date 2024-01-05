/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <map>

/* Global TAO namespace. */
namespace TAO::API
{
    class Operator;

    /** @namespace Operators
     *
     *  Namespace to hold all available operators for use in commands routes.
     *
     **/
    namespace Operators
    {

        /** Track what operators are currently supported by initialize. **/
        extern const std::map<std::string, Operator> mapSupported;


        /** Initialize
         *
         *  Initialize a set of operators using a set series of string arguments in CSV format.
         *
         *  @param[out] mapOperators The operators map to pass back to the host API command-set.
         *  @param[in] strOperators List of operators that will be supported by this command.
         *
         **/
        void Initialize(std::map<std::string, Operator> &mapOperators, const std::string& strOperators = "");

    }
}
