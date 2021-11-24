/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

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
     *  Execute a given contract giving the post-state object register.
     *
     *  @param[in] rContract The contract that we want to execute state for
     *
     *  @return the modified register's post-state
     *
     **/
    TAO::Register::Object ExecuteContract(const TAO::Operation::Contract& rContract);

}
