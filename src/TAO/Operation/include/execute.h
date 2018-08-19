/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

namespace TAO
{

    namespace Operation
    {

        class COperation
        {
            std::vector<uint8_t> vchOperations;

            //counter to track what operation code we are on
            uint32_t nIterator = 0;

            COperation(std::vector<uint8_t> vchOperationsIn) : vchOperations(vchOperationsIn) {}

            bool Execute(LLD::RegisterDB& regDB, uint256_t hashOwner)
            {
                CDataStream ssData(vchOperations.begin(), vchOperations.end(), SER_LLD);

                switch(vchOperations[nIterator])
                {
                    case OP_WRITE:

                        /* Get the Address of the Register. */
                        uint256_t hashAddress;
                        ssData >> hashAddress;

                        /* Read the binary data of the Register. */
                        CStateRegister regState;
                        if(!regDB.Read(hashAddress, regState))
                            return error("Operation::OP_WRITE : Register Address doewn't exist %s", hashAddress.ToString().c_str());

                        /* Check ReadOnly permissions. */
                        if(regState.fReadOnly)
                            return error("Operation::OP_WRITE : Write operation called on read-only register");

                        /* Check that the proper owner is commiting the write. */
                        if(hashOwner != regState.hashOwner)
                            return error("Operation::OP_WRITE : No write permissions for owner %s", hashOwner.ToString().c_str());

                        /* Get the Data of the Register. */
                        CData regData;
                        ssData >> regData;

                        /* Check the new data size against register's allocated size. */
                        if(regState.nLength != 0 && regData.bytes().size() != regState.nLength)
                            return error("Operation::OP_WRITE : New Register State size %u mismatch %u", regData.bytes().size(), regState.nLength);

                        regState.SetState(regData.bytes());

                        if(!regDB.Write(hashAddress, regState))
                            return false;

                        break;

                    case OP_READ:

                        uint256_t hashAddress;
                        ssData >> hashAddress;

                        CStateRegister regState;
                        if(!regDB.Read(hashAddress, regState))
                            return error("Operation::OP_READ : Register Address %s not found", hashAddress.ToString().c_str());

                        //register stake push? or vchRegister data held locally?

                        break;

                    case OP_REGISTER:

                        CData regData;
                        ssData >> regData;

                        /* Check that the register doesn't exist yet. */
                        uint256_t hashAddress = regData.GetHash(); //the address of a register is the hash of the first state
                        if(regDB.Exists(hashAddress))
                            return error("Operation::OP_REGISTER : Cannot allocate register of same memory address %s", hashAddress.ToString().c_str());

                        /* Set the register's state */
                        CStateRegister regState = CStateRegister();
                        regState.SetState(regData.bytes());
                        regState.hashOwner = hashOwner;
                        regState.SetAddress(hashAddress);

                        if(!regDB.Write(hashAddress, regState))
                            return error("Operation::OP_REGISTER : Failed to write state register %s into register DB", hashAddress.ToString().c_str());

                        break;

                    case OP_AUTHORIZE:

                        uint256_t hashToken;
                        ssData >> hashToken;

                        //need to have a database of authorized tokens by ID
                        //to obtain authorization you need to request to another node for access to a said System
                        //they will then post the hashToken to their signature Chain
                        //once cleared this needs to relay backa as a push message to the API with the hashToken
                        //on authorized internal system they need to make API request to verify that the token is vaid on the ledger

                        break;

                    case OP_TRANSFER:

                        uint256_t hashAddress;
                        ssData >> hashAddress;

                        uint256_t hashNewOwner;
                        ssData >> hashNewOwner;

                        CStateRegister regState;
                        if(!regDB.Read(hashAddress, regState))
                            return error("Operation::OP_TRANSFER : Invalid register addrewss in register DB");

                        if(regState.hashOwner != hashOwner)
                            return error("Operation::OP_TRANSFER : %s not authorized to transfer register", hashOwner.ToString().c_str());

                        regState.hashOwner = hashNewOwner;
                        if(!regDB.Write(hashAddress, regState))
                            return error("Opeation::OP_TRANSAFER : Failed to write new owner for register");

                        break;


                    //skip for now
                    case OP_IF:

                        break;

                    case OP_ELSE:

                        break;

                    case OP_ENDIF:

                        break;

                    case OP_NOT:

                        break;

                    case OP_EQUALS:

                        break;


                    case OP_ACCOUNT:

                        break;

                    case OP_CREDIT:

                        break;

                    case OP_DEBIT:

                        break;

                    case OP_BALANCE:

                        break;

                    case OP_EXPIRE:

                        break;

                    case OP_CREATE:

                        break;

                }
            }
        }
    }
}
