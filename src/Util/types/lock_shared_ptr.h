/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

namespace memory
{

	/** lock_control
	 *
	 *  Track the current pointer references.
	 *
	 **/
	struct lock_control
	{
	    /** Recursive mutex for locking lock_shared_ptr. **/
	    std::recursive_mutex MUTEX;


	    /** Reference counter for active copies. **/
	   std::atomic<uint32_t> nCount;


	    /** Default Constructor. **/
	    lock_control( )
	    : MUTEX  ( )
	    , nCount (1)
	    {
	    }


	    /** count
	     *
	     *  Access atomic with easier syntax.
	     *
	     **/
	    uint32_t count()
	    {
	        return nCount.load();
	    }
	};


	/** lock_proxy
	 *
	 *  Temporary class that unlocks a mutex when outside of scope.
	 *  Useful for protecting member access to a raw pointer.
	 *
	 **/
	template <class TypeName>
	class lock_proxy
	{
	    /** Reference of the mutex. **/
	    std::recursive_mutex& MUTEX;


	    /** The pointer being locked. **/
	    TypeName* pData;


	public:

	    /** Basic constructor
	     *
	     *  Assign the pointer and reference to the mutex.
	     *
	     *  @param[in] pData The pointer to shallow copy
	     *  @param[in] MUTEX_IN The mutex reference
	     *
	     **/
	    lock_proxy(TypeName* pDataIn, std::recursive_mutex& MUTEX_IN)
	    : MUTEX (MUTEX_IN)
	    , pData (pDataIn)
	    {
	    }


	    /** Destructor
	    *
	    *  Unlock the mutex.
	    *
	    **/
	    ~lock_proxy()
	    {
	    	MUTEX.unlock();
	    }


	    /** Member Access Operator.
	    *
	    *  Access the memory of the raw pointer.
	    *
	    **/
	    TypeName* operator->() const
	    {
	    	return pData;
	    }
	};


	/** lock_shared_ptr
	 *
	 *  Pointer with member access protected with a mutex.
	 *
	 **/
	template<class TypeName>
	class lock_shared_ptr
	{
	    /** The internal locking mutex. **/
	    mutable lock_control* pRefs;


	    /** The internal raw poitner. **/
	    TypeName* pData;


	public:

	    /** Default Constructor. **/
	    lock_shared_ptr()
	    : pRefs  (nullptr)
	    , pData  (nullptr)
	    {
	    }


	    /** Constructor for storing. **/
	    lock_shared_ptr(TypeName* pDataIn)
	    : pRefs  (new lock_control())
	    , pData  (pDataIn)
	    {
	    }


	    /** Copy Constructor. **/
	    lock_shared_ptr(const lock_shared_ptr<TypeName>& ptrIn)
	    : pRefs (ptrIn.pRefs)
	    , pData (ptrIn.pData)
	    {
	        ++pRefs->nCount;
	    }


	    /** Move Constructor. **/
	    lock_shared_ptr(const lock_shared_ptr<TypeName>&& ptrIn)
	    : pRefs (std::move(ptrIn.pRefs))
	    , pData (std::move(ptrIn.pData))
	    {
	    }


	    /** Destructor. **/
	    ~lock_shared_ptr()
	    {
	        /* Adjust our reference count. */
	        if(pRefs->count() > 0)
	            --pRefs->nCount;

	        /* Delete if no more references. */
	        if(pRefs->count() == 0)
	        {
	            /* Cleanup the main raw pointer. */
	            if(pData)
	            {
	                delete pData;
	                pData = nullptr;
	            }

	            /* Cleanup the control block. */
	            if(pRefs)
	            {
	                delete pRefs;
	                pRefs = nullptr;
	            }
	        }
	    }


	    /** Assignment operator. **/
	    lock_shared_ptr& operator=(const lock_shared_ptr<TypeName>& ptrIn)
	    {
	        /* Shallow copy pointer and control block. */
	        pRefs  = ptrIn.pRefs;
	        pData  = ptrIn.pData;

	        /* Increase our reference count now. */
	        ++pRefs->nCount;
	    }


	    /** Assignment operator. **/
	    lock_shared_ptr& operator=(TypeName* pDataIn) = delete;


	    /** Equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator==(const TypeName& pDataIn) const
	    {
	        RECURSIVE(pRefs->MUTEX);

	        /* Check for dereferencing nullptr. */
	        if(pData == nullptr)
	            throw std::range_error(debug::warning(FUNCTION, "dereferencing nullptr"));

	        return *pData == pDataIn;
	    }


	    /** Equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator==(const TypeName* pDataIn) const
	    {
	        RECURSIVE(pRefs->MUTEX);

	        return pData == pDataIn;
	    }


	    /** Not equivilent operator.
	     *
	     *  @param[in] a The data type to compare to.
	     *
	     **/
	    bool operator!=(const TypeName* pDataIn) const
	    {
	        RECURSIVE(pRefs->MUTEX);

	        return pData != pDataIn;
	    }

	    /** Not operator
	     *
	     *  Check if the pointer is nullptr.
	     *
	     **/
	    bool operator!(void)
	    {
	        RECURSIVE(pRefs->MUTEX, pRefs->MUTEX);

	        return pData == nullptr;
	    }


	    /** Member access overload
	     *
	     *  Allow lock_shared_ptr access like a normal pointer.
	     *
	     **/
	    lock_proxy<TypeName> operator->()
	    {
	        /* Lock our mutex before going forward. */
	        pRefs->MUTEX.lock();

	        return lock_proxy<TypeName>(pData, pRefs->MUTEX);
	    }


	    /** dereference operator overload
	     *
	     *  Load the object from memory.
	     *
	     **/
	    TypeName operator*() const
	    {
	        RECURSIVE(pRefs->MUTEX);

	        /* Check for dereferencing nullptr. */
	        if(pData == nullptr)
	            throw std::range_error(debug::warning(FUNCTION, "dereferencing nullptr"));

	        return *pData;
	    }
	};
}
