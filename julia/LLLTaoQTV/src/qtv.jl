"""
    SwapEvent

An immutable record of one bucket-swap operation.

Fields:
- `epoch`       — monotonically increasing swap counter after this swap
- `from_bucket` — bucket `id` that was active before the swap
- `to_bucket`   — bucket `id` that becomes active after the swap
- `slot`        — permutation slot that was swapped with `active_slot`
- `permutation` — full 4-element permutation after the swap
"""
struct SwapEvent
    epoch       :: Int
    from_bucket :: Int
    to_bucket   :: Int
    slot        :: Int
    permutation :: NTuple{4, Int}
end

"""
    QuantumTunnelVector

Mutable container representing the full QTV state.

Fields:
- `buckets`        — the 4 `FalconBucket`s (immutable after construction)
- `permutation`    — current ordering of bucket ids; `permutation[active_slot]` is active
- `active_slot`    — index into `permutation` that selects the active bucket
- `working_vector` — encoded view of the active bucket payload
- `epoch`          — number of swaps performed so far
- `swap_log`       — append-only log of `SwapEvent`s
- `seed`           — random seed used for reproducible permutations
- `rng`            — seeded Mersenne-Twister used for random target-slot selection
"""
mutable struct QuantumTunnelVector
    buckets        :: NTuple{4, FalconBucket}
    permutation    :: Vector{Int}
    active_slot    :: Int
    working_vector :: Vector{UInt8}
    epoch          :: Int
    swap_log       :: Vector{SwapEvent}
    seed           :: UInt64
    rng            :: MersenneTwister
end

"""
    active_bucket_id(qtv::QuantumTunnelVector) -> Int

Return the `id` of the currently active `FalconBucket`.
"""
active_bucket_id(qtv::QuantumTunnelVector) :: Int = qtv.permutation[qtv.active_slot]

"""
    active_bucket(qtv::QuantumTunnelVector) -> FalconBucket

Return the currently active `FalconBucket`.
"""
active_bucket(qtv::QuantumTunnelVector) :: FalconBucket = qtv.buckets[active_bucket_id(qtv)]

function _refresh_working_vector!(qtv::QuantumTunnelVector)
    qtv.working_vector = encode_bucket(active_bucket(qtv), qtv.seed, qtv.epoch)
    return qtv
end

"""
    build_qtv(privkey::Vector{UInt8}; seed=DEFAULT_SEED, epoch=0) -> QuantumTunnelVector

Construct a `QuantumTunnelVector` from a 2305-byte Falcon-1024 private key.

The key is partitioned into 4 `FalconBucket`s and a seeded random permutation
is generated. The working vector is encoded from the active bucket.
"""
function build_qtv(privkey::Vector{UInt8}; seed::UInt64 = DEFAULT_SEED, epoch::Int = 0) :: QuantumTunnelVector
    length(privkey) == FALCON1024_PRIVKEY_BYTES ||
        throw(ArgumentError("build_qtv: expected $FALCON1024_PRIVKEY_BYTES bytes, got $(length(privkey))"))
    epoch >= 0 || throw(ArgumentError("epoch must be non-negative"))

    buckets     = partition_privkey(privkey)
    rng         = MersenneTwister(seed)
    permutation = collect(randperm(rng, BUCKET_COUNT))

    qtv = QuantumTunnelVector(
        buckets,
        permutation,
        1,
        UInt8[],
        epoch,
        SwapEvent[],
        seed,
        rng,
    )
    _refresh_working_vector!(qtv)
    return qtv
end

"""
    swap_active_bucket!(qtv::QuantumTunnelVector; target_slot=nothing) -> SwapEvent

Swap the active slot with `target_slot` (chosen randomly if `nothing`).
Increments `epoch`, refreshes the working vector, and appends a `SwapEvent`.
"""
function swap_active_bucket!(qtv::QuantumTunnelVector; target_slot::Union{Nothing, Int} = nothing) :: SwapEvent
    if isnothing(target_slot)
        target_slot = rand(qtv.rng, 1:BUCKET_COUNT)
        while target_slot == qtv.active_slot
            target_slot = rand(qtv.rng, 1:BUCKET_COUNT)
        end
    end

    1 <= target_slot <= BUCKET_COUNT || throw(ArgumentError("target_slot out of range 1..$BUCKET_COUNT"))
    target_slot != qtv.active_slot   || throw(ArgumentError("target_slot must differ from active_slot"))

    from_bucket = active_bucket_id(qtv)
    qtv.permutation[qtv.active_slot], qtv.permutation[target_slot] =
        qtv.permutation[target_slot], qtv.permutation[qtv.active_slot]
    qtv.epoch += 1
    _refresh_working_vector!(qtv)

    event = SwapEvent(qtv.epoch, from_bucket, active_bucket_id(qtv), target_slot, Tuple(qtv.permutation))
    push!(qtv.swap_log, event)
    return event
end

"""
    run_swap_rounds!(qtv::QuantumTunnelVector, n::Int) -> QuantumTunnelVector

Perform `n` random swap rounds, mutating `qtv` in place.
"""
function run_swap_rounds!(qtv::QuantumTunnelVector, n::Int) :: QuantumTunnelVector
    n >= 0 || throw(ArgumentError("n must be non-negative"))
    for _ in 1:n
        swap_active_bucket!(qtv)
    end
    return qtv
end

"""
    reconstruct_payload(qtv::QuantumTunnelVector) -> Vector{UInt8}

Reassemble the original private key by concatenating buckets in offset order.
"""
function reconstruct_payload(qtv::QuantumTunnelVector) :: Vector{UInt8}
    ordered = sort(collect(qtv.buckets); by = b -> b.start_offset)
    out = UInt8[]
    sizehint!(out, FALCON1024_PRIVKEY_BYTES)
    for b in ordered
        append!(out, b.payload)
    end
    return out
end

"""
    swap_log_summary(qtv::QuantumTunnelVector) -> String

Return a human-readable summary of the swap log.

If `RISCV.hart_count > 1`, a note is included that `run_swap_rounds!` could
be parallelized across harts using `Threads.@spawn`.
"""
function swap_log_summary(qtv::QuantumTunnelVector) :: String
    io = IOBuffer()
    println(io, "QuantumTunnelVector swap log — $(length(qtv.swap_log)) event(s), epoch=$(qtv.epoch)")
    for ev in qtv.swap_log
        println(io, "  epoch=$(ev.epoch) bucket $(ev.from_bucket) → $(ev.to_bucket) via slot $(ev.slot) perm=$(collect(ev.permutation))")
    end
    if RISCV.hart_count > 1
        println(io, "  [RISC-V] hart_count=$(RISCV.hart_count): run_swap_rounds! could be parallelized across harts using Threads.@spawn")
    end
    return String(take!(io))
end
