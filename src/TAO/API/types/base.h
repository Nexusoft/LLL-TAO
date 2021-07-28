/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/function.h>
#include <TAO/API/types/standard.h>
#include <TAO/API/types/operator.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/types/contract.h>

/* Forward declarations. */
namespace TAO::Register { class Object; }

/* Global TAO namespace. */
namespace TAO::API
{
    /** Base
     *
     *  Base class for all JSON based API's.
     *
     *  Instances of JSONAPIBase derivations must be registered with the
     *  JSONAPINode processing the HTTP requests. This class holds a map of
     *  JSONAPIMethod instances that in turn perform the processing for each
     *  API method request.
     *
     **/
    class Base
    {
        /* Simple enum for readability. */
        enum URI : uint8_t
        {
            VERB     = 0,
            NOUN     = 1,
            FIELD    = 2,
            OPERATOR = 3,
        };

    protected:

        /** Initializer Flag. */
        bool fInitialized;


        /** Map of method names to method function pointer objects for each method supported by this API. **/
        std::map<std::string, Function> mapFunctions;


        /** Map of standard nouns to check for standard object types. **/
        std::map<std::string, Standard>  mapStandards;


        /** Map of standard nouns to check for standard object types. **/
        std::map<std::string, Operator>  mapOperators;


    public:

        /** Default Constructor **/
        Base ( )
        : fInitialized (false)
        , mapFunctions ( )
        , mapStandards ( )
        , mapOperators ( )
        {
        }


        /** Default destructor **/
        virtual ~Base();


        /** Initialize
         *
         *  Abstract initializor that all derived API's must implement to
         *  register their specific APICommands.
         *
         **/
        virtual void Initialize() = 0;


        /** Get
         *
         *  Abstract initializer so we don't need to copy this method for each derived class.
         *
         **/
        virtual Base* Get() = 0;


        /** Status
         *
         *  Get the current status of a given command.
         *
         *  @param[in] strMethod The method we are checking for.
         *
         *  @return a string with status message.
         *
         **/
        std::string Status(const std::string& strMethod) const;


        /** CheckObject
         *
         *  Checks an object's type if it has been standardized for this command-set.
         *
         *  @param[in] strType The object standard name we are checking for.
         *  @param[in] tObject The object to check standard against.
         *
         *  @return true if standard exists and matches, false otherwise.
         *
         **/
        bool CheckObject(const std::string& strType, const TAO::Register::Object& tObject) const;


        /** EncodeObject
         *
         *  Encode a standard object into json using custom encoding function.
         *
         *  @param[in] strType The object type we are encoding.
         *  @param[in] tObject The object we are encoding for.
         *  @param[in] hashRegister The register's address we are encoding for.
         *
         *  @return the json encoded object
         *
         **/
        encoding::json EncodeObject(const std::string& strType, const TAO::Register::Object& tObject, const uint256_t& hashRegister) const;


        /** Execute
         *
         *  Handles the processing of the requested method.
         *  Derivations should implement this to lookup the requested method in the mapFunctions map and pass the processing on.
         *  Derivations should also form the response JSON according to the API specification
         *
         *  @param[out] strMethod The requested API method.
         *  @param[out] jParams The parameters that the caller has passed to the API request.
         *  @param[in] fHelp Flag to determine if command help is requested.
         *
         *  @return JSON encoded response.
         *
         **/
        encoding::json Execute(std::string &strMethod, encoding::json &jParams, const bool fHelp = false);


        /** RewriteURL
         *
         *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  This will break the URI into components
         *  using such to generate parameters for types, fieldnames, and operators.
         *
         *  @param[in] strMethod The name of the method being invoked.
         *  @param[in] jParams The json array of parameters being passed to this method.
         *
         *  @return the API method URL
         *
         **/
        virtual std::string RewriteURL(const std::string& strMethod, encoding::json &jParams);


        /** BuildIndexes
         *
         *  Generic handler for creating new indexes for this specific command-set.
         *  This handler takes a contract and generates indexes according to command's logic.
         *
         *  @param[in] rContract The contract we are building indexes for.
         *
         **/
        virtual void BuildIndexes(const TAO::Operation::Contract& rContract) { }
    };


    /** Derived
     *
     *  Class for forcing the polymorphic get so we can cast to and from child and parent classes.
     *  This follows similar logic as polymorphic clone, but is indented to be used without copying.
     *
     **/
    template<class Type>
    class Derived : public Base
    {
    public:

        /** Get
         *
         *  Method to be used by commands class, to allow casting to and from parent and child classes.
         *
         **/
        Base* Get() final override
        {
            return static_cast<Type*>(this);
        }
    };
}
