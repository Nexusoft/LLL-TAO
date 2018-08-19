/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_INPOINT_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_INPOINT_H


namespace Legacy
{

	namespace Types
	{

		/** An inpoint - a combination of a transaction and an index n into its vin */
		class CInPoint
		{
		public:

			/** The transaction pointer. **/
			CTransaction* ptx;


			/** The index n of transaction input. **/
			unsigned int n;


			/** Default Constructor
			 *
			 *	Sets object to null state.
			 *
			 **/
			CInPoint() { SetNull(); }


			/** Constructor
			 *
			 *	@param[in] ptxIn The transaction input pointer
			 *	@param[in] nIn The index input
			 *
			 **/
			CInPoint(CTransaction* ptxIn, unsigned int nIn)
			{
				ptx = ptxIn;
				n = nIn;
			}


			/** Set Null
			 *
			 *	Sets the object to null state
			 *
			 **/
			void SetNull()
			{
				ptx = NULL;
				n = -1;
			}


			/** Is Null
			 *
			 *	Checks the objects null state.
			 *
			 **/
			bool IsNull() const
			{
				return (ptx == NULL && n == -1);
			}
		};

	}
}
