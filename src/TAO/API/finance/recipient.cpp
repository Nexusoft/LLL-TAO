/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/finance/types/recipient.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Default Constructor. */
        Recipient::Recipient()
        : hashAddress ( )
        , nAmount     (0)
        , nReference  (0)
        {
        }


        /* Constructor from values. */
        Recipient::Recipient(const TAO::Register::Address& hashAddressIn, const uint64_t nAmountIn, const uint64_t nReferenceIn)
        : hashAddress (hashAddressIn)
        , nAmount     (nAmountIn)
        , nReference  (nReferenceIn)
        {
        }


        /* Copy constructor. */
        Recipient::Recipient(const Recipient& balance)
        : hashAddress (balance.hashAddress)
        , nAmount     (balance.nAmount)
        , nReference  (balance.nReference)
        {
        }


        /* Move constructor. */
        Recipient::Recipient(Recipient&& balance) noexcept
        : hashAddress (std::move(balance.hashAddress))
        , nAmount     (std::move(balance.nAmount))
        , nReference  (std::move(balance.nReference))
        {

        }


        /* Copy assignment. */
        Recipient& Recipient::operator=(const Recipient& balance)
        {
            hashAddress = balance.hashAddress;
            nAmount     = balance.nAmount;
            nReference  = balance.nReference;

            return *this;
        }


        /* Move assignment. */
        Recipient& Recipient::operator=(Recipient&& balance) noexcept
        {
            hashAddress = std::move(balance.hashAddress);
            nAmount     = std::move(balance.nAmount);
            nReference  = std::move(balance.nReference);

            return *this;
        }


        /* Default Destructor. */
        Recipient::~Recipient()
        {
        }
            
    }
}
