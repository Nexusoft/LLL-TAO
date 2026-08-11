/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(W.J.[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_REWARD_BINDING_PAYLOAD_H
#define NEXUS_LLP_INCLUDE_REWARD_BINDING_PAYLOAD_H

#include <LLC/types/uint1024.h>
#include <Util/include/hex.h>

#include <cstdint>
#include <string>
#include <vector>

namespace LLP
{
    /** RewardBindingPayload
     *
     *  Shared parser for the decrypted MINER_SET_REWARD payload, used by both
     *  the Stateless mining lane (port 9323) and the Legacy mining lane
     *  (port 8323).  Both lanes route MINER_SET_REWARD through
     *  StatelessMiner::ProcessSetReward, so they share the exact same payload
     *  layout — extracting the parser here makes the wire contract explicit
     *  and unit-testable in isolation from the AEAD/transport layers.
     *
     *  Wire format (big-endian / display order; matches NexusMiner output):
     *
     *      byte[0..31]                     : recipient sigchain genesis hash
     *      byte[32]   (optional)           : 1-byte account-name length N
     *      byte[33..32+N] (optional)       : N bytes of account-name string
     *
     *  Legacy 32-byte payloads remain compatible: parsing succeeds with
     *  hashGenesis populated and strAccountName empty (event-only mode).
     *
     *  Extended payloads remain valid input: a non-empty account-name is
     *  parsed and returned to callers, but current node behavior ignores
     *  the name and keeps coinbase rewards on the standard event/credit
     *  path. The parser is kept for forward compatibility and diagnostics.
     *
     **/
    struct RewardBindingPayload
    {
        /** Result codes for ParsePayload.  Distinct codes give callers
         *  enough information to log a precise error without leaking
         *  implementation detail to the wire (the encrypted REWARD_RESULT
         *  response is a single failure byte either way). */
        enum class ParseResult : uint8_t
        {
            Ok                 = 0, // Parsed cleanly.
            TooShort           = 1, // size < 32 — no genesis hash present.
            ExtensionTooShort  = 2, // 32 < size < 34 — extension marker without name.
            NameLengthZero     = 3, // length byte == 0 (must be empty payload, not extension).
            NameLengthMismatch = 4, // declared N bytes ≠ actual remaining bytes.
        };


        /** ParsePayload
         *
         *  Decode a decrypted MINER_SET_REWARD payload into a recipient
         *  genesis hash and an optional account-name string.  Pure function
         *  — no I/O, no chain state, no logging — safe to unit test.
         *
         *  @param[in]  vDecrypted        Plaintext payload from AEAD decrypt.
         *  @param[out] hashGenesisOut    Recipient sigchain genesis (always set on Ok).
         *  @param[out] strAccountNameOut Resolved account name (empty if legacy 32-byte payload).
         *
         *  @return ParseResult::Ok on success, otherwise the specific failure mode.
         *
         **/
        static ParseResult ParsePayload(
            const std::vector<uint8_t>& vDecrypted,
            uint256_t&                  hashGenesisOut,
            std::string&                strAccountNameOut)
        {
            /* Reset outputs so callers see empty values on every failure path. */
            hashGenesisOut    = uint256_t(0);
            strAccountNameOut.clear();

            /* The 32-byte genesis hash is required.  Smaller payloads cannot
             * possibly carry a recipient. */
            if(vDecrypted.size() < 32)
                return ParseResult::TooShort;

            /* Sizes 33..(33+0) make no sense: either you are the legacy
             * 32-byte form or you carry a length byte plus at least one
             * name byte (>= 34 bytes total). */
            if(vDecrypted.size() > 32 && vDecrypted.size() < 34)
                return ParseResult::ExtensionTooShort;

            /* Decode the genesis hash via SetHex() to preserve the
             * NexusMiner big-endian wire ordering — see the
             * SESSION_START byte-order memory for the same convention. */
            hashGenesisOut.SetHex(HexStr(vDecrypted.begin(), vDecrypted.begin() + 32));

            /* Legacy 32-byte payload — event-only mode. */
            if(vDecrypted.size() == 32)
                return ParseResult::Ok;

            /* Extended payload: validate length byte before reading the name. */
            const uint8_t nNameBytes = vDecrypted[32];
            if(nNameBytes == 0)
                return ParseResult::NameLengthZero;

            if(vDecrypted.size() != static_cast<size_t>(33u) + nNameBytes)
                return ParseResult::NameLengthMismatch;

            strAccountNameOut.assign(vDecrypted.begin() + 33, vDecrypted.end());
            return ParseResult::Ok;
        }


        /** ResultString
         *
         *  Human-readable mapping for diagnostic logging.
         *
         **/
        static const char* ResultString(const ParseResult eResult)
        {
            switch(eResult)
            {
                case ParseResult::Ok:                 return "Ok";
                case ParseResult::TooShort:           return "TooShort";
                case ParseResult::ExtensionTooShort:  return "ExtensionTooShort";
                case ParseResult::NameLengthZero:     return "NameLengthZero";
                case ParseResult::NameLengthMismatch: return "NameLengthMismatch";
            }
            return "Unknown";
        }
    };
}

#endif /* NEXUS_LLP_INCLUDE_REWARD_BINDING_PAYLOAD_H */
