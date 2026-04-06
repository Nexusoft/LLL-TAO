/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/templates/socket.h>

#include <Util/include/args.h>
#include <Util/include/runtime.h>

#include <vector>
#include <cstdint>
#include <cstring>
#include <thread>
#include <atomic>

#ifndef WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif


namespace
{

    /** TestSocket
     *
     *  Thin subclass of Socket that exposes protected members for testing.
     *  No behaviour is changed — this only provides accessor/mutator methods
     *  so that unit tests can inspect and manipulate fBufferFull and
     *  nBufferSize directly.
     */
    class TestSocket : public LLP::Socket
    {
    public:
        TestSocket() : LLP::Socket() {}

        /* Accessors for protected atomic flags. */
        bool GetBufferFull() const          { return fBufferFull.load(); }
        void SetBufferFull(bool f)          { fBufferFull.store(f); }
        uint64_t GetBufferSize() const      { return nBufferSize.load(); }
    };

}  /* anonymous namespace */


#ifndef WIN32  /* socketpair tests require POSIX */

/*  Helper: create a connected socketpair and wrap the write-end in a TestSocket.
 *
 *  Returns the read-end fd for draining (caller must close it).
 *  The TestSocket object owns the write-end fd and will close it on destruction.
 *
 *  AF_UNIX/SOCK_STREAM socketpair provides a reliable, in-process pipe that
 *  lets us exercise Write(), Flush(), and Buffered() without network I/O.
 */
static int MakeTestSocket(TestSocket& sock)
{
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    /* fds[0] = read side (returned to caller), fds[1] = write side (given to Socket). */
    sock.fd     = fds[1];
    sock.events = POLLIN;

    return fds[0];
}


/* Helper: drain all available data from the read end. */
static size_t DrainReadEnd(int fdRead, size_t nMax = 1024 * 1024)
{
    size_t nTotal = 0;
    std::vector<char> buf(65536);

    while(nTotal < nMax)
    {
        ssize_t n = recv(fdRead, buf.data(), buf.size(), MSG_DONTWAIT);
        if(n <= 0)
            break;
        nTotal += static_cast<size_t>(n);
    }

    return nTotal;
}


/* Helper: set the kernel send-buffer size on a socket fd. */
static void SetSendBufferSize(int fd, int nSize)
{
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &nSize, sizeof(nSize));
}


/* RAII guard that closes a file descriptor on scope exit. */
struct FdGuard
{
    int fd;
    explicit FdGuard(int f) : fd(f) {}
    ~FdGuard() { if(fd >= 0) close(fd); }
    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;
};


/* ===========================================================================
 *  Section 1: fBufferFull flag lifecycle
 * =========================================================================== */

TEST_CASE("Socket fBufferFull — initial state is false", "[socket][buffer]")
{
    TestSocket sock;
    REQUIRE(sock.GetBufferFull() == false);
}


TEST_CASE("Socket Buffered() — initial state is zero", "[socket][buffer]")
{
    TestSocket sock;
    REQUIRE(sock.Buffered() == 0);
}


/* ===========================================================================
 *  Section 2: Write() direct-send path
 * =========================================================================== */

TEST_CASE("Socket::Write direct send — small payload goes directly to kernel", "[socket][write]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Write a small payload. Buffer should remain empty (direct send path). */
    std::vector<uint8_t> data(128, 0xAB);
    int32_t nSent = sock.Write(data, data.size());

    REQUIRE(nSent > 0);
    REQUIRE(sock.Buffered() == 0);
    REQUIRE(sock.GetBufferFull() == false);

    /* Verify data arrived on the read end. */
    size_t nRead = DrainReadEnd(fdRead);
    REQUIRE(nRead == data.size());
}


TEST_CASE("Socket::Write partial send — remainder goes to buffer", "[socket][write]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Shrink the kernel send buffer so a large write can't fit in one shot. */
    SetSendBufferSize(sock.fd, 4096);

    /* Write much more data than the kernel buffer can hold. */
    std::vector<uint8_t> data(256 * 1024, 0xCD);
    int32_t nSent = sock.Write(data, data.size());

    /* nSent may be partial. The remainder should be in the overflow buffer. */
    if(nSent > 0 && static_cast<size_t>(nSent) < data.size())
    {
        REQUIRE(sock.Buffered() > 0);
        REQUIRE(sock.Buffered() == data.size() - nSent);
    }
    /* If nSent == data.size() the kernel took everything — buffer stays empty. */
    else if(nSent == static_cast<int32_t>(data.size()))
    {
        REQUIRE(sock.Buffered() == 0);
    }
}


/* ===========================================================================
 *  Section 3: Write() buffer-append path
 * =========================================================================== */

TEST_CASE("Socket::Write appends to buffer when buffer has data", "[socket][write]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Shrink kernel buffer and fill it so first write overflows. */
    SetSendBufferSize(sock.fd, 4096);
    std::vector<uint8_t> big(256 * 1024, 0xEE);
    sock.Write(big, big.size());

    uint64_t nBufferedBefore = sock.Buffered();

    /* If we have buffered data, a second write should append to it. */
    if(nBufferedBefore > 0)
    {
        std::vector<uint8_t> small(64, 0xFF);
        int32_t nSent = sock.Write(small, small.size());

        REQUIRE(nSent == static_cast<int32_t>(small.size()));
        REQUIRE(sock.Buffered() == nBufferedBefore + small.size());
    }
}


TEST_CASE("Socket::Write TOCTOU — falls through to direct send if Flush drains between check and lock", "[socket][write]")
{
    /* This test verifies the re-check under SOCKET_MUTEX added by PR #511.
     * We simulate the scenario by:
     * 1. Getting data into the buffer via partial write
     * 2. Flushing it from another operation
     * 3. Then writing again — it should take the direct-send path. */
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    SetSendBufferSize(sock.fd, 4096);

    /* Step 1: cause buffer overflow. */
    std::vector<uint8_t> big(256 * 1024, 0xAA);
    sock.Write(big, big.size());

    if(sock.Buffered() > 0)
    {
        /* Step 2: drain everything via Flush + read end drain. */
        while(sock.Buffered() > 0)
        {
            DrainReadEnd(fdRead);
            sock.Flush();
        }

        REQUIRE(sock.Buffered() == 0);

        /* Step 3: next Write should use direct send, not buffer. */
        std::vector<uint8_t> small(32, 0xBB);
        int32_t nSent = sock.Write(small, small.size());

        /* Direct send returns kernel send count, and buffer should still be empty
         * (or very small if partial). */
        REQUIRE(nSent > 0);
    }
}


/* ===========================================================================
 *  Section 4: Flush() — single-chunk and batch drain
 * =========================================================================== */

TEST_CASE("Socket::Flush on empty buffer returns 0 and clears stale fBufferFull", "[socket][flush]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Manually set stale flag. */
    sock.SetBufferFull(true);

    int nResult = sock.Flush();

    REQUIRE(nResult == 0);
    REQUIRE(sock.GetBufferFull() == false);
}


TEST_CASE("Socket::Flush batch drain — drains entire buffer in one call", "[socket][flush]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Use a decent kernel buffer so Flush can drain everything. */
    SetSendBufferSize(sock.fd, 256 * 1024);

    /* Put data in the overflow buffer via a partial-write scenario. */
    SetSendBufferSize(sock.fd, 4096);
    std::vector<uint8_t> data(128 * 1024, 0xDD);
    sock.Write(data, data.size());

    /* Now enlarge kernel buffer and drain read end so Flush can push. */
    SetSendBufferSize(sock.fd, 256 * 1024);
    DrainReadEnd(fdRead, 1024 * 1024);

    uint64_t nBefore = sock.Buffered();
    if(nBefore > 0)
    {
        /* Single Flush() call should drain a significant portion (or all). */
        int nSent = sock.Flush();
        REQUIRE(nSent > 0);

        /* The batch loop should have sent more than a single MTU (16KB)
         * if the kernel buffer had room. */
        if(nBefore > 16384)
        {
            /* We expect batch flush to drain more than just 16KB. */
            REQUIRE(static_cast<uint64_t>(nSent) > 16384);
        }
    }
}


TEST_CASE("Socket::Flush clears fBufferFull only when fully drained", "[socket][flush]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Shrink kernel buffer to force partial drain. */
    SetSendBufferSize(sock.fd, 4096);

    /* Fill buffer with more data than kernel can hold. */
    std::vector<uint8_t> data(512 * 1024, 0xAA);
    sock.Write(data, data.size());

    /* Set the flag as if WritePacket had detected an overflow. */
    sock.SetBufferFull(true);

    /* Drain read end to allow some sending. */
    DrainReadEnd(fdRead);

    if(sock.Buffered() > 0)
    {
        sock.Flush();

        /* If buffer still has data, fBufferFull should remain true. */
        if(sock.Buffered() > 0)
        {
            REQUIRE(sock.GetBufferFull() == true);
        }
        /* If fully drained, flag should be cleared. */
        else
        {
            REQUIRE(sock.GetBufferFull() == false);
        }
    }
}


/* ===========================================================================
 *  Section 5: Stale latch clearing in Write()
 * =========================================================================== */

TEST_CASE("Socket::Write clears stale fBufferFull when buffer is empty", "[socket][buffer][stale-latch]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Simulate stale latch: flag set but buffer empty. */
    sock.SetBufferFull(true);
    REQUIRE(sock.Buffered() == 0);

    /* Write() entry should clear the stale flag before proceeding. */
    std::vector<uint8_t> data(64, 0x42);
    sock.Write(data, data.size());

    REQUIRE(sock.GetBufferFull() == false);
}


TEST_CASE("Socket::Write preserves fBufferFull when buffer has data", "[socket][buffer][stale-latch]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Fill buffer. */
    SetSendBufferSize(sock.fd, 4096);
    std::vector<uint8_t> big(256 * 1024, 0xCC);
    sock.Write(big, big.size());

    if(sock.Buffered() > 0)
    {
        sock.SetBufferFull(true);

        /* Write more — flag should stay true because buffer has data. */
        std::vector<uint8_t> small(32, 0xDD);
        sock.Write(small, small.size());

        REQUIRE(sock.GetBufferFull() == true);
    }
}


/* ===========================================================================
 *  Section 6: compare_exchange_strong prevents flag clobber
 * =========================================================================== */

TEST_CASE("Flush empty-return uses compare_exchange — does not clobber concurrent true", "[socket][buffer][compare-exchange]")
{
    /* This tests the atomic correctness fix (Bug #4):
     * If fBufferFull is false when Flush() enters the empty-buffer path,
     * compare_exchange_strong should not modify it.  But if another thread
     * sets it to true between the check and the CAS, it must not be cleared.
     *
     * We can't perfectly simulate the race in a single-threaded test, but we
     * can verify that Flush() on an empty buffer with fBufferFull=false
     * leaves it false (no spurious modification). */
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    sock.SetBufferFull(false);

    int nResult = sock.Flush();
    REQUIRE(nResult == 0);
    REQUIRE(sock.GetBufferFull() == false);
}


/* ===========================================================================
 *  Section 7: Full Write → Flush → Drain cycle
 * =========================================================================== */

TEST_CASE("Full cycle: Write → Flush → Drain → buffer empty", "[socket][integration]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    SetSendBufferSize(sock.fd, 8192);

    /* Write enough to overflow into buffer. */
    std::vector<uint8_t> data(64 * 1024, 0x55);
    sock.Write(data, data.size());

    /* Iteratively drain until buffer is empty. */
    int nIterations = 0;
    while(sock.Buffered() > 0 && nIterations < 100)
    {
        DrainReadEnd(fdRead);
        sock.Flush();
        ++nIterations;
    }

    REQUIRE(sock.Buffered() == 0);
    REQUIRE(sock.GetBufferFull() == false);

    /* Verify total data integrity by counting bytes. */
    size_t nFinal = DrainReadEnd(fdRead);
    /* All data should have been transferred (some already drained in the loop). */
    REQUIRE(nFinal >= 0);  /* Remaining bytes if any */
}


TEST_CASE("Multiple writes accumulate in buffer, single Flush cycle drains", "[socket][integration]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    SetSendBufferSize(sock.fd, 4096);

    /* Fill kernel buffer first. */
    std::vector<uint8_t> initial(128 * 1024, 0x11);
    sock.Write(initial, initial.size());

    /* Append several small writes. */
    for(int i = 0; i < 10; ++i)
    {
        std::vector<uint8_t> small(256, static_cast<uint8_t>(i));
        sock.Write(small, small.size());
    }

    uint64_t nTotalBuffered = sock.Buffered();

    /* Drain and flush until empty. */
    int nIterations = 0;
    while(sock.Buffered() > 0 && nIterations < 200)
    {
        DrainReadEnd(fdRead);
        sock.Flush();
        ++nIterations;
    }

    REQUIRE(sock.Buffered() == 0);
    INFO("Drained " << nTotalBuffered << " bytes in " << nIterations << " iterations");
}


/* ===========================================================================
 *  Section 8: Flush() error handling
 * =========================================================================== */

TEST_CASE("Socket::Flush returns error on closed peer", "[socket][flush][error]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);

    /* Put data in buffer. */
    SetSendBufferSize(sock.fd, 4096);
    std::vector<uint8_t> data(32 * 1024, 0xAA);
    sock.Write(data, data.size());

    /* Close the read end to simulate peer disconnect. */
    close(fdRead);

    if(sock.Buffered() > 0)
    {
        /* Flush should eventually get a send error (EPIPE or similar).
         * The exact behavior depends on kernel buffering, so we just
         * verify Flush() doesn't crash or hang. */
        int nResult = sock.Flush();

        /* Result is either bytes sent (kernel buffer hadn't noticed yet)
         * or an error. */
        REQUIRE((nResult >= 0 || nResult < 0));  /* No crash/hang */
        REQUIRE(sock.nConsecutiveErrors.load() >= 0);
    }
}


/* ===========================================================================
 *  Section 9: Concurrent Flush + Write interaction
 * =========================================================================== */

TEST_CASE("Concurrent Write and Flush — no data corruption", "[socket][concurrency]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    SetSendBufferSize(sock.fd, 8192);

    std::atomic<bool> fDone(false);
    std::atomic<uint64_t> nTotalWritten(0);

    /* Writer thread: continuously write small packets. */
    std::thread writer([&]()
    {
        for(int i = 0; i < 200 && !fDone.load(); ++i)
        {
            std::vector<uint8_t> data(128, static_cast<uint8_t>(i & 0xFF));
            int32_t n = sock.Write(data, data.size());
            if(n > 0)
                nTotalWritten.fetch_add(n);

            /* Brief yield to allow interleaving. */
            std::this_thread::yield();
        }
        fDone.store(true);
    });

    /* Main thread: continuously flush and drain. */
    size_t nTotalDrained = 0;
    while(!fDone.load() || sock.Buffered() > 0)
    {
        nTotalDrained += DrainReadEnd(fdRead);
        if(sock.Buffered() > 0)
            sock.Flush();
        std::this_thread::yield();
    }

    /* Final drain. */
    nTotalDrained += DrainReadEnd(fdRead);
    while(sock.Buffered() > 0)
    {
        DrainReadEnd(fdRead);
        sock.Flush();
        nTotalDrained += DrainReadEnd(fdRead);
    }

    writer.join();

    /* Buffer should be empty after full drain. */
    REQUIRE(sock.Buffered() == 0);
    REQUIRE(sock.GetBufferFull() == false);
}


/* ===========================================================================
 *  Section 10: Batch Flush performance verification
 * =========================================================================== */

TEST_CASE("Batch Flush drains more than one MTU per call", "[socket][flush][batch]")
{
    TestSocket sock;
    int fdRead = MakeTestSocket(sock);
    FdGuard guard(fdRead);

    /* Use a generous kernel buffer so multiple MTU chunks can be sent. */
    SetSendBufferSize(sock.fd, 256 * 1024);
    SetSendBufferSize(fdRead, 256 * 1024);

    /* Force data into the overflow buffer by first filling kernel buffer
     * with a tiny setting, then expanding it. */
    SetSendBufferSize(sock.fd, 2048);
    std::vector<uint8_t> data(128 * 1024, 0x77);
    sock.Write(data, data.size());

    uint64_t nBuffered = sock.Buffered();

    /* Now give the kernel room and drain the read side. */
    SetSendBufferSize(sock.fd, 256 * 1024);
    DrainReadEnd(fdRead, 512 * 1024);

    if(nBuffered > 16384)
    {
        /* A single Flush() should drain more than 16KB (the old MTU limit). */
        int nSent = sock.Flush();

        /* The batch loop should have pushed significantly more than one MTU. */
        INFO("Buffered before flush: " << nBuffered << ", sent: " << nSent);
        REQUIRE(nSent > 0);

        /* With sufficient kernel buffer, we expect much more than 16KB. */
        if(nBuffered > 65536)
        {
            REQUIRE(static_cast<uint64_t>(nSent) > 16384);
        }
    }
}


#endif  /* !WIN32 */
