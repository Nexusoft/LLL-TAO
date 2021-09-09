/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

namespace TAO::Operation { class Contract; }
namespace TAO::Register  { class Object;   }

namespace TAO::API
{
    /** ExecuteContract
     *
     *  Checks a given string against value to find wildcard pattern matches.
     *
     *  @param[in] rContract The contract that we want to execute state for
     *
     *  @return the modified register's post-state
     *
     **/
    TAO::Register::Object ExecuteContract(const TAO::Operation::Contract& rContract);

}
