/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_TEMPLATES_TRANSACTION_H
#define NEXUS_LLD_TEMPLATES_TRANSACTION_H

#include <LLC/types/uint1024.h>

#include <Util/templates/datastream.h>

#include <cstdint>
#include <map>
#include <vector>

namespace LLD
{
    /** SectorTransaction
     *
     *  Transaction Class to hold the data that is stored in Binary.
     *
     *  ACID Transactions:
     *
     *  Atonimicy - All or nothing. The full transaction muts complete. Transaction Record in Memory is held until commited to disk. If any part fails it needs
     *  To Roll Back to all the original data.
     *  I.   If any transaction fails, roll back to original data
     *  II.  This can happen if it fails due to a power failure or system or application crash
     *  III. When rebooted the LLD sector database will check its instance for its original data.
     *
     *  Consistency - The database must be brought from one valid state to the next. If any of the transaction sequences fail due to a violation of the rules
     *  of the database, the whole transaction must fail.
     *  I.  SectorTransaction class to handle the storing of all sequences of the transaction.
     *  II. If the commit fails, and the system is still operating, roll back all the transaction data to original states.
     *
     *  Isolation -All Transactions must execute in the sequence in which they were created in order to have the data changed in the order it was flagged
     *  To change.
     *  I. Use a Vector storing the order with a pair of byte vectors for the data type.
     *  II. Retain this ordering while committing the transaction to disk.
     *
     *  Durability - The data in a transaction must be commited to non volatile memory. This will make sure that the transaction will retain independent of a crash
     *  or power loss even in the middle of the transaction. This is done in three ways:
     *  I. Flagging of the keychain to set the state of the sector as part of a transaction sequence.
     *  II. The data checksum is stored in the keychain that defines the state of the sector data to ensure that there was not a crash
     *  or power loss in the middle of the database write.
     *  II.Return of the sector keys to a valid state stating that the sectors were written successfully. This will only happen after the sector has been written.
     *
     *  If there is a power loss before the transaction successfully commits, the original data is backed up in a separate temporary keychain/sector database
     *  so that on reboot of the node or database, it checks the data for consistency. This will roll back to original data if any of the transaction sequences fail.
     *
     *  Use a checksum as the transaction original key. If the data fails to read due to invalid state that was never reset, search the database for the original data.
     *
     **/
    class SectorTransaction
    {
    public:

        /** New Data to be Added. **/
        std::map< std::vector<uint8_t>, std::vector<uint8_t> > mapTransactions;

        /** Keychain items to commit. */
        std::set< std::vector<uint8_t> > setKeychain;

        /** Index items to commit. */
        std::map< std::vector<uint8_t>, std::vector<uint8_t> > mapIndex;

        /** Vector to hold the keys of transactions to be erased. **/
        std::set< std::vector<uint8_t> > setErasedData;

        /** Binary stream with data to be written. **/
        DataStream ssJournal;

        /** Default Constructor **/
        SectorTransaction();


        /** Default Destructor **/
        ~SectorTransaction();


        /** EraseTransaction
         *
         *  Function to Erase a Key from the Keychain.
         *
         *  @param[in] vKey The key in binary.
         *
         *  @return True if the erase was successful, false otherwise.
         *
         **/
        bool EraseTransaction(const std::vector<uint8_t> &vKey);
    };
}

#endif
