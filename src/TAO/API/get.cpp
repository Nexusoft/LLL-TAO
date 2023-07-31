/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/notifications.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLD/include/global.h>

#include <Util/include/math.h>
#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Converts the decimals from an object into raw figures using power function */
    uint64_t GetFigures(const TAO::Register::Object& object)
    {
        return math::pow(10, GetDecimals(object));
    }


    /* Converts the decimals from an object into raw figures using power function */
    uint64_t GetFigures(const uint256_t& hashToken)
    {
        /* Check for NXS as a value. */
        if(hashToken == 0)
            return TAO::Ledger::NXS_COIN;

        /* Otherwise let's lookup our token object. */
        TAO::Register::Object tToken;
        if(!LLD::Register->ReadObject(hashToken, tToken, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-13, FUNCTION, "Object not found");

        /* Let's check that a token was passed in. */
        if(tToken.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw Exception(-15, FUNCTION, "Object is not a token");

        return math::pow(10, tToken.get<uint8_t>("decimals"));
    }


    /* Get a precision value based on given balance value and token type.*/
    precision_t GetPrecision(const uint64_t nBalance, const uint256_t& hashToken)
    {
        /* Check for NXS as a value. */
        if(hashToken == 0)
            return precision_t(nBalance, TAO::Ledger::NXS_DIGITS);

        /* Otherwise let's lookup our token object. */
        TAO::Register::Object oToken;
        if(!LLD::Register->ReadObject(hashToken, oToken, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-13, FUNCTION, "Object not found");

        /* Let's check that a token was passed in. */
        if(oToken.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw Exception(-15, FUNCTION, "Object is not a token");

        /* Build our return value. */
        precision_t dRet =
            precision_t(oToken.get<uint8_t>("decimals"));

        /* Set our value internally. */
        dRet.nValue = nBalance;

        return dRet;
    }


    /* Converts the decimals from an object into raw figures using power function */
    uint64_t GetDecimals(const uint256_t& hashToken)
    {
        /* Check for NXS as a value. */
        if(hashToken == 0)
            return TAO::Ledger::NXS_DIGITS;

        /* Otherwise let's lookup our token object. */
        TAO::Register::Object tToken;
        if(!LLD::Register->ReadObject(hashToken, tToken, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-13, FUNCTION, "Object not found");

        /* Let's check that a token was passed in. */
        if(tToken.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw Exception(-15, FUNCTION, "Object is not a token");

        return tToken.get<uint8_t>("decimals");
    }


    /* Retrieves the number of decimals that applies to amounts for this token or account object.
     * If the object register passed in is a token account then we need to look at the token definition
     */
    uint8_t GetDecimals(const TAO::Register::Object& object)
    {
        /* Check the object standard. */
        switch(object.Standard())
        {
            /* Standard token contracts we can grab right from the object. */
            case TAO::Register::OBJECTS::TOKEN:
                return object.get<uint8_t>("decimals");

            /* Trust standards use NXS default values. */
            case TAO::Register::OBJECTS::TRUST:
                return TAO::Ledger::NXS_DIGITS;

            /* Account standards need to do a little looking up. */
            case TAO::Register::OBJECTS::ACCOUNT:
            {
                /* Cache our identifier. */
                const uint256_t hashIdentifier = object.get<uint256_t>("token");

                /* NXS has identifier 0, so no look up needed */
                if(hashIdentifier == 0)
                    return TAO::Ledger::NXS_DIGITS;

                /* Read the token register from the DB. */
                TAO::Register::Object object;
                if(!LLD::Register->ReadObject(hashIdentifier, object, TAO::Ledger::FLAGS::LOOKUP))
                    throw Exception(-125, FUNCTION, "Token not found");

                return object.get<uint8_t>("decimals");
            }

            default:
                throw Exception(-36, "Unsupported type [", GetStandardName(object.Standard()), "] for command");
        }

        return 0;
    }


    /* Get the type standardized into object if applicable. */
    uint16_t GetStandardType(const TAO::Register::Object& rObject)
    {
        /* Check for a regular object type. */
        if(rObject.nType == TAO::Register::REGISTER::OBJECT)
            return 0;

        /* Reset read position. */
        rObject.nReadPos = 0;

        /* Find our leading type byte. */
        uint16_t nType;
        rObject >> nType;

        /* Check for valid usertype. */
        if(!USER_TYPES::Valid(nType))
            return 0;

        /* Cleanup our read position. */
        rObject.nReadPos = 0;

        return nType;
    }


    /* Get the sum of all debit notifications for the the specified token */
    uint64_t GetUnclaimed(const uint256_t& hashGenesis, const uint256_t& hashToken, const uint256_t& hashAccount)
    {
        /* Check if we are available to generate our balances. */
        if(!Authentication::Available(hashGenesis))
            return 0;

        /* Th return value */
        uint64_t nUnclaimed = 0;

        /* Get a list of our active events. */
        std::vector<std::pair<uint512_t, uint32_t>> vEvents;

        /* Get our list of active contracts we have issued. */
        LLD::Logical->ListEvents(hashGenesis, vEvents);

        /* For sanitizing our contracts. */
        std::map<uint256_t, TAO::Register::State> mapStates;
        for(const auto& rEvent : vEvents)
        {
            /* Grab a reference of our hash. */
            const uint512_t& hashEvent = rEvent.first;

            /* Check for Tritium transaction. */
            if(hashEvent.GetType() == TAO::Ledger::TRITIUM)
            {
                /* Get the transaction from disk. */
                TAO::API::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashEvent, tx))
                    continue;

                /* Check if contract has been burned. */
                if(tx.Burned(hashEvent, rEvent.second))
                    continue;

                /* Check if the transaction is mature. */
                if(!tx.Mature(hashEvent))
                    continue;
            }

            /* Get a reference of our contract. */
            const TAO::Operation::Contract& rContract =
                LLD::Ledger->ReadContract(hashEvent, rEvent.second, TAO::Ledger::FLAGS::BLOCK);

            /* Check if the given contract is spent already. */
            if(rContract.Spent(rEvent.second))
                continue;

            /* Seek our contract to primitive OP. */
            rContract.SeekToPrimitive();

            /* Get a copy of our primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;

            /* Switch for valid primitives. */
            switch(nOP)
            {
                /* Handle for if we need to credit. */
                case TAO::Operation::OP::LEGACY:
                case TAO::Operation::OP::DEBIT:
                case TAO::Operation::OP::COINBASE:
                {
                    try
                    {
                        /* Get our source address. */
                        uint256_t hashAddress;
                        rContract >> hashAddress;

                        /* Check our account filters first. */
                        if(hashAccount != 0 && hashAccount != hashAddress)
                            continue;

                        /* Build our credit contract now. */
                        uint64_t nAmount = 0;
                        if(TAO::Register::Unpack(rContract, nAmount))
                        {
                            /* Regular check for non coinbase. */
                            if(nOP != TAO::Operation::OP::COINBASE)
                            {
                                /* Check our pre-state for token types. */
                                TAO::Register::Object oAccount =
                                    rContract.PreState();

                                /* Check for null value. */
                                if(oAccount.IsNull())
                                    continue;

                                /* Parse account now. */
                                oAccount.Parse();

                                /* Check for valid token types. */
                                if(oAccount.get<uint256_t>("token") != hashToken)
                                    continue;
                            }
                            else if(hashToken != TOKEN::NXS)
                                continue;

                            /* Increment our pending balance now. */
                            nUnclaimed += nAmount;
                        }

                    }
                    catch(const Exception& e)
                    {
                        continue;
                    }

                    break;
                }

                /* Unknown ops we want to continue looping. */
                default:
                {
                    continue;
                }
            }
        }

        /* Get all transactions in the mempool */
        std::vector<uint512_t> vMempool;
        if(TAO::Ledger::mempool.List(vMempool))
        {
            /* Loop through the list of transactions. */
            for(const auto& hash : vMempool)
            {
                /* Get the transaction from the memory pool. */
                TAO::Ledger::Transaction tx;
                if(!TAO::Ledger::mempool.Get(hash, tx))
                    continue;

                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Get a reference of our contract. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    /* Reset the op stream */
                    rContract.Reset();

                    /* The operation */
                    uint8_t nOp;
                    rContract >> nOp;

                    /* Check for that the debit is meant for us. */
                    if(nOp == TAO::Operation::OP::DEBIT)
                    {
                        /* Get the source address which is the proof for the debit */
                        TAO::Register::Address hashFrom;
                        rContract >> hashFrom;

                        /* Check we made the transaction */
                        if(tx.hashGenesis == hashGenesis)
                            continue;

                        /* Check the account filter based on the originating account*/
                        if(hashAccount != 0 && hashAccount != hashFrom)
                            continue;

                        /* Skip over our hashTo for OP::DEBIT. */
                        rContract.Seek(32);

                        /* Retrieve the account. */
                        TAO::Register::Object oFrom;
                        if(!LLD::Register->ReadObject(hashFrom, oFrom, TAO::Ledger::FLAGS::LOOKUP))
                            continue;

                        /* Check that this is an account */
                        if(oFrom.Base() != TAO::Register::OBJECTS::ACCOUNT)
                            continue;

                        /* Check the account token matches the one passed in*/
                        if(hashToken != oFrom.get<uint256_t>("token"))
                            continue;

                        /* Get the amount */
                        uint64_t nAmount = 0;
                        rContract >> nAmount;

                        /* Add this amount to our total */
                        nUnclaimed += nAmount;
                    }
                }
            }
        }

        return nUnclaimed;
    }


    /* Get the sum of all debit transactions in the mempool for the the specified token */
    uint64_t GetUnconfirmed(const uint256_t& hashGenesis, const uint256_t& hashToken, bool fOutgoing, const uint256_t& hashAccount)
    {
        /* Check if we are available to generate our balances. */
        if(!Authentication::Available(hashGenesis))
            return 0;

        /* The return value */
        uint64_t nUnconfirmed = 0;

        /* Get all transactions in the mempool */
        std::vector<uint512_t> vMempool;
        if(TAO::Ledger::mempool.List(vMempool))
        {
            /* Loop through the list of transactions. */
            for(const auto& hash : vMempool)
            {
                /* Get the transaction from the memory pool. */
                TAO::Ledger::Transaction tx;
                if(!TAO::Ledger::mempool.Get(hash, tx))
                    continue;

                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Get a reference of our contract. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    /* Reset the op stream */
                    rContract.Reset();

                    /* The operation */
                    uint8_t nOp;
                    rContract >> nOp;

                    /* Check for outgoing unconfirmed balances. */
                    if(fOutgoing)
                    {
                        /* Check for that the debit is meant for us. */
                        if(nOp == TAO::Operation::OP::DEBIT || nOp == TAO::Operation::OP::LEGACY)
                        {
                            /* Get the source address which is the proof for the debit */
                            TAO::Register::Address hashFrom;
                            rContract >> hashFrom;

                            /* Check we made the transaction */
                            if(tx.hashGenesis != hashGenesis)
                                continue;

                            /* Check the account filter based on the originating account*/
                            if(hashAccount != 0 && hashAccount != hashFrom)
                                continue;

                            /* Special handle for DEBIT. */
                            if(nOp == TAO::Operation::OP::DEBIT)
                            {
                                /* Skip over our hashTo for OP::DEBIT. */
                                rContract.Seek(32);

                                /* Retrieve the account. */
                                TAO::Register::Object oFrom;
                                if(!LLD::Register->ReadObject(hashFrom, oFrom, TAO::Ledger::FLAGS::MEMPOOL))
                                    continue;

                                /* Check that this is an account */
                                if(oFrom.Base() != TAO::Register::OBJECTS::ACCOUNT)
                                    continue;

                                /* Check the account token matches the one passed in*/
                                if(hashToken != oFrom.get<uint256_t>("token"))
                                    continue;
                            }

                            /* Get the amount */
                            uint64_t nAmount = 0;
                            rContract >> nAmount;

                            /* Add this amount to our total */
                            nUnconfirmed += nAmount;
                        }
                    }

                    /* If checking inbound unconfirmed, we need to watch for our own CREDITS. */
                    else if(nOp == TAO::Operation::OP::CREDIT)
                    {
                        /* For credits first make sure we made the transaction */
                        if(tx.hashGenesis != hashGenesis)
                            continue;

                        /* Skip over irrelevant data. */
                        rContract.Seek(68);

                        /* Get the account address. */
                        TAO::Register::Address hashTo;
                        rContract >> hashTo;

                        /* Skip over proof data. */
                        rContract.Seek(32);

                        /* Get the credit amount. */
                        uint64_t nCredit = 0;
                        rContract >> nCredit;

                        /* Check the account filter */
                        if(hashAccount != 0 && hashAccount != hashTo)
                            continue;

                        /* Retrieve the account. */
                        TAO::Register::Object oAccount;
                        if(!LLD::Register->ReadObject(hashTo, oAccount, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Check owner that we are the owner of the recipient account  */
                        if(oAccount.hashOwner != hashGenesis)
                            continue;

                        /* Check that this is an account */
                        if(oAccount.Base() != TAO::Register::OBJECTS::ACCOUNT)
                            continue;

                        /* Check the account token matches the one passed in*/
                        if(oAccount.get<uint256_t>("token") != hashToken)
                            continue;

                        /* Add this amount to our total */
                        nUnconfirmed += nCredit;
                    }
                }
            }
        }

        return nUnconfirmed;
    }


    /*  Get the outstanding coinbases. */
    uint64_t GetImmature(const uint256_t& hashGenesis)
    {
        /* Check if we are available to generate our balances. */
        if(!Authentication::Available(hashGenesis))
            return 0;

        /* Return amount */
        uint64_t nImmature = 0;

        /* Get a list of our active events. */
        std::vector<std::pair<uint512_t, uint32_t>> vEvents;
        LLD::Logical->ListEvents(hashGenesis, vEvents);

        /* Build our list of contracts. */
        for(const auto& rEvent : vEvents)
        {
            /* Grab a reference of our hash. */
            const uint512_t& hashEvent = rEvent.first;

            /* Check for Tritium transaction. */
            if(hashEvent.GetType() == TAO::Ledger::TRITIUM)
            {
                /* Get the transaction from disk. */
                TAO::API::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashEvent, tx))
                    continue;

                /* Check if contract has been burned. */
                if(tx.Burned(hashEvent, rEvent.second))
                    continue;

                /* Check if the transaction is mature. */
                if(tx.Mature(hashEvent))
                    continue;
            }

            /* Get a reference of our contract. */
            const TAO::Operation::Contract& rContract =
                LLD::Ledger->ReadContract(hashEvent, rEvent.second, TAO::Ledger::FLAGS::BLOCK);

            /* Check if the given contract is spent already. */
            if(rContract.Spent(rEvent.second))
                continue;

            /* Seek our contract to primitive OP. */
            rContract.SeekToPrimitive();

            /* Get a copy of our primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;

            /* Switch for valid primitives. */
            switch(nOP)
            {
                /* Handle for if we need to credit. */
                case TAO::Operation::OP::COINBASE:
                {
                    /* Extract our coinbase recipient. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    /* Check for valid recipient. */
                    if(hashRecipient != hashGenesis)
                        continue;

                    /* Extract our amount from contract. */
                    uint64_t nAmount = 0;
                    rContract >> nAmount;

                    /* Add to our total expected value. */
                    nImmature += nAmount;

                    break;
                }

                /* Unknown ops we want to continue looping. */
                default:
                {
                    continue;
                }
            }
        }

        return nImmature;
    }


    /* Get any unclaimed transfer transactions. */
    bool GetUnclaimed(const uint256_t& hashGenesis,
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts)
    {
        /* Check if we are available to generate our balances. */
        if(!Authentication::Available(hashGenesis))
            return false;

        /* Get a list of our active events. */
        std::vector<std::pair<uint512_t, uint32_t>> vEvents;
        LLD::Logical->ListEvents(hashGenesis, vEvents);
        LLD::Logical->ListContracts(hashGenesis, vEvents);

        /* Build our list of contracts. */
        for(const auto& rEvent : vEvents)
        {
            /* Grab a reference of our hash. */
            const uint512_t& hashEvent = rEvent.first;

            /* Skip over legacy notifications. */
            if(hashEvent.GetType() != TAO::Ledger::TRITIUM)
                continue;

            /* Get the transaction from disk. */
            TAO::API::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashEvent, tx))
                continue;

            /* Check if contract has been spent. */
            if(tx.Spent(hashEvent, rEvent.second))
                continue;

            /* Get a reference of our contract. */
            const TAO::Operation::Contract& rContract = tx[rEvent.second];

            /* Seek our contract to primitive OP. */
            rContract.SeekToPrimitive();

            /* Get a copy of our primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;

            /* Switch for valid primitives. */
            switch(nOP)
            {
                /* Handle for if we need to credit. */
                case TAO::Operation::OP::TRANSFER:
                {
                    try
                    {
                        /* Let's extract register address to add. */
                        uint256_t hashAddress;
                        rContract >> hashAddress;

                        /* Push to our returned tuple now. */
                        vContracts.push_back(std::make_tuple(rContract, rEvent.second, hashAddress));
                    }
                    catch(const Exception& e)
                    {
                        continue;
                    }

                    break;
                }

                /* Unknown ops we want to continue looping. */
                default:
                {
                    continue;
                }
            }
        }

        return !vContracts.empty();
    }


    /* Returns a type string for the register type */
    std::string GetRegisterName(const uint8_t nType)
    {
        /* Switch based on register type. */
        switch(nType)
        {
            /* State type is READONLY. */
            case TAO::Register::REGISTER::READONLY:
                return "READONLY";

            /* State type is APPEND. */
            case TAO::Register::REGISTER::APPEND:
                return "APPEND";

            /* State type is RAW. */
            case TAO::Register::REGISTER::RAW:
                return "RAW";

            /* State type is OBJECT. */
            case TAO::Register::REGISTER::OBJECT:
                return "OBJECT";

            /* State type is SYSTEM. */
            case TAO::Register::REGISTER::SYSTEM:
                return "SYSTEM";
        }

        return "INVALID";
    }


    /* Returns a type string for the register object type */
    std::string GetStandardName(const uint8_t nType)
    {
        /* Switch based on standard type. */
        switch(nType)
        {
            /* Non Standard types are NONSTANDARD. */
            case TAO::Register::OBJECTS::NONSTANDARD:
                return "OBJECT";

            /* Account standard types are ACCOUNT. */
            case TAO::Register::OBJECTS::ACCOUNT:
                return "ACCOUNT";

            /* Name standard types are NAME. */
            case TAO::Register::OBJECTS::NAME:
                return "NAME";

            /* Namespace standard types are NAMESPACE. */
            case TAO::Register::OBJECTS::NAMESPACE:
                return "NAMESPACE";

            /* Token standard types are TOKEN. */
            case TAO::Register::OBJECTS::TOKEN:
                return "TOKEN";

            /* Trust standard types are TRUST. */
            case TAO::Register::OBJECTS::TRUST:
                return "TRUST";

            /* Crypto standard types are CRYPTO. */
            case TAO::Register::OBJECTS::CRYPTO:
                return "CRYPTO";
        }

        return "UNKNOWN";
    }


    /* Returns a type string for the register _usertype name */
    std::string GetRegisterForm(const uint8_t nType)
    {
        /* Switch based on standard type. */
        switch(nType)
        {
            /* Supply standard _usertype. */
            case USER_TYPES::SUPPLY:
                return "SUPPLY";

            /* Invoice standard _usertype. */
            case USER_TYPES::INVOICE:
                return "INVOICE";

            /* Invoice standard _usertype. */
            case USER_TYPES::ASSET:
                return "ASSET";
        }

        return "STANDARD"; //this is our dummy type
    }
} // End TAO namespace
