/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#include "../main.h"

namespace Core
{
	/* Dispatching Functions: To Dispatch to Registered Wallets */
	void RegisterWallet(Wallet::CWallet* pwalletIn)
	{
		{
			LOCK(cs_setpwalletRegistered);
			setpwalletRegistered.insert(pwalletIn);
		}
	}

	void UnregisterWallet(Wallet::CWallet* pwalletIn)
	{
		{
			LOCK(cs_setpwalletRegistered);
			setpwalletRegistered.erase(pwalletIn);
		}
	}

	/* Check whether the transaction is from us */
	bool IsFromMe(CTransaction& tx)
	{
		for(auto pwallet : setpwalletRegistered)
			if (pwallet->IsFromMe(tx))
				return true;
		return false;
	}

	/* Get the transaction from the Wallet */
	bool GetTransaction(const uint512& hashTx, Wallet::CWalletTx& wtx)
	{
		for(auto pwallet : setpwalletRegistered)
			if (pwallet->GetTransaction(hashTx,wtx))
				return true;
		return false;
	}

	/* Removes given transaction from the wallet */
	void EraseFromWallets(uint512 hash)
	{
		for(auto pwallet : setpwalletRegistered)
			pwallet->EraseFromWallet(hash);
	}

	/* Make sure all wallets know about the given transaction, in the given block */
	void SyncWithWallets(const CTransaction& tx, const CBlock* pblock, bool fUpdate, bool fConnect)
	{
		if (!fConnect)
		{
			/** Wallets need to refund inputs when disconnecting coinstake **/
			if (tx.IsCoinStake())
			{
				for(auto pwallet : setpwalletRegistered)
					if (pwallet->IsFromMe(tx))
						pwallet->DisableTransaction(tx);
			}
			return;
		}

		for(auto pwallet : setpwalletRegistered)
			pwallet->AddToWalletIfInvolvingMe(tx, pblock, fUpdate);
	}

	/* Notify wallets about a new best chain */
	bool SetBestChain(const CBlockLocator& loc)
	{
		for(auto pwallet : setpwalletRegistered)
			pwallet->SetBestChain(loc);
			
		return true;
	}

	/* Notify wallets about an updated transaction */
	void UpdatedTransaction(const uint512& hashTx)
	{
		for(auto pwallet : setpwalletRegistered)
			pwallet->UpdatedTransaction(hashTx);
	}

	/* Dump All Wallets */
	void PrintWallets(const CBlock& block)
	{
		for(auto pwallet : setpwalletRegistered)
			pwallet->PrintWallet(block);
	}

	/* Notify wallets about an incoming inventory (for request counts) */
	void Inventory(const uint1024& hash)
	{
		for(auto pwallet : setpwalletRegistered)
			pwallet->Inventory(hash);
	}

	/* Ask wallets to resend their transactions */
	void ResendWalletTransactions()
	{
		for(auto pwallet : setpwalletRegistered)
			pwallet->ResendWalletTransactions();
	}
}

