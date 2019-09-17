/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_STAKE_CHANGE_H
#define NEXUS_TAO_LEDGER_INCLUDE_STAKE_CHANGE_H

#include <Util/include/runtime.h>
#include <Util/templates/serialize.h>


/* Global TAO namespace. */
namespace TAO
{
    /* Ledger layer namespace. */
    namespace Ledger
    {

        /** StakeChange
         *
         *  Contains data requesting to add or remove stake from a trust account.
         *
         *  This data will be stored into the local database, with the change applied via the next
         *  stake block found by requesting user.
         *
         **/
        class StakeChange
        {
        public:

            /** Genesis hash of user requesting the change **/
            uint256_t hashGenesis;


            /** Amount of stake change. Positive implies add stake from trust account balance.
             *  Negative implies unstake to trust account balance.
             **/
            int64_t nAmount;


            /** Last stake tx for the user at the time the change was requested. If this changes, request becomes stale. **/
            uint512_t hashLast;


            /** Time when this stake change request was initiated. **/
            uint64_t nTime;


            /** Time when this stake change request expires. **/
            uint64_t nExpires;


            /** Indicates whether or not this stake change request has been processed. **/
            bool fProcessed;


            /** Transaction implementing this stake change request. **/
            uint512_t hashTx;


            //Object serialization for storage
            IMPLEMENT_SERIALIZE
            (
                READWRITE(hashGenesis);
                READWRITE(nAmount);
                READWRITE(hashLast);
                READWRITE(nTime);
                READWRITE(nExpires);
                READWRITE(fProcessed);
                READWRITE(hashTx);
            )


            /** Default Constructor
             *
             *  Initializes data
             *
             **/
            StakeChange()
            : hashGenesis(0)
            , nAmount(0)
            , hashLast(0)
            , nTime(runtime::unifiedtimestamp())
            , nExpires(0)
            , fProcessed(false)
            , hashTx(0)
            {
            }


            /** SetNull
             *
             *  Resets all data in this stake change request
             *
             **/
            void SetNull()
            {
                hashGenesis = 0;
                nAmount = 0;
                hashLast = 0;
                nTime = runtime::unifiedtimestamp();
                nExpires = 0;
                fProcessed = false;
                hashTx = 0;
            }

        };
    }
}

#endif
