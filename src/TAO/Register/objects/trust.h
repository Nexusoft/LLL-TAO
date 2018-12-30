/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_OBJECTS_ACCOUNT_H
#define NEXUS_TAO_REGISTER_OBJECTS_ACCOUNT_H

#include <cstdio>
#include <cstdint>

#include <Util/templates/serialize.h>

namespace TAO::Register
{

    /** Trust Object.
     *
     *  Holds the state of a trust account.
     *
     **/
    class Trust
    {
    public:

        /** The version of this account. **/
        uint32_t nVersion;


        /** The identifier of the account token. **/
        uint32_t nIdentifier;


        /** The trust account balance. **/
        uint64_t nBalance;


        /** Conditions required to be fulfilled. **/
        std::vector<uint8_t> vchConditions;


        /** The beneficiaries. **/
        std::vector<uint256_t> vBeneficiaries;


        /** Default Constructor. **/
        Trust()
        {
            SetNull();
        }

        //serialization functions
        IMPLEMENT_SERIALIZE
        (
            READWRITE(nVersion);
            READWRITE(nIdentifier);
            READWRITE(nBalance);
            READWRITE(vchConditions);
            READWRITE(vBeneficiaries);
        )


        /** Consturctor
         *
         *  @param[in] nIdentifierIn The identifier in
         *  @param[in] nBalanceIn The balance in
         *
         **/
        Trust(uint32_t nIdentifierIn, uint64_t nBalanceIn)
        : nVersion(1)
        , nIdentifier(nIdentifierIn)
        , nBalance(nBalanceIn)
        {

        }


        /** SetNull
         *
         *  Set this object to null state.
         *
         **/
        void SetNull()
        {
            nVersion       = 0;
            nIdentifier    = 0;
            nBalance       = 0;
        }


        /** IsNull
         *
         *  Checks if object is in null state
         *
         **/
        bool IsNull() const
        {
            return (nVersion == 0 && nIdentifier == 0 && nBalance == 0);
        }


        /** print
         *
         *  Output the state of this object.
         *
         **/
        void print() const
        {
            debug::log(0, "Trust(version=%u, id=%u, balance=%" PRIu64 ")", nVersion, nIdentifier, nBalance);
        }
    };
}

#endif
