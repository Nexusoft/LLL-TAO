"""
    FalconBucket

An immutable segment of a Falcon-1024 private key with its SHA-512 tag.

Fields:
- `id`           — 1-based bucket index (1..4)
- `start_offset` — 1-based byte offset in the full private key
- `stop_offset`  — inclusive end byte offset
- `payload`      — raw bytes of this segment (immutable after construction)
- `tag`          — SHA-512 hex digest of `payload`
"""
struct FalconBucket
    id           :: Int
    start_offset :: Int
    stop_offset  :: Int
    payload      :: Vector{UInt8}
    tag          :: String
end

"""
    bucket_sha512_tag(payload::Vector{UInt8}) -> String

Compute the SHA-512 hex digest of `payload`.
On RISC-V an aligned buffer is used as a memory-layout hint.
"""
function bucket_sha512_tag(payload::Vector{UInt8}) :: String
    if RISCV.is_riscv
        buf = aligned_allocate(length(payload))
        resize!(buf, length(payload))
        copyto!(buf, payload)
        return bytes2hex(SHA.sha512(buf))
    else
        return bytes2hex(SHA.sha512(payload))
    end
end

"""
    partition_privkey(privkey::Vector{UInt8}) -> NTuple{4, FalconBucket}

Partition a 2305-byte Falcon-1024 private key into exactly 4 `FalconBucket`s
according to `BUCKET_OFFSETS`.

Throws `ArgumentError` if `length(privkey) != FALCON1024_PRIVKEY_BYTES`.
"""
function partition_privkey(privkey::Vector{UInt8}) :: NTuple{4, FalconBucket}
    length(privkey) == FALCON1024_PRIVKEY_BYTES ||
        throw(ArgumentError("partition_privkey: expected $FALCON1024_PRIVKEY_BYTES bytes, got $(length(privkey))"))

    buckets = ntuple(BUCKET_COUNT) do i
        rng     = BUCKET_OFFSETS[i]
        segment = privkey[rng.start:rng.stop]
        FalconBucket(i, rng.start, rng.stop, segment, bucket_sha512_tag(segment))
    end
    return buckets
end

"""
    verify_bucket_tags(buckets::NTuple{4, FalconBucket}) -> Bool

Recompute all 4 SHA-512 tags and compare against stored tags.
Returns `true` iff all tags match.
"""
function verify_bucket_tags(buckets::NTuple{4, FalconBucket}) :: Bool
    return all(b -> bucket_sha512_tag(b.payload) == b.tag, buckets)
end

"""
    bucket_manifest(buckets::NTuple{4, FalconBucket}) -> Vector{NamedTuple}

Return a summary of bucket metadata without exposing payload bytes.
Each entry has fields: `id`, `start_offset`, `stop_offset`, `length`, `tag`.
"""
function bucket_manifest(buckets::NTuple{4, FalconBucket}) :: Vector{NamedTuple}
    return [
        (
            id           = b.id,
            start_offset = b.start_offset,
            stop_offset  = b.stop_offset,
            length       = length(b.payload),
            tag          = b.tag,
        ) for b in buckets
    ]
end
