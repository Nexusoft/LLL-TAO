/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_LOCATOR_H
#define NEXUS_LLP_INCLUDE_LOCATOR_H

#include <Util/templates/serialize.h>

//forward Declarations
namespace TAO
{
    namespace Ledger
    {
        class BlockState;
    }
}

/* Global Legacy namespace. */
namespace LLP
{

    /** Legacy object to handle legacy serialization. **/
    class Locator
    {
    public:

        /** The list of inventory one has. **/
        std::vector<uint1024_t> vHave;

        IMPLEMENT_SERIALIZE
        (
            READWRITE(vHave);
        )


        /** Default constructor. **/
        Locator()
        : vHave()
        {

        }

        /** Constructor
         *
         *  Set a locator from block state.
         *
         *  @param[in] state The block state to work from.
         *
         **/
        explicit Locator(const TAO::Ledger::BlockState& state);


        /** Constructor
         *
         *  Set a locator from block hash.
         *
         *  @param[in] hashBlock The block hash to work from.
         *
         **/
        explicit Locator(const uint1024_t& hashBlock);


        /** Constructor
         *
         *  Set a locator from list of hashes.
         *
         *  @param[in] vHaveIn The list of hashes to use.
         *
         **/
        Locator(const std::vector<uint1024_t>& vHaveIn)
        : vHave(vHaveIn)
        {
        }


        /** Destructor **/
        ~Locator()
        {
        }


        /** Set Null
         *
         *  Set the object to null.
         *
         **/
        void SetNull()
        {
            vHave.clear();
        }


        /** Is Null
         *
         *  Flag to determine if object is null.
         *
         *  @return true if object is in null state.
         *
         **/
        bool IsNull() const
        {
            return vHave.empty();
        }


        /** Set
         *
         *  Set a locator object from a block state.
         *
         *  @param[in] state The state to set object from.
         *
         **/
        void Set(const TAO::Ledger::BlockState& state);
    };
}

#endif
