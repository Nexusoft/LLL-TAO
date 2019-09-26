/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/transaction.h>
#include <LLD/include/version.h>

namespace LLD
{

    /* Default Constructor */
    SectorTransaction::SectorTransaction()
    : mapTransactions()
    , setKeychain()
    , mapIndex()
    , setErasedData()
    , ssJournal(SER_LLD, DATABASE_VERSION)
    {
    }


    /* Default Destructor */
    SectorTransaction::~SectorTransaction()
    {
    }


    /*  Function to Erase a Key from the Keychain. */
    bool SectorTransaction::EraseTransaction(const std::vector<uint8_t> &vKey)
    {
        /* Add the erased data to the map. */
        setErasedData.insert(vKey);

        /* Delete from transactions map if exists. */
        if(mapTransactions.count(vKey))
            mapTransactions.erase(vKey);

        /* Delete from keychain if exists. */
        if(setKeychain.count(vKey))
            setKeychain.erase(vKey);

        /* Delete from indexes if exists. */
        if(mapIndex.count(vKey))
            mapIndex.erase(vKey);

        return true;
    }

}
