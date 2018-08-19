/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_TRANSACTION_H
#define NEXUS_LLD_TEMPLATES_TRANSACTION_H

#include <boost/thread.hpp>
#include <map>
#include <vector>

#include "../../LLC/hash/SK.h"
#include "../../Util/include/mutex.h"

//#define MUTEX_LOCK(a) boost::lock_guard<boost::mutex> lock(a)

namespace LLD
{
        /** ACID Transactions:
    
        Atonimicy - All or nothing. The full transaction muts complete. Transaction Recrod in Memory is held until commited to disk. If any part fails it needs
        To Roll Back to all the original data.
        I.   If any transactionf fails, roll back to original data
        II.  This can happen if it fails due to a power failure or system or application crash
        III. When rebooted the LLD sector database will check its instance for its original data.
        
        Consistency - The database must be brought from one valid state to the next. If any of the transaction sequences fail due to a violation of the rules
        of the database, the whole transaction must fail.
        I.  SectorTransaction class to handle the storing of all sequences of the transaction.
        II. If the commit fails, and the system is still operating, roll back all the transactiond data to original states.
        
        Isolation -All Transactions must execute in the sequence in which they were created in order to have the data changed in the order it was flagged
        To change.
        I. Use a Vector storing the order with a pair of byte vectors for the data type.
        II. Retain this ordering while commiting the transaction to disk.
        
        Durability - The data in a transacdtion must be commited to non volatile memory. This will make sure that the transaction will retain indpendent of a crash
        or power loss even in the middle of the transaction. This is done in three ways:
        I. Flagging of the keychain to set the state of the sector as part of a transaction sequence. 
        II. The data checksum is stored in the keychain that defines the state of the sector data to ensure that there was not a crash
        or power loss in the middle of the databse write. 
        II.Return of the sector keys to a valid state stating that the sectors were written successfully. This will only happen after the sector has been written.
        
        If there is a power loss before the transaction successfully commits, the original data is backed up in a sepearte temporary keychain/sector database 
        so that on reboot of the node or database, it checks the data for consistency. This will roll back to original data if any of the transaction sequences fail.
        
        Use a checksum as the transaction original key. If the data fails to read due to invalid state that was never reset, search the database for the original data.
        
    **/
    
    /** Transactino Class to hold the data that is stored in Binary. **/
    class SectorTransaction
    {
    public:
    
        /** The hash for the Transaction to be saved under. **/
        uint64_t TransactionID;
        
        /** Only let one operation happen on the transaction at one time. **/
        Mutex_t TX_MUTEX;
        
        /** New Data to be Added. **/
        std::map< std::vector<unsigned char>, std::vector<unsigned char> > mapTransactions;
        
        /** Original Data that is retained when new one is added. **/
        std::map< std::vector<unsigned char>, std::vector<unsigned char> > mapOriginalData;
        
        /** Vector to hold the keys of transactions to be erased. **/
        std::map< std::vector<unsigned char>, unsigned int > mapEraseData;
        
        /** Basic Constructor. **/
        SectorTransaction(){ }
        
        /** Add a new Transaction to the Memory Map. **/
        bool AddTransaction(std::vector<unsigned char> vKey, std::vector<unsigned char> vData,
                            std::vector<unsigned char> vOriginalData)
        {
            LOCK(TX_MUTEX);
            
            mapTransactions[vKey] = vData;
            mapOriginalData[vKey] = vOriginalData;
            
            return true;
        }
        
        /** Function to Erase a Key from the Keychain. **/
        bool EraseTransaction(std::vector<unsigned char> vKey)
        {
            LOCK(TX_MUTEX);
            
            mapEraseData[vKey] = 0;
            if(mapTransactions.count(vKey))
                mapTransactions.erase(vKey);
            
            if(mapOriginalData.count(vKey))
                mapOriginalData.erase(vKey);
            
            return true;
        }
        
        /* Hash to keep track of the Sector Transactions in the Transaction History Journal. */
        LLC::uint512 GetHash()
        {
            LLC::uint512 hashTransaction;
            
            return hashTransaction;
        }
    };
}

#endif
