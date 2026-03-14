#pragma once

#include <atomic>
#include <cstdint>

namespace LLP
{
    namespace GracefulShutdown
    {
        /* Planned node shutdown reason code used by NODE_SHUTDOWN (0xD0FF). */
        static constexpr uint32_t REASON_GRACEFUL = 1;

        /* Bounded grace period that gives queued NODE_SHUTDOWN packets time to egress. */
        static constexpr uint32_t MINER_SHUTDOWN_FLUSH_MS = 250;

        /* Per-connection state that tracks whether graceful shutdown was already sent. */
        class NotificationState
        {
        public:
            bool MarkSent()
            {
                return !fSent.exchange(true);
            }

            bool Sent() const
            {
                return fSent.load();
            }

        private:
            std::atomic<bool> fSent{false};
        };
    }
}
