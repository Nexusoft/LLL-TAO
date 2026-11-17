"""
    _encode_uint64_le(value::UInt64) -> Vector{UInt8}

Encode a UInt64 as 8 bytes in little-endian order.
"""
function _encode_uint64_le(value::UInt64) :: Vector{UInt8}
    bytes = Vector{UInt8}(undef, 8)
    @inbounds for i in 1:8
        bytes[i] = UInt8((value >> (8 * (i - 1))) & 0xff)
    end
    return bytes
end

"""
    derive_keystream(length_bytes::Int, seed::UInt64, epoch::Int, bucket_id::Int, tag::String) -> Vector{UInt8}

Derive a pseudo-random keystream using iterated SHA-512 chaining.

The derivation is deterministic given the inputs and uses domain separation
via `DOMAIN_TAG_BYTES` to prevent cross-context collisions.

On RISC-V, the chaining calls are routed through `dispatch_sha512_chain` which
uses cache-aligned buffer preparation as a memory-layout hint.
"""
function derive_keystream(length_bytes::Int, seed::UInt64, epoch::Int, bucket_id::Int, tag::String) :: Vector{UInt8}
    length_bytes >= 0  || throw(ArgumentError("keystream length must be non-negative"))
    epoch      >= 0    || throw(ArgumentError("epoch must be non-negative"))
    bucket_id  >= 1    || throw(ArgumentError("bucket_id must be >= 1"))

    tag_bytes = collect(codeunits(tag))
    state = SHA.sha512(vcat(
        DOMAIN_TAG_BYTES,
        _encode_uint64_le(seed),
        _encode_uint64_le(UInt64(epoch)),
        _encode_uint64_le(UInt64(bucket_id)),
        tag_bytes,
    ))

    stream  = UInt8[]
    counter = UInt64(0)

    while length(stream) < length_bytes
        # Route through RISC-V aligned chaining when on RISC-V hardware
        state = dispatch_sha512_chain(state, tag_bytes, UInt64(epoch), counter)
        append!(stream, state)
        counter += UInt64(1)
    end

    resize!(stream, length_bytes)
    return stream
end

"""
    encode_bucket(bucket::FalconBucket, seed::UInt64, epoch::Int) -> Vector{UInt8}

XOR-encode `bucket.payload` with a derived keystream.
The result is the working vector stored in a `QuantumTunnelVector`.
"""
function encode_bucket(bucket::FalconBucket, seed::UInt64, epoch::Int) :: Vector{UInt8}
    keystream = derive_keystream(length(bucket.payload), seed, epoch, bucket.id, bucket.tag)
    return dispatch_xor(bucket.payload, keystream)
end

"""
    decode_working_vector(working_vector::Vector{UInt8}, bucket::FalconBucket, seed::UInt64, epoch::Int) -> Vector{UInt8}

Recover the original bucket payload from a working vector produced by `encode_bucket`.
XOR is self-inverse: `decode(encode(p)) == p`.
"""
function decode_working_vector(working_vector::Vector{UInt8}, bucket::FalconBucket, seed::UInt64, epoch::Int) :: Vector{UInt8}
    keystream = derive_keystream(length(working_vector), seed, epoch, bucket.id, bucket.tag)
    return dispatch_xor(working_vector, keystream)
end
