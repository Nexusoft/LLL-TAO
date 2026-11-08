/* Distributed under the MIT software license, see the accompanying COPYING. */
#pragma once

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/stateless_packet.h>
#include <TAO/Ledger/include/stateless_block_utility.h>

#include <string>

namespace LLP
{
    inline StatelessPacket MinedBlockRejection(TAO::Ledger::MinedBlockFailure failure,
                                               bool acceptance, const std::string& reason)
    {
        using TAO::Ledger::MinedBlockFailure;
        using OpcodeUtility::RejectionReason;
        using OpcodeUtility::SubmitBlockRejectionReason;
        uint8_t code = static_cast<uint8_t>(acceptance
            ? SubmitBlockRejectionReason::ACCEPTANCE_FAILED
            : SubmitBlockRejectionReason::VALIDATION_FAILED);
        if(failure == MinedBlockFailure::WORK_REVOKED)
            code = static_cast<uint8_t>(SubmitBlockRejectionReason::WORK_REVOKED);
        else if(failure == MinedBlockFailure::STALE)
            code = static_cast<uint8_t>(RejectionReason::STALE);
        else if(failure == MinedBlockFailure::DUPLICATE)
            code = static_cast<uint8_t>(RejectionReason::DUPLICATE);

        StatelessPacket packet(OpcodeUtility::Stateless::BLOCK_REJECTED);
        packet.DATA.push_back(code);
        /* Printable ASCII is valid UTF-8 and bounds arbitrary exception text. */
        for(unsigned char c : reason)
        {
            if(packet.DATA.size() == 241)
                break;
            packet.DATA.push_back(c >= 0x20 && c <= 0x7e ? c : '?');
        }
        packet.LENGTH = static_cast<uint32_t>(packet.DATA.size());
        return packet;
    }
}
