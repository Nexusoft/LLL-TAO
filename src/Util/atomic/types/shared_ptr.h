/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <atomic>

namespace util::atomic
{

	/** shared_ptr
	 *
	 *  Protects an object inside with a mutex.
	 *
	 **/
	template<class TypeName>
	class shared_ptr
	{
	    /* The internal data. */
	    std::shared_ptr<TypeName> pdata;

	public:

	    /** Default Constructor. **/
	    shared_ptr ()
	    : pdata (nullptr)
	    {
	    }


	    /** Constructor for storing. **/
	    shared_ptr(const TypeName* dataIn)
	    : pdata (nullptr)
	    {
	        store(*dataIn);
	    }


	    /** Assignment operator.
	     *
	     *  @param[in] a The atomic to assign from.
	     *
	     **/
	    shared_ptr& operator=(const shared_ptr& a)
	    {
	        store(*a.load());
	        return *this;
	    }


	    /** Assignment operator.
	     *
	     *  @param[in] dataIn The atomic to assign from.
	     *
	     **/
	    shared_ptr& operator=(const TypeName& in)
	    {
	        store(in);

	        return *this;
	    }


	    /** Equivilent operator.
	     *
	     *  @param[in] a The atomic to compare to.
	     *
	     **/
	    bool operator==(const shared_ptr& a) const
	    {
	        return *load() == *a.load();
	    }


	    /** Equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator==(const TypeName& in) const
	    {
	        return *load() == in;
	    }


	    /** Not equivilent operator.
	     *
	     *  @param[in] a The atomic to compare to.
	     *
	     **/
	    bool operator!=(const shared_ptr& a) const
	    {
	        return *load() != *a.load();
	    }


	    /** Not equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator!=(const TypeName& in) const
	    {
	        return *load() != in;
	    }


	    /** load
	     *
	     *  Load the object from memory.
	     *
	     **/
	    std::shared_ptr<TypeName> load(const std::memory_order mOrder = std::memory_order_seq_cst) const
	    {
	        return std::atomic_load_explicit(&pdata, mOrder);
	    }


	    /** operator()
	     *
	     *  Atomically loads the value pointed to by pdata.
	     *
	     **/
	    operator TypeName()
	    {
	        return std::atomic_load_explicit(&pdata, std::memory_order_seq_cst);
	    }


	    /** store
	     *
	     *  Stores an object into memory.
	     *
	     *  @param[in] dataIn The data to into protected memory.
	     *
	     **/
	    void store(const TypeName& in, const std::memory_order mOrder = std::memory_order_seq_cst)
	    {
	        /* Cleanup our memory usage on store. */
	        if(pdata)
	            pdata = nullptr;

	        std::atomic_store_explicit(&pdata, std::make_shared<TypeName>(in), mOrder);
	    }


	    /** compare_exchange_weak
	     *
	     *  Exchanges two shared_ptr objects if they are equal, returns false otherwise.
	     *  This can have spurious wake-ups so should be used in a loop.
	     *
	     *  @param[in] pExpected The expected pointer value
	     *  @param[in] pDesired The desired pointer value.
	     *  @param[in] mSuccess The memory ordering for successful operation
	     *  @param[in] mFailure The memory ordering for failed operation
	     *
	     *  @return true if the values are equal and operation succeeds.
	     *
	     **/
	    bool compare_exchange_weak(std::shared_ptr<TypeName> &pExpected, std::shared_ptr<TypeName> pDesired,
	        const std::memory_order mSuccess = std::memory_order_seq_cst, const std::memory_order mFailure = std::memory_order_seq_cst)
	    {
	        return std::atomic_compare_exchange_weak_explicit(&pdata, &pExpected, pDesired, mSuccess, mFailure);
	    }


	    /** compare_exchange_strong
	     *
	     *  Exchanges two shared_ptr objects if they are equal, returns false otherwise.
	     *  This is guarenteed to have no spurious wake-ups, but padding bits can cause false negatives.
	     *
	     *  @param[in] pExpected The expected pointer value
	     *  @param[in] pDesired The desired pointer value.
	     *  @param[in] mSuccess The memory ordering for successful operation
	     *  @param[in] mFailure The memory ordering for failed operation
	     *
	     *  @return true if the values are equal and operation succeeds.
	     *
	     **/
	    bool compare_exchange_strong(std::shared_ptr<TypeName> &pExpected, std::shared_ptr<TypeName> pDesired,
	        const std::memory_order mSuccess = std::memory_order_seq_cst, const std::memory_order mFailure = std::memory_order_seq_cst)
	    {
	        return std::atomic_compare_exchange_strong_explicit(&pdata, &pExpected, pDesired, mSuccess, mFailure);
	    }


	    /** compare_exchange_auto
	     *
	     *  Exchanges two shared_ptr objects if they are equal, returns false otherwise.
	     *  This guarentees exchange with one call, and handles spurious wake-ups automatically.
	     *
	     *  This method does not return for success or failure, it continues until it succeeds.
	     *
	     *  @param[in] pExpected The expected pointer value
	     *  @param[in] pDesired The desired pointer value
	     *  @param[in] mSuccess The memory ordering for successful operation
	     *  @param[in] mFailure The memory ordering for failed operation
	     *
	     *
	     **/
	    void compare_exchange_auto(std::shared_ptr<TypeName> &pExpected, std::shared_ptr<TypeName> pDesired,
	        const std::memory_order mSuccess = std::memory_order_seq_cst, const std::memory_order mFailure = std::memory_order_seq_cst)
	    {
	        while(!std::atomic_compare_exchange_weak_explicit(&pdata, &pExpected, pDesired, mSuccess, mFailure));
	    }


	    /** exchange
	     *
	     *  Exchanges pDesired with pdata and returns older value.
	     *
	     *  @param[in] pDesired The pointer to exchange.
	     *
	     *  @return The old value pointed to by p
	     **/
	    std::shared_ptr<TypeName> exchange(std::shared_ptr<TypeName> pDesired, const std::memory_order mOrder = std::memory_order_seq_cst)
	    {
	        return std::atomic_exchange_explicit(&pdata, pDesired, mOrder);
	    }
	};
}
