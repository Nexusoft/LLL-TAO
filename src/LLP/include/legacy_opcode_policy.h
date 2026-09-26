/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_LEGACY_OPCODE_POLICY_H
#define NEXUS_LLP_INCLUDE_LEGACY_OPCODE_POLICY_H

#include <LLP/include/opcode_utility.h>
#include <LLP/include/stateless_miner.h>

#include <cstdint>

namespace LLP
{

    /** Single-source-of-truth policy table for legacy (8-bit) miner opcodes
     *  on port 8323.
     *
     *  Replaces three independent switch statements (bypass / protected /
     *  minimum-state) that tended to drift out of sync.  Each protected
     *  opcode declares its required MinerSessionState in one place; bypass
     *  opcodes are absent from the table and resolve to CONNECTED.
     *
     *  Layout: { opcode, requiredState }.  Lookup is O(N) over a tiny array
     *  — N is small enough (≤16) that a hash table would only add overhead
     *  and obscure the policy.
     **/
    struct LegacyOpcodePolicy
    {
        uint8_t           nOpcode;
        MinerSessionState nRequired;
    };

    inline constexpr LegacyOpcodePolicy LEGACY_OPCODE_POLICY[] =
    {
        /* Channel-requiring opcodes need at least CHANNEL_SET */
        { OpcodeUtility::Opcodes::GET_BLOCK,         MinerSessionState::CHANNEL_SET     },
        { OpcodeUtility::Opcodes::SUBMIT_BLOCK,      MinerSessionState::CHANNEL_SET     },
        { OpcodeUtility::Opcodes::GET_ROUND,         MinerSessionState::CHANNEL_SET     },
        { OpcodeUtility::Opcodes::GET_REWARD,        MinerSessionState::CHANNEL_SET     },
        { OpcodeUtility::Opcodes::MINER_READY,       MinerSessionState::CHANNEL_SET     },
        { OpcodeUtility::Opcodes::CHECK_BLOCK,       MinerSessionState::CHANNEL_SET     },

        /* Reward binding requires encryption */
        { OpcodeUtility::Opcodes::MINER_SET_REWARD,  MinerSessionState::ENCRYPTION_READY },

        /* Other protected opcodes need only AUTHENTICATED */
        { OpcodeUtility::Opcodes::SET_CHANNEL,       MinerSessionState::AUTHENTICATED   },
        { OpcodeUtility::Opcodes::SESSION_KEEPALIVE, MinerSessionState::AUTHENTICATED   },
        { OpcodeUtility::Opcodes::GET_HEIGHT,        MinerSessionState::AUTHENTICATED   },
        { OpcodeUtility::Opcodes::CLEAR_MAP,         MinerSessionState::AUTHENTICATED   },
        { OpcodeUtility::Opcodes::SUBSCRIBE,         MinerSessionState::AUTHENTICATED   },
    };


    /** FindLegacyPolicy
     *
     *  Locate the policy entry for a legacy opcode, or nullptr if the opcode
     *  is not protected (i.e. is a bypass opcode such as MINER_AUTH_INIT).
     *
     **/
    inline const LegacyOpcodePolicy* FindLegacyPolicy(const uint8_t nOpcode)
    {
        for(const auto& entry : LEGACY_OPCODE_POLICY)
        {
            if(entry.nOpcode == nOpcode)
                return &entry;
        }
        return nullptr;
    }


    /** IsLegacyPreflightBypassOpcode
     *
     *  Opcodes that intentionally bypass the preflight gate because they
     *  drive the authentication / session-start handshake itself.
     *
     **/
    inline bool IsLegacyPreflightBypassOpcode(const uint8_t nOpcode)
    {
        switch(nOpcode)
        {
            case OpcodeUtility::Opcodes::MINER_AUTH_INIT:
            case OpcodeUtility::Opcodes::MINER_AUTH_RESPONSE:
            case OpcodeUtility::Opcodes::MINER_AUTH_RESULT:
            case OpcodeUtility::Opcodes::SESSION_START:
                return true;

            default:
                return false;
        }
    }


    /** IsLegacyPreflightProtectedOpcode
     *
     *  True if the opcode appears in LEGACY_OPCODE_POLICY and therefore
     *  requires a minimum session state before processing.
     *
     **/
    inline bool IsLegacyPreflightProtectedOpcode(const uint8_t nOpcode)
    {
        return FindLegacyPolicy(nOpcode) != nullptr;
    }


    /** MinimumStateForLegacyOpcode
     *
     *  Returns the minimum MinerSessionState required for a given protected
     *  legacy opcode.  Bypass / unknown opcodes return CONNECTED (no gating).
     *
     **/
    inline MinerSessionState MinimumStateForLegacyOpcode(const uint8_t nOpcode)
    {
        const LegacyOpcodePolicy* policy = FindLegacyPolicy(nOpcode);
        return policy ? policy->nRequired : MinerSessionState::CONNECTED;
    }


    /** IsLegacyBlockOpcode
     *
     *  True for opcodes whose semantics make BLOCK_REJECTED a meaningful
     *  response (GET_BLOCK / SUBMIT_BLOCK).  Used by Miner::PreflightSessionGate
     *  to avoid sending BLOCK_REJECTED in reply to non-block opcodes such as
     *  MINER_READY, SET_CHANNEL, or SUBSCRIBE — a protocol violation that
     *  confuses legacy miner clients into POLL_ERROR / disconnect.
     *
     **/
    inline bool IsLegacyBlockOpcode(const uint8_t nOpcode)
    {
        return nOpcode == OpcodeUtility::Opcodes::GET_BLOCK
            || nOpcode == OpcodeUtility::Opcodes::SUBMIT_BLOCK;
    }

}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_LEGACY_OPCODE_POLICY_H */
