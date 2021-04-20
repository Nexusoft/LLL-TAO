/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <LLC/types/uint1024.h>

/* Forward declarations. */
namespace TAO::Register { class Address; }


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** ListRegisters
         *
         *  Scans a signature chain to work out all registers that it owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[out] vRegisters The list of register addresses from sigchain.
         *
         *  @return A vector of register addresses owned by the sig chain
         *
         **/
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters);


        /** ListObjects
         *
         *  Scans a signature chain to work out all non-standard object that it owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[out] vObjects The list of object register addresses from sigchain.
         *
         *  @return A vector of register addresses owned by the sig chain
         *
         **/
        bool ListObjects(const uint256_t& hashOwner, std::vector<TAO::Register::Address>& vObjects);


        /** ListAccounts
         *
         *  Scans a signature chain to work out all assets that it owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[in] fTokens If set then this will include tokens in the list
         *  @param[in] fTrust If set then this will include trust accounts in the list
         *  @param[out] vAccounts The list of account register addresses from sigchain.
         *
         *  @return A vector of register addresses owned by the sig chain
         *
         **/
        bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts,
                          bool fTokens, bool fTrust);

        /** ListPartial
         *
         *  Lists all object registers partially owned by way of tokens that the sig chain owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[out] vObjects The list of object register addresses from sigchain.
         *
         *  @return A vector of register addresses partially owned by the sig chain
         *
         **/
        bool ListPartial(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters);


        /** ListTokenizedObjects
         *
         *  Finds all objects that have been tokenized and therefore owned by hashToken
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[in] hashToken The token to find objects for
         *  @param[out] vObjects The list of object register addresses owned by the token.
         *
         *  @return A vector of register addresses owned by the token
         *
         **/
        bool ListTokenizedObjects(const uint256_t& hashGenesis, const TAO::Register::Address& hashToken,
                                  std::vector<TAO::Register::Address>& vObjects);

    }
}
