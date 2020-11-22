/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/unpack.h>

#include <Util/include/encoding.h>
#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Utility method to determine whether a contract is a DEX contract.  This is identified by the operation being a 
         *  conditional DEBIT or TRANSFER being made to a wildcard address  */
        bool IsDEX(const TAO::Operation::Contract& contract)
        {
            bool fDEX = false;

            /* Firstly check that it is conditional */
            if(contract.IsCondition())
            {
                /* Reset all streams */
                contract.Reset();

                /* Seek the operation stream to the position of the primitive. */
                contract.SeekToPrimitive();

                /* Deserialize the primitive operation */
                uint8_t nOP;
                contract >> nOP;

                /* Check the primitive */
                if(nOP == TAO::Operation::OP::DEBIT || nOP == TAO::Operation::OP::TRANSFER)
                {
                    /* The sending account / register */
                    uint8_t hashFrom;
                    contract >> hashFrom;

                    /* The recipient address */
                    uint256_t hashTo;
                    contract >> hashTo;
                    
                    /* Check that the recipient is a wildcard */
                    fDEX = hashTo == TAO::Register::WILDCARD_ADDRESS;

                }
            }

            return fDEX;

        }


        /* Utility method to generate a unique identifier for the market pair.  This is used for the batch key for storing
        *  DEX orders in the DEX database, which requires a string for the key.  */
        std::string GetMarketID(const TAO::Register::Address& hashSell, const TAO::Register::Address hashBuy)
        {
            /* Stream to serialize the market data */
            DataStream ssData(SER_REGISTER, 1);

            /* Add initial byte to distinguish token pairs from assets from from NXS (which has a token ID of 0) */
            if(hashSell == 0 ) // NXS
                ssData << uint256_t(0);
            else if(hashSell.IsToken()) // Token pair
                ssData << hashSell;
            else
                ssData << uint256_t(1); // a non token market (an asset or other register for sale)

            /* Add the receiving token bytes to the stream */
            ssData << hashBuy;

            /* Hash the data to reduce its size to 64 bit */
            uint64_t hashMarket = LLC::SK64(ssData.Bytes());            
        
            /* Convert to string */
            std::string strMarket = std::to_string(hashMarket);
            
            /* Base58 encode it  */
            return encoding::EncodeBase58(std::vector<uint8_t>(strMarket.begin(), strMarket.end()));
        }

    } /* End API namespace */

}/* End TAO namespace*/