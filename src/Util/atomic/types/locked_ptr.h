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
		TypeName* data;


	public:

		/** Basic constructor
		 *
		 *  Assign the pointer and reference to the mutex.
		 *
		 *  @param[in] pData The pointer to shallow copy
		 *  @param[in] MUTEX_IN The mutex reference
		 *
		 **/
		lock_proxy(TypeName* pData, std::recursive_mutex& MUTEX_IN)
		: MUTEX(MUTEX_IN)
		, data(pData)
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
			if(data == nullptr)
				throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));

			return data;
		}
	};


	/** locked_ptr
	 *
	 *  Pointer with member access protected with a mutex.
	 *
	 **/
	template<class TypeName>
	class locked_ptr
	{
		/** The internal locking mutex. **/
		mutable std::recursive_mutex MUTEX;


		/** The internal raw poitner. **/
		TypeName* data;


	public:

		/** Default Constructor. **/
		locked_ptr()
		: MUTEX()
		, data(nullptr)
		{
		}


		/** Constructor for storing. **/
		locked_ptr(TypeName* dataIn)
		: MUTEX()
		, data(dataIn)
		{
		}


		/** Copy Constructor. **/
		locked_ptr(const locked_ptr<TypeName>& pointer) = delete;


		/** Move Constructor. **/
		locked_ptr(const locked_ptr<TypeName>&& pointer)
		: data(pointer.data)
		{
		}


		/** Destructor. **/
		~locked_ptr()
		{
		}


		/** Assignment operator. **/
		locked_ptr& operator=(const locked_ptr<TypeName>& dataIn) = delete;


		/** Assignment operator. **/
		locked_ptr& operator=(TypeName* dataIn) = delete;


		/** Equivilent operator.
		 *
		 *  @param[in] a The data type to compare to.
		 *
		 **/
		bool operator==(const TypeName& dataIn) const
		{
			RLOCK(MUTEX);

			/* Throw an exception on nullptr. */
			if(data == nullptr)
				return false;

			return *data == dataIn;
		}


		/** Equivilent operator.
		 *
		 *  @param[in] a The data type to compare to.
		 *
		 **/
		bool operator==(const TypeName* ptr) const
		{
			RLOCK(MUTEX);

			return data == ptr;
		}


		/** Not equivilent operator.
		 *
		 *  @param[in] a The data type to compare to.
		 *
		 **/
		bool operator!=(const TypeName* ptr) const
		{
			RLOCK(MUTEX);

			return data != ptr;
		}

		/** Not operator
		 *
		 *  Check if the pointer is nullptr.
		 *
		 **/
		bool operator!(void)
		{
			RLOCK(MUTEX);

			return data == nullptr;
		}


		/** Member access overload
		 *
		 *  Allow locked_ptr access like a normal pointer.
		 *
		 **/
		lock_proxy<TypeName> operator->()
		{
			MUTEX.lock();

			return lock_proxy<TypeName>(data, MUTEX);
		}


		/** dereference operator overload
		 *
		 *  Load the object from memory.
		 *
		 **/
		TypeName operator*() const
		{
			RLOCK(MUTEX);

			/* Throw an exception on nullptr. */
			if(data == nullptr)
				throw std::runtime_error(debug::safe_printstr(FUNCTION, "dereferencing a nullptr"));

			return *data;
		}


		/** store
		 *
		 *  Stores an object into memory.
		 *
		 *  @param[in] dataIn The data into protected memory.
		 *
		 **/
		void store(TypeName* dataIn)
		{
			RLOCK(MUTEX);

			if(data)
				delete data;

			data = dataIn;
		}


		/** free
		 *
		 *  Free the internal memory of the encrypted pointer.
		 *
		 **/
		void free()
		{
			RLOCK(MUTEX);

			if(data)
				delete data;

			data = nullptr;
		}
	};
}
