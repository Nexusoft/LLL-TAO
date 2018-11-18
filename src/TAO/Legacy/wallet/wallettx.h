/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLETTX_H
#define NEXUS_LEGACY_WALLET_WALLETTX_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/types/merkle.h>

#include <Util/templates/serialize.h>


/* forward declaration */
namespace LLD {
    class CIndexDB;
}

namespace Legacy
{
    
    /* forward declarations */    
    namespace Types
    {
        class CTxIn;
        class CTxOut;
    }

    namespace Wallet
    {
        /* forward declaration */
        class CWallet;

        /** A transaction with a bunch of additional info that only the owner cares about.
        * It includes any unrecorded transactions needed to link it back to the block chain.
        */
        class CWalletTx : public Legacy::Types::CMerkleTx
        {
        private:
            const CWallet* pwallet;

        public:
            std::vector<Legacy::Types::CMerkleTx> vtxPrev;
            std::map<std::string, std::string> mapValue;
            std::vector<std::pair<std::string, std::string> > vOrderForm;
            uint32_t fTimeReceivedIsTxTime;
            uint32_t nTimeReceived;  // time received by this node
            char fFromMe;
            std::string strFromAccount;
            std::vector<char> vfSpent; // which outputs are already spent

            // memory only
            mutable bool fDebitCached;
            mutable bool fCreditCached;
            mutable bool fAvailableCreditCached;
            mutable bool fChangeCached;
            mutable int64_t nDebitCached;
            mutable int64_t nCreditCached;
            mutable int64_t nAvailableCreditCached;
            mutable int64_t nChangeCached;

            CWalletTx()
            {
                Init(NULL);
            }

            CWalletTx(const CWallet* pwalletIn)
            {
                Init(pwalletIn);
            }

            CWalletTx(const CWallet* pwalletIn, const Legacy::Types::CMerkleTx& txIn) : Legacy::Types::CMerkleTx(txIn)
            {
                Init(pwalletIn);
            }

            CWalletTx(const CWallet* pwalletIn, const Legacy::Types::CTransaction& txIn) : Legacy::Types::CMerkleTx(txIn)
            {
                Init(pwalletIn);
            }

            void Init(const CWallet* pwalletIn)
            {
                pwallet = pwalletIn;
                vtxPrev.clear();
                mapValue.clear();
                vOrderForm.clear();
                fTimeReceivedIsTxTime = false;
                nTimeReceived = 0;
                fFromMe = false;
                strFromAccount.clear();
                vfSpent.clear();
                fDebitCached = false;
                fCreditCached = false;
                fAvailableCreditCached = false;
                fChangeCached = false;
                nDebitCached = 0;
                nCreditCached = 0;
                nAvailableCreditCached = 0;
                nChangeCached = 0;
            }

            IMPLEMENT_SERIALIZE
            (
                CWalletTx* pthis = const_cast<CWalletTx*>(this);
                if (fRead)
                    pthis->Init(NULL);
                char fSpent = false;

                if (!fRead)
                {
                    pthis->mapValue["fromaccount"] = pthis->strFromAccount;

                    std::string str;
                    for(char f : vfSpent)
                    {
                        str += (f ? '1' : '0');
                        if (f)
                            fSpent = true;
                    }
                    pthis->mapValue["spent"] = str;
                }

                nSerSize += SerReadWrite(s, *(Legacy::Types::CMerkleTx*)this, nSerType, nVersion,ser_action);
                READWRITE(vtxPrev);
                READWRITE(mapValue);
                READWRITE(vOrderForm);
                READWRITE(fTimeReceivedIsTxTime);
                READWRITE(nTimeReceived);
                READWRITE(fFromMe);
                READWRITE(fSpent);

                if (fRead)
                {
                    pthis->strFromAccount = pthis->mapValue["fromaccount"];

                    if (mapValue.count("spent"))
                        for(char c : pthis->mapValue["spent"])
                            pthis->vfSpent.push_back(c != '0');
                    else
                        pthis->vfSpent.assign(vout.size(), fSpent);
                }

                pthis->mapValue.erase("fromaccount");
                pthis->mapValue.erase("version");
                pthis->mapValue.erase("spent");
            )

            // marks certain txout's as spent
            // returns true if any update took place
            bool UpdateSpent(const std::vector<char>& vfNewSpent)
            {
                bool fReturn = false;
                for (uint32_t i = 0; i < vfNewSpent.size(); i++)
                {
                    if (i == vfSpent.size())
                        break;

                    if (vfNewSpent[i] && !vfSpent[i])
                    {
                        vfSpent[i] = true;
                        fReturn = true;
                        fAvailableCreditCached = false;
                    }
                }
                return fReturn;
            }

            // make sure balances are recalculated
            void MarkDirty()
            {
                fCreditCached = false;
                fAvailableCreditCached = false;
                fDebitCached = false;
                fChangeCached = false;
            }

            void BindWallet(CWallet *pwalletIn)
            {
                pwallet = pwalletIn;
                MarkDirty();
            }

            void MarkSpent(uint32_t nOut)
            {
                if (nOut >= vout.size())
                    throw std::runtime_error("CWalletTx::MarkSpent() : nOut out of range");
                vfSpent.resize(vout.size());
                if (!vfSpent[nOut])
                {
                    vfSpent[nOut] = true;
                    fAvailableCreditCached = false;
                }
            }

            void MarkUnspent(uint32_t nOut)
            {
                if (nOut >= vout.size())
                    throw std::runtime_error("CWalletTx::MarkUnspent() : nOut out of range");
                vfSpent.resize(vout.size());
                if (vfSpent[nOut])
                {
                    vfSpent[nOut] = false;
                    fAvailableCreditCached = false;
                }
            }

            bool IsSpent(uint32_t nOut) const
            {
                if (nOut >= vout.size())
                    throw std::runtime_error("CWalletTx::IsSpent() : nOut out of range");
                if (nOut >= vfSpent.size())
                    return false;
                return (!!vfSpent[nOut]);
            }

            int64_t GetDebit() const
            {
                if (vin.empty())
                    return 0;
                if (fDebitCached)
                    return nDebitCached;
                nDebitCached = pwallet->GetDebit(*this);
                fDebitCached = true;
                return nDebitCached;
            }

            int64_t GetCredit(bool fUseCache=true) const
            {
                // Must wait until coinbase / coinstake is safely deep enough in the chain before valuing it
                if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
                    return 0;

                // GetBalance can assume transactions in mapWallet won't change
                if (fUseCache && fCreditCached)
                    return nCreditCached;

                nCreditCached = pwallet->GetCredit(*this);
                fCreditCached = true;
                return nCreditCached;
            }

            int64_t GetAvailableCredit(bool fUseCache=true) const
            {
                // Must wait until coinbase is safely deep enough in the chain before valuing it
                if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
                    return 0;

                //if (fUseCache && fAvailableCreditCached)
                //return nAvailableCreditCached;

                int64_t nCredit = 0;
                for (uint32_t i = 0; i < vout.size(); i++)
                {
                    if (!IsSpent(i) && pwallet->IsMine(vout[i]) && vout[i].nValue > 0)
                    {
                        const Legacy::Types::CTxOut &txout = vout[i];
                        nCredit += pwallet->GetCredit(txout);
                        if (!Core::MoneyRange(nCredit))
                            throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
                    }
                }

                nAvailableCreditCached = nCredit;
                fAvailableCreditCached = true;
                return nCredit;
            }


            int64_t GetChange() const
            {
                if (fChangeCached)
                    return nChangeCached;
                nChangeCached = pwallet->GetChange(*this);
                fChangeCached = true;
                return nChangeCached;
            }

            void GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, std::list<std::pair<NexusAddress, int64_t> >& listReceived,
                            std::list<std::pair<NexusAddress, int64_t> >& listSent, int64_t& nFee, std::string& strSentAccount) const;

            void GetAccountAmounts(const std::string& strAccount, int64_t& nGenerated, int64_t& nReceived,
                                int64_t& nSent, int64_t& nFee) const;

            bool IsFromMe() const
            {
                return (GetDebit() > 0);
            }

            bool IsConfirmed() const
            {
                // Quick answer in most cases
                if (!IsFinal())
                    return false;
                if (GetDepthInMainChain() >= 1)
                    return true;
                if (!IsFromMe()) // using wtx's cached debit
                    return false;

                // If no confirmations but it's from us, we can still
                // consider it confirmed if all dependencies are confirmed
                std::map<uint512_t, const Legacy::Types::CMerkleTx*> mapPrev;
                std::vector<const Legacy::Types::CMerkleTx*> vWorkQueue;
                vWorkQueue.reserve(vtxPrev.size()+1);
                vWorkQueue.push_back(this);

                for (uint32_t i = 0; i < vWorkQueue.size(); i++)
                {
                    const Legacy::Types::CMerkleTx* ptx = vWorkQueue[i];

                    if (!ptx->IsFinal())
                        return false;
                    if (ptx->GetDepthInMainChain() >= 1)
                        continue;
                    if (!pwallet->IsFromMe(*ptx))
                        return false;

                    if (mapPrev.empty())
                    {
                        for(const Legacy::Types::CMerkleTx& tx : vtxPrev)
                            mapPrev[tx.GetHash()] = &tx;
                    }

                    for(const Legacy::Types::CTxIn& txin : ptx->vin)
                    {
                        if (!mapPrev.count(txin.prevout.hash))
                            return false;
                        vWorkQueue.push_back(mapPrev[txin.prevout.hash]);
                    }
                }
                return true;
            }

            bool WriteToDisk();

            int64_t GetTxTime() const;
            int GetRequestCount() const;

            void AddSupportingTransactions(LLD::CIndexDB& indexdb);

            bool AcceptWalletTransaction(LLD::CIndexDB& indexdb, bool fCheckInputs=true);
            bool AcceptWalletTransaction();

            void RelayWalletTransaction(LLD::CIndexDB& indexdb);
            void RelayWalletTransaction();
        };

    }
}

#endif
