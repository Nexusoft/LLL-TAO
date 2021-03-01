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
	/** atomic
	 *
	 *  Protects an object inside with a mutex.
	 *
	 **/
	template<class TypeName>
	class type
	{
	    /* The internal data. */
	    std::shared_ptr<TypeName> data;

	public:

	    /** Default Constructor. **/
	    type ()
	    : data (nullptr)
	    {
	    }


	    /** Constructor for storing. **/
	    type(const TypeName& dataIn)
	    : data (nullptr)
	    {
	        store(dataIn);
	    }


	    /** Assignment operator.
	     *
	     *  @param[in] a The type to assign from.
	     *
	     **/
	    type& operator=(const type& a)
	    {
	        store(a.load());
	        return *this;
	    }


	    /** Assignment operator.
	     *
	     *  @param[in] dataIn The type to assign from.
	     *
	     **/
	    type& operator=(const TypeName& dataIn)
	    {
	        store(dataIn);

	        return *this;
	    }


	    /** Equivilent operator.
	     *
	     *  @param[in] a The type to compare to.
	     *
	     **/
	    bool operator==(const type& a) const
	    {
	        return load() == a.load();
	    }


	    /** Equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator==(const TypeName& dataIn) const
	    {
	        return load() == dataIn;
	    }


	    /** Not equivilent operator.
	     *
	     *  @param[in] a The type to compare to.
	     *
	     **/
	    bool operator!=(const type& a) const
	    {
	        return load() != a.load();
	    }


	    /** Not equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator!=(const TypeName& dataIn) const
	    {
	        return load() != dataIn;
	    }


	    /** load
	     *
	     *  Load the object from memory.
	     *
	     **/
	    TypeName load() const
	    {
	        return *std::atomic_load_explicit(&data, std::memory_order_seq_cst);
	    }


	    /** store
	     *
	     *  Stores an object into memory.
	     *
	     *  @param[in] dataIn The data to into protected memory.
	     *
	     **/
	    void store(const TypeName& dataIn)
	    {
	        /* Cleanup our memory usage on store. */
	        if(data)
	            data = nullptr;

	        std::atomic_store_explicit(&data, std::make_shared<TypeName>(dataIn), std::memory_order_seq_cst);
	    }
	};
}
