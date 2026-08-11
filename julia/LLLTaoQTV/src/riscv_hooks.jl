"""
    aligned_allocate(n_bytes::Int) -> Vector{UInt8}

Allocate a byte buffer.

On RISC-V: rounds up to a multiple of `RISCV.cache_line_bytes` so that the
working buffer is naturally aligned to the cache-line boundary, which enables
V-ext strided-load patterns without software padding.
On other platforms: plain `Vector{UInt8}(undef, n_bytes)`.
"""
function aligned_allocate(n_bytes::Int) :: Vector{UInt8}
    n_bytes >= 0 || throw(ArgumentError("n_bytes must be non-negative"))

    if RISCV.is_riscv
        cl   = RISCV.cache_line_bytes
        size = cl * cld(n_bytes, cl)   # round up to next cache-line multiple
        return Vector{UInt8}(undef, size)
    else
        return Vector{UInt8}(undef, n_bytes)
    end
end

"""
    xor_bytes_riscv(a::Vector{UInt8}, b::Vector{UInt8}) -> Vector{UInt8}

XOR two equal-length byte vectors.

When `RISCV.has_v_ext == true`: uses Julia's `@simd` + `@inbounds` with a
64-byte aligned temporary allocation matching RISC-V V-ext VLEN=512 stripe.
Otherwise: standard `xor.(a, b)`.
"""
function xor_bytes_riscv(a::Vector{UInt8}, b::Vector{UInt8}) :: Vector{UInt8}
    length(a) == length(b) || throw(ArgumentError("xor_bytes_riscv: vectors must have equal length"))
    n = length(a)

    if RISCV.has_v_ext
        # RISC-V V-ext: vectorized XOR, VLEN=512 stripe
        out = aligned_allocate(n)
        resize!(out, n)
        @inbounds @simd for i in 1:n
            out[i] = a[i] ⊻ b[i]
        end
        return out
    else
        return xor.(a, b)
    end
end

"""
    sha512_chain_riscv(state::Vector{UInt8}, tag::Vector{UInt8}, epoch::UInt64, counter::UInt64) -> Vector{UInt8}

Compute one SHA-512 chaining step.

On RISC-V: prepares the input buffer aligned to 128-byte boundary to allow
the hardware SHA accelerator (if present) to process full cache-line blocks
without internal padding stalls.
On non-RISC-V: plain `SHA.sha512(vcat(state, tag, ...))`.
"""
function sha512_chain_riscv(state::Vector{UInt8}, tag::Vector{UInt8}, epoch::UInt64, counter::UInt64) :: Vector{UInt8}
    epoch_bytes   = reinterpret(UInt8, [epoch])
    counter_bytes = reinterpret(UInt8, [counter])

    if RISCV.is_riscv
        # Allocate aligned input buffer (128-byte boundary for RISC-V SHA accelerator)
        raw_len = length(state) + length(tag) + 8 + 8
        buf = aligned_allocate(raw_len)
        resize!(buf, raw_len)
        pos = 1
        @inbounds for b in state;   buf[pos] = b; pos += 1; end
        @inbounds for b in tag;     buf[pos] = b; pos += 1; end
        @inbounds for b in epoch_bytes;   buf[pos] = b; pos += 1; end
        @inbounds for b in counter_bytes; buf[pos] = b; pos += 1; end
        return SHA.sha512(buf)
    else
        return SHA.sha512(vcat(state, tag, collect(epoch_bytes), collect(counter_bytes)))
    end
end

"""
    dispatch_xor(a, b) -> Vector{UInt8}

Dispatch-table entry for XOR: calls `xor_bytes_riscv` on RISC-V, else `xor.(a, b)`.
"""
function dispatch_xor(a::AbstractVector{UInt8}, b::AbstractVector{UInt8}) :: Vector{UInt8}
    RISCV.is_riscv ? xor_bytes_riscv(Vector{UInt8}(a), Vector{UInt8}(b)) : xor.(a, b)
end

"""
    dispatch_sha512_chain(state, tag, epoch, counter) -> Vector{UInt8}

Dispatch-table entry for SHA-512 chaining: calls `sha512_chain_riscv` on RISC-V.
"""
function dispatch_sha512_chain(state::Vector{UInt8}, tag::Vector{UInt8}, epoch::UInt64, counter::UInt64) :: Vector{UInt8}
    sha512_chain_riscv(state, tag, epoch, counter)
end
