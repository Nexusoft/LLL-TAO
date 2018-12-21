/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_WALLET_ADDRESSBOOK_H
#define NEXUS_TAO_LEGACY_WALLET_ADDRESSBOOK_H

#include <map>
#include <string>

namespace Legacy
{
    
    /* forward declarations */    
    class NexusAddress;
    class CWallet;


    /** AddressBookMap is type alias defining a map for storing address book labels by Nexus address. **/
    using AddressBookMap = std::map<NexusAddress, std::string>;


    /** @class CAddressBook
     *
     *  Implements an address book mapping Nexus addresses (accounts) to address labels.
     * 
     *  For a file backed wallet, adding or removing address book entries will also update
     *  the wallet database. 
     *
     *  Database key for address book entries is name<address> where address is string representation of Nexus address.
     **/
    class CAddressBook
    {
        friend class CWalletDB;
        
    private:
        /** Map of address book labels for this address book **/
        AddressBookMap mapAddressBook;


        /** The wallet containing this address book **/
        CWallet& addressBookWallet;


        /** LoadAddressBookName
         *
         *  Loads an address book entry without updating the database (for file backed wallet). 
         *  For use by LoadWallet.
         *
         *  @param[in] address The Nexus address of the label to store
         *
         *  @param[in] strName The address label to store
         *
         *  @see CWalletDB::LoadWallet
         *
         **/
        inline void LoadAddressBookName(const NexusAddress& address, const std::string& strName) { mapAddressBook[address] = strName; }


    public:
        /** Constructor
         *
         *  Initializes an address book associated with a given wallet.
         *
         *  @param[in] walletIn The wallet containing this address book
         *
         **/
        CAddressBook(CWallet& walletIn) : addressBookWallet(walletIn) {}


        /** HasAddress
         *
         *  Checks whether this address book has an entry for a given address.
         *
         *  @param[in] address The Nexus address to check
         *
         *  @return true if address is in the address book
         *
         **/
        inline bool HasAddress(const NexusAddress& address) { return mapAddressBook.count(address) > 0; }


        /** GetAddressBookName
         *
         *  Retrieves the name (label) associated with an address.
         *
         *  @param[in] address The Nexus address of the label to retrieve
         *
         *  @return the address label
         *
         **/
        inline std::string GetAddressBookName(const NexusAddress& address) { return mapAddressBook[address]; }


        /** SetAddressBookName
         *
         *  Adds an address book entry for a given Nexus address and address label.
         *
         *  @param[in] address The Nexus address of the label to store
         *
         *  @param[in] strName The address label to store
         *
         *  @return true if the address book entry was successfully added
         *
         **/
        bool SetAddressBookName(const NexusAddress& address, const std::string& strName);


        /** DelAddressBookName
         *
         *  Removes the address book entry for a given Nexus address.
         *
         *  @param[in] address The Nexus address to remove
         *
         *  @return true if the address book entry was successfully removed
         *
         **/
        bool DelAddressBookName(const NexusAddress& address);


        /** AvailableAddresses
         *
         *  Get Nexus addresses that have a balance associated with the wallet for this address book. 
         *
         *  @param[in] nSpendTime Target time for balance totals. Any transaction with a later timestamp is filtered from balance calculation
         *
         *  @param[out] mapAddressBalances Collection of Nexus addresses with their available balance
         *
         *  @param[in] fOnlyConfirmed true only includes confirmed balance. Any unconfirmed transaction is filtered from balance calculation
         *
         *  @param[in] nMinDepth Mimimum depth required for transaction balances to be included
         *
         *  @return true if addresses retrieved successfully
         *
         **/
        bool AvailableAddresses(const uint32_t nSpendTime, std::map<NexusAddress, int64_t>& mapAddressBalances, 
                                const bool fOnlyConfirmed = false, const uint32_t nMinDepth = 1) const;


        /** BalanceByAccount
         *
         *  Get the current balance for a given account 
         *
         *  @param[in] strAccount The account to retrieve balance for, * for all
         *
         *  @param[out] nBalance The retrieved balance for the requested account
         *
         *  @param[in] nMinDepth Mimimum depth required for transaction balances to be included
         *
         *  @return true if balance retrieved successfully
         *
         **/
        bool BalanceByAccount(const std::string strAccount, int64_t& nBalance, const uint32_t nMinDepth = 3) const;

    };

}

#endif
