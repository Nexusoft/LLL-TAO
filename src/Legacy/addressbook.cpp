/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

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

    /* Adds an address book entry for a given Nexus address and address label. */
    bool CAddressBook::SetAddressBookName(const NexusAddress& address, const std::string& strName)
    {
        {
            std::lock_guard<std::recursive_mutex> walletLock(addressBookWallet.cs_wallet);

            mapAddressBook[address] = strName;

            if (addressBookWallet.IsFileBacked())
            {
                CWalletDB walletdb(addressBookWallet.GetWalletFile());

                if (!walletdb.WriteName(address.ToString(), strName))
                    throw std::runtime_error("CAddressBook::AddAddressBookName() : writing added address book entry failed");

                walletdb.Close();
            }
        }

        return true;
    }


    /* Removes the address book entry for a given Nexus address. */
    bool CAddressBook::DelAddressBookName(const NexusAddress& address)
    {
        {
            std::lock_guard<std::recursive_mutex> walletLock(addressBookWallet.cs_wallet);

            mapAddressBook.erase(address);

            if (addressBookWallet.IsFileBacked())
            {
                CWalletDB walletdb(addressBookWallet.GetWalletFile());

                if (!walletdb.EraseName(address.ToString()))
                    throw std::runtime_error("CAddressBook::DelAddressBookName() : removing address book entry failed");

                walletdb.Close();
            }
        }

        return true;
    }


    /* Get Nexus addresses that have a balance associated with the wallet for this address book */
    bool CAddressBook::AvailableAddresses(const uint32_t nSpendTime, std::map<NexusAddress, int64_t>& mapAddressBalances, 
                                          const bool fOnlyConfirmed, const uint32_t nMinDepth) const
    {
        { //Begin lock scope
            std::lock_guard<std::recursive_mutex> walletLock(addressBookWallet.cs_wallet);

            for (auto& item : addressBookWallet.mapWallet)
            {
                const CWalletTx& walletTx = item.second;

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
                            return false; // Unable to extract valid address from mapWallet. Should it really eat that error?

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
    bool CAddressBook::BalanceByAccount(const std::string strAccount, int64_t& nBalance, const uint32_t nMinDepth) const
    {
        { //Begin lock scope
            std::lock_guard<std::recursive_mutex> walletLock(addressBookWallet.cs_wallet);

            nBalance = 0;

            for (auto& item : addressBookWallet.mapWallet)
            {
                const CWalletTx& walletTx = item.second;

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

}
