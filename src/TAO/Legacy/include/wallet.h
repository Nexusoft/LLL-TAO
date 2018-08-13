/** Locate the Add Coinstake Inputs Method Here for access. **/
namespace Wallet
{

    bool CWallet::AddCoinstakeInputs(Core::CTransaction& txNew)
    {
        
        /* Add Each Input to Transaction. */
        vector<const CWalletTx*> vInputs;
        vector<const CWalletTx*> vCoins;
        
        txNew.vout[0].nValue = 0;
        
        {
        LOCK(cs_wallet);
        
        vCoins.reserve(mapWallet.size());
        for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            vCoins.push_back(&(*it).second);
        }
        
        random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);
        BOOST_FOREACH(const CWalletTx* pcoin, vCoins)
        {
            if (!pcoin->IsFinal() || pcoin->GetDepthInMainChain() < 60)
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->IsSpent(i) || !IsMine(pcoin->vout[i]))
                    continue;

                if (pcoin->nTime > txNew.nTime)
                    continue;

                //if(txNew.vout[0].nValue > (nBalance - nReserveBalance))
                //	break;
                    
                /* Stop adding Inputs if has reached Maximum Transaction Size. */
                unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
                if (nBytes >= Core::MAX_BLOCK_SIZE_GEN / 5)
                    break;

                txNew.vin.push_back(Core::CTxIn(pcoin->GetHash(), i));
                vInputs.push_back(pcoin);
                    
                /** Add the value to the first Output for Coinstake. **/
                txNew.vout[0].nValue += pcoin->vout[i].nValue;
            }
        }
        
        if(txNew.vin.size() == 1)
            return false;
            
        /** Set the Interest for the Coinstake Transaction. **/
        int64 nInterest;
        LLD::CIndexDB indexdb("rw");
        if(!txNew.GetCoinstakeInterest(indexdb, nInterest))
            return error("AddCoinstakeInputs() : Failed to Get Interest");
        
        txNew.vout[0].nValue += nInterest;
            
        /** Sign Each Input to Transaction. **/
        for(int nIndex = 0; nIndex < vInputs.size(); nIndex++)
        {
            if (!SignSignature(*this, *vInputs[nIndex], txNew, nIndex + 1))
                return error("AddCoinstakeInputs() : Unable to sign Coinstake Transaction Input.");

        }
        
        return true;
    }
}
