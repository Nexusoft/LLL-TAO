/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/object_utils.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/types/stream.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/include/unpack.h>

#include <Util/include/debug.h>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Converts an object register to a new object register instance from a base class type.  The method will compare the
        *  fields in object to be converted to the requested type and generate a new object containing all matching fields.  The 
        *  cast will fail if any of the fields in the base type are missing, have a different type, or are in the wrong order in 
        *  the object.  If the cast fails then an object set to Null will be returned */
        TAO::Register::Object Cast(const TAO::Register::Object& object, const TAO::Register::Object& type)
        {
            /* The cast object to return */
            TAO::Register::Object cast;

            /* Flag to track of the cast is successful */
            bool fMatches = true;

            /* The fields in the type object to check */
            std::vector<std::string> vFields = type.GetFieldNames();

            /* The field type */
            uint8_t nType;

            /* Mutable flag for the field */
            bool fMutable;

            /* Check the object against each field in the type to be cast to, to see if it matches the type. */
            for(const auto& strField: vFields)
            {
                /* Get the the type. */
                type.Type(strField, nType);

                /* Get the mutable flag. */ 
                type.Mutable(strField, fMutable);

                /* Check to see if the field exists in the target object */
                if(!object.Check(strField, nType, fMutable))
                {
                    fMatches = false;
                    break;
                }

                /* If the field matches then add the field and its data from object to the newly cast type */
                
                /* Add the field name */
                cast << strField;

                /* Add mutable flag */
                if(fMutable) 
                    cast << uint8_t(TAO::Register::TYPES::MUTABLE);

                /* Add the field type */
                cast << nType;

                /* Finally add the field data based on field type*/
                switch(nType)
                {
                    case TAO::Register::TYPES::UINT8_T:
                        cast << object.get<uint8_t>(strField);
                        break;

                    case TAO::Register::TYPES::UINT16_T:
                        cast << object.get<uint16_t>(strField);
                        break;

                    case TAO::Register::TYPES::UINT32_T:
                        cast << object.get<uint32_t>(strField);
                        break;

                    case TAO::Register::TYPES::UINT64_T:
                        cast << object.get<uint64_t>(strField);
                        break;

                    case TAO::Register::TYPES::UINT256_T:
                        cast << object.get<uint256_t>(strField);
                        break;

                    case TAO::Register::TYPES::UINT512_T:
                        cast << object.get<uint512_t>(strField);
                        break;

                    case TAO::Register::TYPES::UINT1024_T:
                        cast << object.get<uint1024_t>(strField);
                        break;

                    case TAO::Register::TYPES::STRING:
                        cast << object.get<std::string>(strField);
                        break;

                    case TAO::Register::TYPES::BYTES:
                        cast << object.get<std::vector<uint8_t>>(strField);
                        break;
                    
                    default:
                        fMatches = false;
                }
            }

            /* If the cast was not successful then return an empty (null) object */
            if(!fMatches)
                cast.SetNull();

            /* ensure the return object is parsed */
            cast.Parse();

            return cast;
        }

        
        /* The method will compare the fields in object to be checked to those in the desired type.  If all of the fields in type
        *  exist in the object, are of the same type, and are in the matching order, then the method returns true.  
        *  NOTE: the data in the fields is not checked - only the existence of matching fields, type, length, and order */
        bool Matches(const TAO::Register::Object& object, const TAO::Register::Object& type)
        {
            /* The return value flag */
            bool fMatches = true;

            /* The fields in the type object to check */
            std::vector<std::string> vFields = type.GetFieldNames();

            /* The field type */
            uint8_t nType;

            /* Mutable flag for the field */
            bool fMutable;

            /* Check each field in the type. */
            for(const auto& strField: vFields)
            {

                /* Get the the type. */
                type.Type(strField, nType);

                /* Get the mutable flag. */ 
                type.Mutable(strField, fMutable);
                
                /* Check to see if the field exists in the target object */
                if(!object.Check(strField, nType, fMutable))
                {
                    fMatches = false;
                    break;
                }
            }

            return fMatches;
        }


    } // end API namespace

} // end TAO namespace