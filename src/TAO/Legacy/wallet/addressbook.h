/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_ADDRESSBOOK_H
#define NEXUS_LEGACY_WALLET_ADDRESSBOOK_H

#include <map>
#include <string>

namespace Legacy
{
    
    /* forward declarations */    
    class NexusAddress;
    class CWallet;


    /** AddressBookMap is type alias defining a map for storing address book labels by Nexus address. **/
    using AddressBookMap = std::map<Legacy::Types::NexusAddress, std::string>;


    /** @class CAddressBook
     *
     *  Implements an address book mapping Nexus addresses (accounts) to address labels.
     * 
     *  For a file backed wallet, adding or removing address book entries will also update
     *  the wallet database. 
     *
     *  Copy operations are disabled on CAddressBook. This prevents the possibility of 
     *  having multiple copies where an address gets added or removed from one copy and not
     *  the other.
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


        /** Copy constructor deleted. No copy allowed **/
        CAddressBook(const CAddressBook&) = delete;


        /** Copy assignment operator deleted. No copy allowed **/
        CAddressBook& operator= (const CAddressBook &rhs) = delete;


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
         *  @return the address label
         *
         **/
        bool AvailableAddresses(uint32_t nSpendTime, std::map<Legacy::Types::NexusAddress, int64_t>& mapAddressBalances, bool fOnlyConfirmed = false) const;

    };

}
