/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_OBJECT_H
#define NEXUS_TAO_REGISTER_INCLUDE_OBJECT_H

#include <TAO/Register/include/state.h>
#include <TAO/Operation/include/enum.h>

#include <LLP/include/version.h> //stop using protocol version so much here

#include <Util/templates/serialize.h>

namespace TAO
{

    namespace Register
    {

        template<typename TypeObject> class Object : public State
        {
        public:

            uint32_t nStateEnd; //the binary end of the state bytes

            std::map<uint64_t, std::vector<uint8_t> > mapMethods; //the byte data of the mothods

            Object(TypeObject classObjectIn, std::vector<uint8_t> vchOperations)
            {
                CDataStream ssObject(SER_NETWORK, LLP::PROTOCOL_VERSION);
                ssObject << classObjectIn;

                std::vector<uint8_t> vState(ssObject.begin(), ssObject.end());
                vState.insert(vState.end(), vchOperations.begin(), vchOperations.end());

                SetState(vState);
            }

            Object(TypeObject classObjectIn)
            {
                CDataStream ssObject(SER_NETWORK, LLP::PROTOCOL_VERSION);
                ssObject << classObjectIn;

                std::vector<uint8_t> vState(ssObject.begin(), ssObject.end());
                SetState(vState);
            }

            std::vector< uint64_t > GetAddresses() //get method addresses
            {
                std::vector<uint64_t> vnAddresses;

                for(auto methods : mapMethods)
                    vnAddresses.push_back(methods.first);

                return vnAddresses;
            }

            TypeObject GetObject()
            {
                TypeObject classObject;

                CDataStream ssObject(vchState, SER_NETWORK, LLP::PROTOCOL_VERSION);
                ssObject >> classObject;

                return classObject;
            }


            void ParseMethods()
            {
                std::vector<uint8_t> vchOperations(vchState.begin() + nStateEnd, vchState.end());

                //really bad algorithm I know... TODO log(n) rather than O(n) to parse methods. Keep iterator pointer for method point or make a method class with a length field in it
                std::vector<uint8_t> vchMethod;
                for(auto chOP : vchOperations)
                {
                    if(chOP == TAO::Operation::OP_METHOD)
                    {
                        //get the address of the method out of the data. uint32
                        //state address method call is hashAddress->uint32

                        //vchMethods.push_back(vchMethod);
                        vchMethod.clear();

                        continue;
                    }

                    vchMethod.push_back(chOP);
                }
            }


            template<typename TypeReturn>
            TypeReturn Execute(uint64_t nAddress, std::vector<uint8_t> vchParameters)
            {
                TypeReturn objReturn;
                objReturn.SetNull(); //all return types must have a virtual TypeReturn

                if(!mapMethods.count(nAddress))
                    return objReturn; //return a null value if the method address was not found

                std::vector<uint8_t> vchMethod = mapMethods[nAddress];
            }
        };

    }
}

#endif
