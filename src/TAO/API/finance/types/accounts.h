/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/wallet/wallet.h>

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>

#include <TAO/API/include/conditions.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>
#include <Util/include/string.h>
#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Token struct to manage token related data. **/
    class Accounts
    {
        /** Vector of addresses and balances to check against. **/
        std::vector<std::pair<uint256_t, uint64_t>> vAddresses;


        /** Set of addresses, to track duplicate entries. **/
        std::set<uint256_t> setAddresses;


        /** Iterator for our vector to get current account. **/
        uint32_t nIterator;


    public:

        /** The decimals for this specific token. **/
        uint8_t nDecimals;


        /* Default constructor. */
        Accounts()
        : vAddresses   ( )
        , setAddresses ( )
        , nIterator    (0)
        , nDecimals    (0)
        {
        }


        /* Copy constructor. */
        Accounts(const Accounts& in)
        : vAddresses   (in.vAddresses)
        , setAddresses (in.setAddresses)
        , nIterator    (in.nIterator)
        , nDecimals    (in.nDecimals)
        {
        }


        /* Move constructor. */
        Accounts(Accounts&& in)
        : vAddresses   (std::move(in.vAddresses))
        , setAddresses (std::move(in.setAddresses))
        , nIterator    (std::move(in.nIterator))
        , nDecimals    (std::move(in.nDecimals))
        {
        }


        /** Copy assignment. **/
        Accounts& operator=(const Accounts& in)
        {
            vAddresses   = in.vAddresses;
            setAddresses = in.setAddresses;
            nIterator    = in.nIterator;
            nDecimals    = in.nDecimals;

            return *this;
        }


        /** Move assignment. **/
        Accounts& operator=(Accounts&& in)
        {
            vAddresses   = std::move(in.vAddresses);
            setAddresses = std::move(in.setAddresses);
            nIterator    = std::move(in.nIterator);
            nDecimals    = std::move(in.nDecimals);

            return *this;
        }


        /** Default destructor. **/
        ~Accounts()
        {
        }


        /** Constructor for decimals. **/
        Accounts(const uint8_t nDecimalsIn)
        : vAddresses   ( )
        , setAddresses ( )
        , nIterator    (0)
        , nDecimals    (nDecimalsIn)
        {
        }


        /** Operator-= overload
         *
         *  Adjusts the balance for the currently loaded account.
         *
         *  @param[in] nBalance The balance to deduct by.
         *
         **/
        Accounts& operator-=(const uint32_t nBalance)
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            /* Check that we don't underflow here. */
            if(vAddresses[nIterator].second < nBalance)
                throw APIException(-69, "Insufficient funds");

            /* Adjust the balance for designated account. */
            vAddresses[nIterator].second -= nBalance;

            return *this;
        }


        /** Operator++ overload
         *
         *  Adjusts the iterator for the currently loaded account.
         *
         **/
        Accounts& operator++(int)
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(++nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            return *this;
        }


        /** Insert
         *
         *  Insert a new address to our addresses vector.
         *
         *  @param[in] hashAddress The address to add to the list.
         *  @param[in] nBalance The balance to add to the vector.
         *
         **/
        void Insert(const uint256_t& hashAddress, const uint64_t nBalance)
        {
            /* Check for duplicates (check to protect against misuse). */
            if(setAddresses.count(hashAddress))
                return;

            /* Add to set for duplicate checks. */
            setAddresses.insert(hashAddress);

            /* Add to our address vector. */
            vAddresses.push_back(std::make_pair(hashAddress, nBalance));
        }


        /** HasNext
         *
         *  Checks if we have another account to iterate to.
         *
         **/
        bool HasNext() const
        {
            return (nIterator < (vAddresses.size() - 1));
        }


        /** GetBalance
         *
         *  Get the balance of current token account being iterated.
         *
         **/
        uint64_t GetBalance() const
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            return vAddresses[nIterator].second;
        }


        /** GetAddress
         *
         *  Gets the current address of the account we are operating on.
         *
         **/
        uint256_t GetAddress() const
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            return vAddresses[nIterator].first;
        }
    };
}
