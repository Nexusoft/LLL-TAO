/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <exception>
#include <mutex>

#include <Legacy/include/evaluate.h>
#include <Legacy/types/address.h>
#include <Legacy/types/script.h>
#include <Legacy/wallet/addressbook.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/wallet/wallettx.h>

namespace Legacy
{

    /* Initialize static variables */
    std::mutex AddressBook::cs_addressBook;


    /* Adds an address book entry for a given Nexus address and address label. */
    bool AddressBook::SetAddressBookName(const NexusAddress& address, const std::string& strName)
    {
        {
            LOCK(AddressBook::cs_addressBook);

            mapAddressBook[address] = strName;

            if (addressBookWallet.IsFileBacked())
            {
                WalletDB walletdb(addressBookWallet.GetWalletFile());

                if (!walletdb.WriteName(address.ToString(), strName))
                    return debug::error(FUNCTION, "Failed writing address book entry");

                walletdb.Close();
            }
        }

        return true;
    }


    /* Removes the address book entry for a given Nexus address. */
    bool AddressBook::DelAddressBookName(const NexusAddress& address)
    {
        {
            LOCK(AddressBook::cs_addressBook);

            mapAddressBook.erase(address);

            if (addressBookWallet.IsFileBacked())
            {
                WalletDB walletdb(addressBookWallet.GetWalletFile());

                if (!walletdb.EraseName(address.ToString()))
                    throw std::runtime_error("AddressBook::DelAddressBookName() : removing address book entry failed");

                walletdb.Close();
            }
        }

        return true;
    }


    /* Get Nexus addresses that have a balance associated with the wallet for this address book */
    bool AddressBook::AvailableAddresses(const uint32_t nSpendTime, std::map<NexusAddress, int64_t>& mapAddressBalances,
                                          const bool fOnlyConfirmed, const uint32_t nMinDepth) const
    {
        { //Begin lock scope
            LOCK(AddressBook::cs_addressBook);

            for (const auto& item : addressBookWallet.mapWallet)
            {
                const WalletTx& walletTx = item.second;

                /* Filter any transaction output with timestamp exceeding requested spend time */
                if (walletTx.nTime > nSpendTime)
                    continue;

                /* Filter transaction from results if not final */
                if (!walletTx.IsFinal())
                    continue;

                /* Filter unconfirmed/immature transaction from results when only want confirmed */
                if (fOnlyConfirmed && !walletTx.IsConfirmed())
                    continue;

                /* Filter any transaction that does not meet minimum depth requirement */
                if (walletTx.GetDepthInMainChain() < nMinDepth)
                    continue;

                /* Filter immature coinbase/coinstake transaction */
                if ((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                    continue;

                for (int i = 0; i < walletTx.vout.size(); i++)
                {
                    /* For all unfiltered transactions, add any unspent output belonging to this wallet to the available balance */
                    if (!(walletTx.IsSpent(i)) && addressBookWallet.IsMine(walletTx.vout[i]) && walletTx.vout[i].nValue > 0)
                    {
                        NexusAddress address;

                        if(!ExtractAddress(walletTx.vout[i].scriptPubKey, address) || !address.IsValid())
                            return debug::error(FUNCTION, "Unable to extract valid Nexus address from walletTx key");

                        /* Add txout value to result */
                        if(mapAddressBalances.count(address) > 0)
                            mapAddressBalances[address] += walletTx.vout[i].nValue;
                        else
                            mapAddressBalances[address] = walletTx.vout[i].nValue;

                    }
                }
            }
        } //End lock scope

        return true;
    }


    /*  Get the current balance for a given account */
    bool AddressBook::BalanceByAccount(const std::string& strAccount, int64_t& nBalance, const uint32_t nMinDepth) const
    {
        { //Begin lock scope
            LOCK(AddressBook::AddressBook::cs_addressBook);

            nBalance = 0;

            for (const auto& item : addressBookWallet.mapWallet)
            {
                const WalletTx& walletTx = item.second;

                if (!walletTx.IsFinal())
                    continue;

                if (walletTx.GetDepthInMainChain() < nMinDepth)
                    continue;

                if ((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                    continue;

                for (int i = 0; i < walletTx.vout.size(); i++)
                {

                    if (!walletTx.IsSpent(i) && addressBookWallet.IsMine(walletTx.vout[i]) && walletTx.vout[i].nValue > 0)
                    {
                        if(strAccount == "*")
                        {
                            /* Wildcard request includes all accounts */
                            nBalance += walletTx.vout[i].nValue;

                            continue;
                        }

                        NexusAddress address;
                        if(!ExtractAddress(walletTx.vout[i].scriptPubKey, address) || !address.IsValid())
                            return false;

                        if(mapAddressBook.count(address))
                        {
                            std::string strEntry = mapAddressBook.at(address);
                            if(strEntry == "")
                                strEntry = "default";

                            if(strEntry == strAccount)
                                nBalance += walletTx.vout[i].nValue;
                        }
                        else if(strAccount == "default")
                            nBalance += walletTx.vout[i].nValue;

                    }
                }
            }
        } //End lock scope

        return true;
    }

    /* returns the address for the given account, adding a new address if one has not already been assigned*/
    Legacy::NexusAddress AddressBook::GetAccountAddress(const std::string& strAccount, bool fForceNew )
    {
        Legacy::NexusAddress address;
        bool fKeyUsed = false;
        
        if( !fForceNew )
        {
            // first look up the address currently assigned to the account
            AddressBookMap::iterator it = std::find_if(std::begin(mapAddressBook), std::end(mapAddressBook),
                            [=](AddressBookMap::value_type& entry) { return entry.second == strAccount; });

            if (it != std::end(mapAddressBook))
                address = it->first;

            

            // Check if the current key has been used
            if( address.IsValid() )
            {
                Legacy::CScript scriptPubKey;
                scriptPubKey.SetNexusAddress(address);
                for (std::map<uint512_t, Legacy::WalletTx>::iterator it = Legacy::Wallet::GetInstance().mapWallet.begin();
                        it != Legacy::Wallet::GetInstance().mapWallet.end() && !fKeyUsed;
                        ++it)
                {
                    const Legacy::WalletTx& wtx = (*it).second;
                    for(const Legacy::CTxOut& txout : wtx.vout)
                        if (txout.scriptPubKey == scriptPubKey)
                        {
                            fKeyUsed = true;
                            break;
                        }
                }
            }
        }

        // Generate a new key
        if (!address.IsValid() || fForceNew || fKeyUsed)
        {
            std::vector<uint8_t> vchPubKey;

            if (!Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vchPubKey, false))
                throw std::runtime_error("Error: Keypool ran out, please call keypoolrefill first");

            address = Legacy::NexusAddress(vchPubKey);
            SetAddressBookName(address, strAccount);
        }

        return address;
    }

}
