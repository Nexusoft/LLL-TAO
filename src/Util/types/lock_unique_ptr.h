/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <mutex>
#include <inttypes.h>

#include <Util/include/debug.h>

namespace util::atomic
{
	/** lock_unique_ptr
     *
     *  Protects a pointer with a mutex.
     *
     **/
    template<class TypeName>
    class lock_unique_ptr
    {
        /** The internal locking mutex. **/
        mutable std::recursive_mutex MUTEX;


        /** The internal raw poitner. **/
        TypeName* pData;


		/** lock_proxy
		 *
		 *  Temporary class that unlocks a mutex when outside of scope.
		 *  Useful for protecting member access to a raw pointer.
		 *
		 **/
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
			lock_proxy(TypeName* pData, std::recursive_mutex& MUTEX_IN)
			: MUTEX(MUTEX_IN)
			, pData(pData)
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
				if(pData == nullptr)
					throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));

				return pData;
			}
		};


    public:

        /** Default Constructor. **/
        lock_unique_ptr()
        : MUTEX()
        , pData(nullptr)
        {
        }


        /** Constructor for storing. **/
        lock_unique_ptr(TypeName* pDataIn)
        : MUTEX()
        , pData(pDataIn)
        {
        }


        /** Copy Constructor. **/
        lock_unique_ptr(const lock_unique_ptr<TypeName>& pointer) = delete;


        /** Move Constructor. **/
        lock_unique_ptr(const lock_unique_ptr<TypeName>&& pointer)
        : pData(pointer.pData)
        {
        }


        /** Destructor. **/
        ~lock_unique_ptr()
        {
			/* Free the memory. */
			free();
        }


        /** Assignment operator. **/
        lock_unique_ptr& operator=(const lock_unique_ptr<TypeName>& pDataIn) = delete;


		/** Move Assignment operator. **/
		lock_unique_ptr& operator=(lock_unique_ptr<TypeName>&& pDataIn)
		{
			/* Store the new value. */
			store(pDataIn.pData);

			/* Reset our old pointer to ensure it doesn't get deleted. */
			{
				RECURSIVE(pDataIn.MUTEX);

				pDataIn.pData = nullptr;
			}

			return *this;
		}


        /** Assignment operator. **/
        lock_unique_ptr& operator=(TypeName* pDataIn) = delete;


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName& pDataIn) const
        {
            RECURSIVE(MUTEX);

            /* Throw an exception on nullptr. */
            if(pData == nullptr)
                return false;

            return *pData == pDataIn;
        }


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName* ptr) const
        {
            RECURSIVE(MUTEX);

            return pData == ptr;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator!=(const TypeName* ptr) const
        {
            RECURSIVE(MUTEX);

            return pData != ptr;
        }

        /** Not operator
         *
         *  Check if the pointer is nullptr.
         *
         **/
        bool operator!(void)
        {
            RECURSIVE(MUTEX);

            return pData == nullptr;
        }


        /** Member access overload
         *
         *  Allow lock_unique_ptr access like a normal pointer.
         *
         **/
        lock_proxy operator->()
        {
            MUTEX.lock();

            return lock_proxy(pData, MUTEX);
        }


        /** dereference operator overload
         *
         *  Load the object from memory.
         *
         **/
        TypeName operator*() const
        {
            RECURSIVE(MUTEX);

            /* Throw an exception on nullptr. */
            if(pData == nullptr)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "dereferencing a nullptr"));

            return *pData;
        }


		/** free
		 *
		 *  Free the internal memory of the encrypted pointer.
		 *
		 **/
		void free()
		{
			RECURSIVE(MUTEX);

			if(pData)
				delete pData;

			pData = nullptr;
		}


	private:

        /** store
         *
         *  Stores an object into memory.
         *
         *  @param[in] pDataIn The data into protected memory.
         *
         **/
        void store(TypeName* pDataIn)
        {
            RECURSIVE(MUTEX);

            if(pData)
                delete pData;

            pData = pDataIn;
        }
    };
}
