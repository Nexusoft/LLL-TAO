module FalconQuantumTunnelVector

using Random: MersenneTwister, rand, randperm
using SHA: sha512

export BUCKET_COUNT,
       FALCON1024_PRIVATE_KEY_SIZE,
       FALCON1024_PUBLIC_KEY_SIZE,
       FalconBucket,
       SwapEvent,
       QuantumTunnelVector,
       active_bucket,
       active_bucket_id,
       bucket_manifest,
       build_qtv,
       decode_working_vector,
       reconstruct_payload,
       run_swap_rounds!,
       swap_active_bucket!

const BUCKET_COUNT = 4
const FALCON1024_PUBLIC_KEY_SIZE = 1793
const FALCON1024_PRIVATE_KEY_SIZE = 2305
const DEFAULT_SEED = 0x1024
const DOMAIN_TAG = collect(codeunits("FalconQTV"))

struct FalconBucket
    id::Int
    start_offset::Int
    stop_offset::Int
    payload::Vector{UInt8}
    tag::String
end

struct SwapEvent
    epoch::Int
    from_bucket::Int
    to_bucket::Int
    slot::Int
    permutation::NTuple{BUCKET_COUNT, Int}
end

mutable struct QuantumTunnelVector
    buckets::Vector{FalconBucket}
    permutation::Vector{Int}
    active_slot::Int
    working_vector::Vector{UInt8}
    epoch::Int
    swap_log::Vector{SwapEvent}
    seed::UInt64
    rng::MersenneTwister
end

function encode_uint64_little_endian(value::Integer)
    value >= 0 || throw(ArgumentError("value must be non-negative"))

    bytes = Vector{UInt8}(undef, 8)
    encoded = UInt64(value)

    for index in 1:8
        bytes[index] = UInt8((encoded >> (8 * (index - 1))) & 0xff)
    end

    return bytes
end

function bucket_ranges(total_length::Integer, bucket_count::Integer = BUCKET_COUNT)
    total_length > 0 || throw(ArgumentError("payload length must be positive"))
    bucket_count > 0 || throw(ArgumentError("bucket count must be positive"))

    base, remainder = divrem(Int(total_length), Int(bucket_count))
    start_offset = 1
    ranges = UnitRange{Int}[]

    for bucket_id in 1:Int(bucket_count)
        bucket_length = base + (bucket_id <= remainder ? 1 : 0)
        stop_offset = start_offset + bucket_length - 1
        push!(ranges, start_offset:stop_offset)
        start_offset = stop_offset + 1
    end

    return ranges
end

bucket_tag(payload::AbstractVector{UInt8}) = bytes2hex(sha512(payload))

function derive_keystream(length_bytes::Integer, seed::UInt64, epoch::Integer, bucket_id::Integer, tag::AbstractString)
    length_bytes >= 0 || throw(ArgumentError("keystream length must be non-negative"))
    epoch >= 0 || throw(ArgumentError("epoch must be non-negative"))
    bucket_id > 0 || throw(ArgumentError("bucket id must be positive"))

    state = sha512(vcat(DOMAIN_TAG, encode_uint64_little_endian(seed), encode_uint64_little_endian(epoch), encode_uint64_little_endian(bucket_id), collect(codeunits(tag))))
    stream = UInt8[]
    counter = 0

    while length(stream) < length_bytes
        state = sha512(vcat(state, encode_uint64_little_endian(counter)))
        append!(stream, state)
        counter += 1
    end

    resize!(stream, Int(length_bytes))
    return stream
end

function xor_bytes(payload::AbstractVector{UInt8}, keystream::AbstractVector{UInt8})
    length(payload) == length(keystream) || throw(ArgumentError("payload and keystream lengths must match"))
    return UInt8[xor(payload[index], keystream[index]) for index in eachindex(payload)]
end

function encode_bucket(bucket::FalconBucket, seed::UInt64, epoch::Integer)
    keystream = derive_keystream(length(bucket.payload), seed, epoch, bucket.id, bucket.tag)
    return xor_bytes(bucket.payload, keystream)
end

active_bucket_id(qtv::QuantumTunnelVector) = qtv.permutation[qtv.active_slot]
active_bucket(qtv::QuantumTunnelVector) = qtv.buckets[active_bucket_id(qtv)]

function refresh_working_vector!(qtv::QuantumTunnelVector)
    qtv.working_vector = encode_bucket(active_bucket(qtv), qtv.seed, qtv.epoch)
    return qtv
end

function build_qtv(privkey::AbstractVector{UInt8}; seed::Integer = DEFAULT_SEED, epoch::Integer = 0)
    length(privkey) == FALCON1024_PRIVATE_KEY_SIZE || throw(ArgumentError("Falcon-1024 private key payload must be 2305 bytes"))
    seed >= 0 || throw(ArgumentError("seed must be non-negative"))
    epoch >= 0 || throw(ArgumentError("epoch must be non-negative"))

    payload = Vector{UInt8}(privkey)
    buckets = FalconBucket[]

    for (bucket_id, bucket_range) in enumerate(bucket_ranges(length(payload)))
        segment = payload[bucket_range]
        push!(buckets, FalconBucket(bucket_id, first(bucket_range), last(bucket_range), segment, bucket_tag(segment)))
    end

    rng = MersenneTwister(seed)
    permutation = collect(randperm(rng, BUCKET_COUNT))
    qtv = QuantumTunnelVector(buckets, permutation, 1, UInt8[], Int(epoch), SwapEvent[], UInt64(seed), rng)

    refresh_working_vector!(qtv)
    return qtv
end

function decode_working_vector(qtv::QuantumTunnelVector)
    return xor_bytes(qtv.working_vector, derive_keystream(length(qtv.working_vector), qtv.seed, qtv.epoch, active_bucket_id(qtv), active_bucket(qtv).tag))
end

function swap_active_bucket!(qtv::QuantumTunnelVector; target_slot::Union{Nothing, Int} = nothing)
    if isnothing(target_slot)
        target_slot = rand(qtv.rng, 1:length(qtv.permutation))
        while target_slot == qtv.active_slot
            target_slot = rand(qtv.rng, 1:length(qtv.permutation))
        end
    end

    1 <= target_slot <= length(qtv.permutation) || throw(ArgumentError("target_slot out of range"))
    target_slot != qtv.active_slot || throw(ArgumentError("target_slot must differ from active slot"))

    from_bucket = active_bucket_id(qtv)
    qtv.permutation[qtv.active_slot], qtv.permutation[target_slot] = qtv.permutation[target_slot], qtv.permutation[qtv.active_slot]
    qtv.epoch += 1
    refresh_working_vector!(qtv)

    event = SwapEvent(qtv.epoch, from_bucket, active_bucket_id(qtv), target_slot, Tuple(qtv.permutation))
    push!(qtv.swap_log, event)
    return event
end

function run_swap_rounds!(qtv::QuantumTunnelVector, rounds::Integer)
    rounds >= 0 || throw(ArgumentError("rounds must be non-negative"))

    for _ in 1:Int(rounds)
        swap_active_bucket!(qtv)
    end

    return qtv
end

function reconstruct_payload(qtv::QuantumTunnelVector)
    ordered_buckets = sort(qtv.buckets; by = bucket -> bucket.start_offset)
    payload = UInt8[]
    sizehint!(payload, sum(length(bucket.payload) for bucket in ordered_buckets))

    for bucket in ordered_buckets
        append!(payload, bucket.payload)
    end

    return payload
end

function bucket_manifest(qtv::QuantumTunnelVector)
    return [
        (
            id = bucket.id,
            start_offset = bucket.start_offset,
            stop_offset = bucket.stop_offset,
            length = length(bucket.payload),
            tag = bucket.tag,
        ) for bucket in qtv.buckets
    ]
end

end
