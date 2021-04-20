/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/check.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Determines whether a string value is a valid base58 encoded register address.
     *  This only checks to see if the value an be decoded into a valid Register::Address with a valid Type.
     *  It does not check to see whether the register address exists in the database
     */
    bool CheckAddress(const std::string& strValueToCheck)
    {
        /* Decode the incoming string into a register address */
        TAO::Register::Address address;
        address.SetBase58(strValueToCheck);

        /* Check to see whether it is valid */
        return address.IsValid();

    }


    /* Utilty method that checks that the signature chain is mature and can therefore create new transactions.
    *  Throws an appropriate APIException if it is not mature. */
    void CheckMature(const uint256_t& hashGenesis)
    {
        /* No need to check this in private mode as there is no PoS/Pow */
        if(!config::fHybrid.load())
        {
            /* Get the number of blocks to maturity for this sig chain */
            const uint32_t nBlocksToMaturity = users->BlocksToMaturity(hashGenesis);
            if(nBlocksToMaturity > 0)
                throw APIException(-202, debug::safe_printstr( "Signature chain not mature after your previous mined/stake block. ", nBlocksToMaturity, " more confirmation(s) required."));
        }
    }
} // End TAO namespace
