/* Distributed under the MIT software license, see the accompanying COPYING. */
#pragma once

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/stateless_packet.h>
#include <TAO/Ledger/include/stateless_block_utility.h>

namespace LLP
{
    inline uint8_t MinedBlockRejectionCode(TAO::Ledger::MinedBlockFailure failure, bool acceptance)
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

        return code;
    }

    inline StatelessPacket SubmitBlockRejection(uint8_t code)
    {
        StatelessPacket packet(OpcodeUtility::Stateless::BLOCK_REJECTED);
        packet.DATA.push_back(code);
        packet.LENGTH = static_cast<uint32_t>(packet.DATA.size());
        return packet;
    }

    inline StatelessPacket SubmitBlockRejection(OpcodeUtility::SubmitBlockRejectionReason reason)
    {
        return SubmitBlockRejection(static_cast<uint8_t>(reason));
    }

    inline StatelessPacket MinedBlockRejection(TAO::Ledger::MinedBlockFailure failure,
                                               bool acceptance)
    {
        return SubmitBlockRejection(MinedBlockRejectionCode(failure, acceptance));
    }
}
