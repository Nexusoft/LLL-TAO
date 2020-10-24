/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <cstdint>
#include <string>

#include <Util/include/config.h>

namespace LLD::Config
{
    /** Structure to contain the configuration variables for a generic database object. **/
    class Base
    {
    public:

        /** The string to hold the database location. **/
        std::string DIRECTORY;


        /** The string to hold the database name. **/
        std::string NAME;


        /** The keychain level flags. **/
        uint8_t FLAGS;


        /** No default constructor. **/
        Base() = delete;


        /** Copy Constructor. **/
        Base(const Base& map)
        : DIRECTORY               (map.DIRECTORY)
        , NAME                    (map.NAME)
        , FLAGS                   (map.FLAGS)
        {
        }


        /** Move Constructor. **/
        Base(Base&& map)
        : DIRECTORY               (std::move(map.DIRECTORY))
        , NAME                    (std::move(map.NAME))
        , FLAGS                   (std::move(map.FLAGS))
        {
        }


        /** Copy Assignment **/
        Base& operator=(const Base& map)
        {
            DIRECTORY               = map.DIRECTORY;
            NAME                    = map.NAME;
            FLAGS                   = map.FLAGS;

            return *this;
        }


        /** Move Assignment **/
        Base& operator=(Base&& map)
        {
            DIRECTORY               = std::move(map.DIRECTORY);
            NAME                    = std::move(map.NAME);
            FLAGS                   = std::move(map.FLAGS);

            return *this;
        }


        /** Destructor. **/
        ~Base()
        {
        }


        /** Required Constructor. **/
        Base(const std::string& strName, const uint8_t nFlags)
        : DIRECTORY               (config::GetDataDir() + strName + "/")
        , NAME                    (strName)
        , FLAGS                   (nFlags)
        {
        }

    protected:

        /** check_limits
         *
         *  Checks the limits on a specific data type and resets it's value to maximum if out of range.
         *
         *  @param[in] strName The name of the variable.
         *  @param[out] nType The type range is being checked for.
         *
         **/
        template<typename Limits, typename TypeName>
        void check_limits(const std::string& strName, TypeName &nType)
        {
            /* Check our ranges. */
            if(nType > std::numeric_limits<Limits>::max())
            {
                /* Adjust range to maximum. */
                nType = std::numeric_limits<Limits>::max();
                debug::warning(FUNCTION, strName, " exceeded limits, adjusted to: ", nType);
            }
        }


        /** check_ranges
         *
         *  Checks the range on a specific data type and resets it's value to limits
         *
         *  @param[in] strName The name of the variable.
         *  @param[out] nType The type range is being checked for.
         *  @param[in] nMaximum The maximum range of the value
         *
         **/
        template<typename Limits, typename TypeName>
        void check_ranges(const std::string& strName, TypeName &nType, const Limits& nMaximum)
        {
            /* Check our ranges. */
            if(nType > nMaximum)
            {
                /* Adjust range to maximum. */
                nType = nMaximum;
                debug::warning(FUNCTION, strName, " exceeded range, adjusted to: ", nType);
            }
        }
    };
}
