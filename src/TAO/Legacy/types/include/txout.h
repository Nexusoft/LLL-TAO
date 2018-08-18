/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_TXIN_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_TXIN_H


namespace Legacy
{

	namespace Types
	{

        /** An output of a transaction.  It contains the public key that the next input
    	 * must be able to sign with to claim it.
    	 */
    	class CTxOut
    	{
    	public:
    		int64 nValue;
    		CScript scriptPubKey;

    		CTxOut()
    		{
    			SetNull();
    		}

    		CTxOut(int64 nValueIn, Wallet::CScript scriptPubKeyIn)
    		{
    			nValue = nValueIn;
    			scriptPubKey = scriptPubKeyIn;
    		}

    		IMPLEMENT_SERIALIZE
    		(
    			READWRITE(nValue);
    			READWRITE(scriptPubKey);
    		)

    		void SetNull()
    		{
    			nValue = -1;
    			scriptPubKey.clear();
    		}

    		bool IsNull()
    		{
    			return (nValue == -1);
    		}

    		void SetEmpty()
    		{
    			nValue = 0;
    			scriptPubKey.clear();
    		}

    		bool IsEmpty() const
    		{
    			return (nValue == 0 && scriptPubKey.empty());
    		}

    		uint512 GetHash() const
    		{
    			return SerializeHash(*this);
    		}

    		friend bool operator==(const CTxOut& a, const CTxOut& b)
    		{
    			return (a.nValue       == b.nValue &&
    					a.scriptPubKey == b.scriptPubKey);
    		}

    		friend bool operator!=(const CTxOut& a, const CTxOut& b)
    		{
    			return !(a == b);
    		}

    		std::string ToStringShort() const;

    		std::string ToString() const;

    		void print() const;
    	};
	}

}
