/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <mutex>

#include <TAO/Legacy/types/address.h>
#include <TAO/Legacy/types/script.h>
#include <TAO/Legacy/wallet/wallet.h>
#include <TAO/Legacy/wallet/walletdb.h>

namespace Legacy
{

    /* Adds an address book entry for a given Nexus address and address label. */
    bool CAddressBook::SetAddressBookName(const NexusAddress& address, const std::string& strName)
    {
        {
            std::lock_guard<std::mutex> walletLock(addressBookWallet.cs_wallet);

            mapAddressBook[address] = strName;

            if (addressBookWallet.IsFileBacked())
            {
                CWalletDB walletdb(addressBookWallet.GetWalletFile());

                if (!walletdb.WriteName(address.ToString(), strName))
                    throw runtime_error("CAddressBook::AddAddressBookName() : writing added address book entry failed");

                walletdb.Close();
            }
        }

        return true;
    }


    /* Removes the address book entry for a given Nexus address. */
    bool CAddressBook::DelAddressBookName(const NexusAddress& address)
    {
        {
            std::lock_guard<std::mutex> walletLock(addressBookWallet.cs_wallet);

            mapAddressBook.erase(address);

            if (addressBookWallet.IsFileBacked())
            {
                CWalletDB walletdb(addressBookWallet.GetWalletFile());

                if (!walletdb.EraseName(address.ToString()))
                    throw runtime_error("CAddressBook::DelAddressBookName() : removing address book entry failed");

                walletdb.Close();
            }
        }

        return;
    }


    /* Get Nexus addresses that have a balance associated with the wallet for this address book */
    bool CAddressBook::AvailableAddresses(uint32_t nSpendTime, map<NexusAddress, int64_t>& mapAddressBalances, bool fOnlyConfirmed) const
    {
        {
            std::lock_guard<std::mutex> walletLock(addressBookWallet.cs_wallet);

            for (auto& item : mapWallet)
            {
                const CWalletTx& walletTx = item.second;

                /* Filter any transaction output with timestamp exceeding requested spend time */
                if (walletTx.nTime > nSpendTime)
                    continue;  

                /* Filter transaction from results if not final */
                if (!walletTx.IsFinal())
                    continue;

                if (fOnlyConfirmed)
                {
                    /* Filter unconfirmed/immature transaction from results when only want confirmed */
                    if (!walletTx.IsConfirmed() ||  walletTx.GetBlocksToMaturity() > 0)
                        continue;
                }

                for (int i = 0; i < walletTx.vout.size(); i++)
                {
                    /* For all unfiltered transactions, add any unspent output belonging to this wallet to the available balance */
                    if (!(walletTx.IsSpent(i)) && IsMine(walletTx.vout[i]) && walletTx.vout[i].nValue > 0) 
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
        }

        return true;
    }

}
