const QTV_HOOK_STATUS_OK              = Cint(0)
const QTV_HOOK_STATUS_ERROR           = Cint(1)
const QTV_HOOK_STATUS_INVALID_CASE    = Cint(2)
const QTV_HOOK_STATUS_PARITY_MISMATCH = Cint(3)

const QTV_HOOK_CASE_PARITY = 1
const QTV_HOOK_CASE_BASELINE = 0

"""
    qtv_fixture_case(case_id::Integer) -> NamedTuple

Return deterministic metadata for a supported Julia/C hook fixture case.

The returned tuple is intentionally small so it can define a research-only ABI
boundary for PackageCompiler `create_library(...)` experiments.
"""
function qtv_fixture_case(case_id::Integer) :: NamedTuple
    case_id == QTV_HOOK_CASE_BASELINE && return (
        case_id                   = QTV_HOOK_CASE_BASELINE,
        seed                      = UInt64(0x0000_0000_0000_1024),
        swap_sequence             = Int[],
        benchmark_message         = collect(codeunits("qtv hook baseline fixture")),
        expected_final_permutation = nothing,
        expected_working_vector_digest = nothing,
    )

    case_id == QTV_HOOK_CASE_PARITY && return (
        case_id                   = QTV_HOOK_CASE_PARITY,
        seed                      = UInt64(0x0000_0000_0000_1024),
        swap_sequence             = [2, 2, 2, 3, 4, 2],
        benchmark_message         = collect(codeunits("laser quantum vector handoff")),
        expected_final_permutation = [4, 2, 1, 3],
        expected_working_vector_digest =
            "16697c4aedac647ba9dc76a7db751fcec2593a723e8af280c16645856a148577" *
            "a38c8b71c0d8b56b76bfa9da45d22c2c752d9fbf40382d3b08a5a56f4b206b5b",
    )

    throw(ArgumentError("unknown QTV hook fixture case_id: $case_id"))
end

function _build_hook_qtv(fixture::NamedTuple) :: QuantumTunnelVector
    qtv = build_qtv(fixture_privkey(); seed=fixture.seed, epoch=0)

    for target_slot in fixture.swap_sequence
        swap_active_bucket!(qtv; target_slot=target_slot)
    end

    return qtv
end

"""
    qtv_run_fixture(case_id::Integer) -> Cint

Replay a deterministic research fixture and verify local invariants. This is the
primary Julia hook for fixture replay and deterministic benchmark setup.
"""
function qtv_run_fixture(case_id::Integer) :: Cint
    fixture = try
        qtv_fixture_case(case_id)
    catch error
        return error isa ArgumentError ? QTV_HOOK_STATUS_INVALID_CASE : QTV_HOOK_STATUS_ERROR
    end

    try
        qtv = _build_hook_qtv(fixture)
        active = active_bucket(qtv)
        decode_working_vector(qtv.working_vector, active, qtv.seed, qtv.epoch) == active.payload ||
            return QTV_HOOK_STATUS_ERROR

        reconstruct_payload(qtv) == fixture_privkey() || return QTV_HOOK_STATUS_ERROR

        parity_fixture = make_parity_fixture(fixture_privkey(), fixture.seed, fixture.swap_sequence)
        parity_fixture.epoch == length(fixture.swap_sequence) || return QTV_HOOK_STATUS_ERROR

        return QTV_HOOK_STATUS_OK
    catch
        return QTV_HOOK_STATUS_ERROR
    end
end

"""
    qtv_compare_parity(case_id::Integer) -> Cint

Replay a deterministic fixture and compare the resulting permutation and working
vector digest against the C++ parity fixture expectations.
"""
function qtv_compare_parity(case_id::Integer) :: Cint
    fixture = try
        qtv_fixture_case(case_id)
    catch error
        return error isa ArgumentError ? QTV_HOOK_STATUS_INVALID_CASE : QTV_HOOK_STATUS_ERROR
    end

    try
        qtv = _build_hook_qtv(fixture)

        if fixture.expected_final_permutation !== nothing &&
           qtv.permutation != fixture.expected_final_permutation
            return QTV_HOOK_STATUS_PARITY_MISMATCH
        end

        if fixture.expected_working_vector_digest !== nothing &&
           bytes2hex(SHA.sha512(qtv.working_vector)) != fixture.expected_working_vector_digest
            return QTV_HOOK_STATUS_PARITY_MISMATCH
        end

        return QTV_HOOK_STATUS_OK
    catch
        return QTV_HOOK_STATUS_ERROR
    end
end

Base.@ccallable function qtv_run_fixture(case_id::Cint) :: Cint
    return qtv_run_fixture(Int(case_id))
end

Base.@ccallable function qtv_compare_parity(case_id::Cint) :: Cint
    return qtv_compare_parity(Int(case_id))
end
