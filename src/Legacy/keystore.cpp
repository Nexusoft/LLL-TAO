/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/wallet/keystore.h>

namespace Legacy 
{
    
    /*  Retrieve the public key for a key in the key store. */
    bool KeyStore::GetPubKey(const NexusAddress& address, std::vector<uint8_t>& vchPubKeyOut) const
    {
        LLC::ECKey key;
        if(!GetKey(address, key))
            return false;
        vchPubKeyOut = key.GetPubKey();
        return true;
    }


    /*  Retrieve the private key associated with an address. */
    bool KeyStore::GetSecret(const NexusAddress& address, LLC::CSecret& vchSecret, bool& fCompressed) const
    {
        LLC::ECKey key;
        if(!GetKey(address, key))
            return false;

        vchSecret = key.GetSecret(fCompressed);

        return true;
    }

}
