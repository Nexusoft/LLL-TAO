/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <TAO/Ledger/include/constants.h>


/* Global TAO namespace. */
namespace TAO::Register
{
    /** Maximum register sizes. **/
    const uint32_t MAX_REGISTER_SIZE = 1024; //1 KB


    /** Wildcard register address. **/
    const uint256_t WILDCARD_ADDRESS = ~uint256_t(0);


    /** Cost to create generic object registers **/
    const uint64_t OBJECT_FEE = 1 * TAO::Ledger::NXS_COIN;


    /** Cost to create a local or namesaced Name **/
    const uint64_t NAME_FEE = 1 * TAO::Ledger::NXS_COIN;


    /** Cost to create a global name **/
    const uint64_t GLOBAL_NAME_FEE = 2000 * TAO::Ledger::NXS_COIN;


    /** Cost to create a namespace **/
    const uint64_t NAMESPACE_FEE = 1000 * TAO::Ledger::NXS_COIN;


    /** Cost for token creation, calclated logarithmically starting at 100 **/
    const uint64_t TOKEN_FEE = 100 * TAO::Ledger::NXS_COIN;


    /** The minimum token fee for less than 100 token units **/
    const uint64_t MIN_TOKEN_FEE = 1 * TAO::Ledger::NXS_COIN;


    /** Cost to crete a NXS or token account **/
    const uint64_t ACCOUNT_FEE = 0 * TAO::Ledger::NXS_COIN;


    /** Cost to crete a crypto register **/
    const uint64_t CRYPTO_FEE = 1 * TAO::Ledger::NXS_COIN;


    /** The minimum cost to create a register **/
    const uint64_t MIN_DATA_FEE = 1 * TAO::Ledger::NXS_COIN;


    /* The cost per byte to create registers */
    const uint64_t DATA_FEE_V1 =  0.01 * TAO::Ledger::NXS_COIN;
    const uint64_t DATA_FEE    = 0.001 * TAO::Ledger::NXS_COIN;
}
