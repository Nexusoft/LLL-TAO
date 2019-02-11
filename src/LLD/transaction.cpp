/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/transaction.h>

namespace LLD
{

    /** Default Constructor **/
    SectorTransaction::SectorTransaction()
    {
    }


    /** Default Destructor **/
    SectorTransaction::~SectorTransaction()
    {
    }


    /*  Function to Erase a Key from the Keychain. */
    bool SectorTransaction::EraseTransaction(const std::vector<uint8_t> &vKey)
    {
        mapEraseData[vKey] = 0;
        if(mapTransactions.count(vKey))
            mapTransactions.erase(vKey);

        if(mapOriginalData.count(vKey))
            mapOriginalData.erase(vKey);

        return true;
    }


    uint512_t SectorTransaction::GetHash() const
    {
        uint512_t hashTransaction;

        return hashTransaction;
    }

}
