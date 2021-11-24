/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <string>

/* Global TAO namespace. */
namespace TAO::API
{

    /** VariableToJSON
     *
     *  Converts a query variable into a string.
     *
     *  Varibles needs to be modular functional statements with return type specifications.
     *  This function is hard coded variables for now, need to make it modular.
     */
    __attribute__((const)) std::string VariableToJSON(const std::string& strValue);
