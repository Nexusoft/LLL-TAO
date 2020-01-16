/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        
        /** Cast
        *
        *  Converts an object register to a new object register instance from a base class type.  The method will compare the
        *  fields in object to be converted to the requested type and generate a new object containing all matching fields.  The 
        *  cast will fail if any of the fields in the base type are missing, have a different type, or are in the wrong order in 
        *  the object.  If the cast fails then an object set to Null will be returned 
        *
        *  @param[in] object The object to be converted
        *  @param[in] type The base class type to use for the cast
        *
        *  @return Object containing all of the fields in the base type but all of the data from the object to be converted 
        *
        **/
        TAO::Register::Object Cast(const TAO::Register::Object& object, const TAO::Register::Object& type);

        
        /** Matches
        *
        *  The method will compare the fields in object to be checked to those in the desired type.  If all of the fields in type
        *  exist in the object, are of the same type, and are in the matching order, then the method returns true.  
        *  NOTE: the data in the fields is not checked - only the existence of matching fields, type, length, and order   
        *
        *  @param[in] object The object to be checked
        *  @param[in] type The base class type to compare to
        *
        *  @return True if all of the fields in type exist in the object, are of the same type, and are in the matching order 
        *
        **/
        bool Matches(const TAO::Register::Object& object, const TAO::Register::Object& type);


    }
}

