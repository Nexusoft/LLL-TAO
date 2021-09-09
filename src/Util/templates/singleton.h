/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <atomic>

#include <Util/include/debug.h>

/** @class
 *
 *  This class is a base class for any object that wants to use a singleton access pattern.
 *
 **/
template<class Type>
class Singleton
{
protected:

    /** Static instance pointer. **/
    static std::atomic<Type*> INSTANCE;

public:

    /** Default Constructor. **/
    Singleton() {}


    /** Default Destructor. **/
    ~Singleton(){}


    /** Singleton instance. **/
    static Type& Instance()
    {
        /* Check if instance needs to be initialized. */
        if(Type::INSTANCE.load() == nullptr)
            throw debug::exception(FUNCTION, "instance not initialized");

        return *INSTANCE.load();
    }


    /** Singleton initialize instance. **/
    static void Initialize()
    {
        /* Check if instance needs to be initialized. */
        if(Type::INSTANCE.load() == nullptr)
            Type::INSTANCE.store(new Type());
    }


    /** Singleton shutdown instance. **/
    static void Shutdown()
    {
        /* Check if instance needs to be initialized. */
        if(Type::INSTANCE.load() != nullptr)
        {
            /* Cleanup our pointers. */
            delete Type::INSTANCE.load();

            Type::INSTANCE.store(nullptr);
        }
    }
};

/* Initialize our static instance. */
template<class Type>
std::atomic<Type*> Singleton<Type>::INSTANCE;
