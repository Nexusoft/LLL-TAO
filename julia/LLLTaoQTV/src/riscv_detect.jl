"""
    RiscvCapabilities

Struct capturing RISC-V CPU feature detection results at library load time.
On non-RISC-V platforms all boolean fields are `false` and numeric fields
hold sensible host defaults.
"""
struct RiscvCapabilities
    is_riscv          :: Bool    # true if running on any RISC-V hart
    has_v_ext         :: Bool    # true if "V" vector extension available
    has_zba           :: Bool    # true if Zba bit-manipulation available
    has_zbb           :: Bool    # true if Zbb bit-manipulation available
    hart_count        :: Int     # number of harts (RISC-V cores) detected
    xlen              :: Int     # register width: 32 or 64
    cache_line_bytes  :: Int     # detected or assumed cache-line size in bytes
    detected_at       :: String  # ISO-8601 timestamp of detection
end

"""
    _parse_cpuinfo_riscv() -> NamedTuple

Read `/proc/cpuinfo` on Linux and extract RISC-V ISA fields.
Returns `(hart_count, has_v_ext, has_zba, has_zbb)`.
Falls back to Sys.CPU_THREADS on any read/parse error.
"""
function _parse_cpuinfo_riscv()
    hart_count = Sys.CPU_THREADS
    has_v_ext  = false
    has_zba    = false
    has_zbb    = false

    try
        text = read("/proc/cpuinfo", String)
        isa_lines = filter(l -> occursin(r"^\s*isa\s*:", l), split(text, '\n'))
        if !isempty(isa_lines)
            hart_count = length(isa_lines)
            # The V extension appears as 'v' in the ISA string (e.g. "rv64gcv")
            has_v_ext = any(line -> occursin(r"rv\d+[a-z]*v[a-z_]*", lowercase(line)), isa_lines)
            has_zba   = any(line -> occursin("_zba", lowercase(line)), isa_lines)
            has_zbb   = any(line -> occursin("_zbb", lowercase(line)), isa_lines)
        end
    catch
        # Non-Linux or unreadable cpuinfo — use Sys fallback
    end

    return (hart_count=hart_count, has_v_ext=has_v_ext, has_zba=has_zba, has_zbb=has_zbb)
end

"""
    detect_riscv_capabilities() -> RiscvCapabilities

Detect RISC-V CPU features at library load time.

On Linux RISC-V, reads `/proc/cpuinfo` to parse ISA strings and detect the
"V" vector extension, Zba, and Zbb bit-manipulation extensions.
On all other platforms, boolean fields are `false` and `xlen` defaults to 64.
"""
function detect_riscv_capabilities() :: RiscvCapabilities
    arch       = Sys.ARCH
    is_riscv   = arch === :riscv64 || arch === :riscv32
    xlen       = (arch === :riscv32) ? 32 : 64
    ts         = string(Dates.now())

    if is_riscv
        info = _parse_cpuinfo_riscv()
        return RiscvCapabilities(
            true,
            info.has_v_ext,
            info.has_zba,
            info.has_zbb,
            info.hart_count,
            xlen,
            64,   # standard RISC-V cache-line assumption in bytes
            ts,
        )
    else
        return RiscvCapabilities(
            false,
            false,
            false,
            false,
            Sys.CPU_THREADS,
            64,
            64,
            ts,
        )
    end
end

"""
    print_riscv_capabilities([io=stdout])

Print a human-readable table of detected RISC-V capabilities.
"""
function print_riscv_capabilities(io::IO = stdout)
    caps = RISCV
    W = 23   # field column width
    V = 17   # value column width
    sep = "─"^(W + 2) * "┬" * "─"^(V + 2)

    println(io, "┌" * "─"^(W + 2) * "┬" * "─"^(V + 2) * "┐")
    header = "RISC-V Capability Report"
    inner_w = W + V + 5  # total inner width of the merged header row
    println(io, "│" * lpad(header, (inner_w + length(header)) ÷ 2) * " "^(inner_w - (inner_w + length(header)) ÷ 2) * "│")
    println(io, "├" * sep * "┤")
    rows = [
        ("is_riscv",         string(caps.is_riscv)),
        ("has_v_ext",        string(caps.has_v_ext)),
        ("has_zba",          string(caps.has_zba)),
        ("has_zbb",          string(caps.has_zbb)),
        ("hart_count",       string(caps.hart_count)),
        ("xlen",             string(caps.xlen)),
        ("cache_line_bytes", string(caps.cache_line_bytes)),
        ("detected_at",      caps.detected_at[1:min(V, length(caps.detected_at))]),
    ]
    for (k, v) in rows
        println(io, "│ " * rpad(k, W) * " │ " * rpad(v, V) * " │")
    end
    println(io, "└" * "─"^(W + 2) * "┴" * "─"^(V + 2) * "┘")
end
