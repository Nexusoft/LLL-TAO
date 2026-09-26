/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_OPCODES_H
#define NEXUS_LLP_INCLUDE_STATELESS_OPCODES_H

/** BACKWARD COMPATIBILITY WRAPPER
 *
 *  This file provides backward compatibility for code using the old
 *  StatelessOpcodes namespace. All stateless opcode definitions have
 *  been consolidated into OpcodeUtility::Stateless in opcode_utility.h
 *
 *  New code should use OpcodeUtility::Stateless directly.
 *  
 *  INTENTIONAL NAMESPACE ALIAS:
 *  This header intentionally introduces the StatelessOpcodes alias into
 *  the including scope to maintain backward compatibility with existing
 *  code that uses patterns like:
 *    - using namespace StatelessOpcodes;
 *    - StatelessOpcodes::Mirror(opcode)
 *    - StatelessOpcodes::STATELESS_GET_BLOCK
 *  
 *  This is a transitional compatibility layer. Legacy code continues to
 *  work without changes, while all definitions come from the canonical
 *  source (OpcodeUtility::Stateless).
 **/

#include <LLP/include/opcode_utility.h>

namespace LLP
{
    /** Backward compatibility namespace alias
     *  StatelessOpcodes now points to OpcodeUtility::Stateless
     *  
     *  NOTE: Including this header introduces the StatelessOpcodes alias.
     *  This is intentional for backward compatibility.
     **/
    namespace StatelessOpcodes = OpcodeUtility::Stateless;

} // namespace LLP

#endif
