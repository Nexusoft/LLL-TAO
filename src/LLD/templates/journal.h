/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
            
            (c) Copyright The Nexus Developers 2014 - 2018
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_JOURNAL_H
#define NEXUS_LLD_TEMPLATES_JOURNAL_H

#include "sector.h"
#include "core.h"

namespace LLD
{
    /* Journal Database keeps in non-volatile memory the transaction record for rollback.
    * This will be triggered if the checksums don't match with sector data and the keychain 
    * 
    * TODO: Figure out the best Key system to correlate a transaction to the data in the journal
    * This should be seen by the sector as well, which means that the keychain should keep a list
    * of the keys that were changed.
    * 
    * This should be done with a std::pair(key, txid) for the journal's key. 
    * Records should be resued,
    * 
    * TODO: Get the Sector Chain working properly
    */
    class CJournalDB : public SectorDatabase
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        CJournalDB(const char* pszMode="r+", std::string strID) : SectorDatabase(strID, strID, pszMode) {}
        
        /* TODO: Delete the Journal Database if it is deconstructed. 
        *       This should only happen when the database commits the transaction */
        ~CJournalDB()
        {
            
        }
        
        bool AddTransaction(uint512_t hash, Core::CTxIndex& txindex);
        bool RemoveTransaction(uint512_t hash, const Core::CTxIndex& txindex);
    };
}

#endif
