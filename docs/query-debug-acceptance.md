# Public query, expression and trace Debug acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
expr.rs, query.rs and trace.rs. Original derived Debug is a public model
responsibility, separate from privacy-filtered SQL/audit/log projections.

## Production implementation

query_debug.hpp exposes twenty overloads through core.hpp. The implementation
uses the existing DebugWriter, metadata formatter and nested Value formatter.
No additional dependency, lifetime or asynchronous operation is introduced.

| Source family | Preserved types and structure |
| --- | --- |
| Expression | BinaryOp's twelve variants, ExprFunction's fourteen variants and all eleven Expr variants. Tuple/struct/list shapes, operand order, recursive arguments and SubQuery's complete entity/query metadata remain visible. |
| Query units | SortDirection's two variants and AggregateFunction's twelve variants. |
| Query helpers | NamedExpr, OrderBy, Aggregate, Slice, RelationLoad, RelationAggregate, RawSqlProjection, ObjectGroupBy, AggregationCacheOptions, StreamConfig, ContinuousPageFetchOptions and IdSetPaginationOptions. Every source field and declaration order is retained; optional boxed queries format transparently. |
| SelectQuery | All twenty-six source fields, including separate projection kinds, filter/having, slice/partition/probe options, nested relations/groups/enhancements, cache/stream/pagination configuration, raw SQL and trace chain. C++ field-name adaptations do not change diagnostic labels. |
| Trace | All nine TraceKind variants and all four TraceNode fields, including optional entity ID and complete raw comment. |

The previous private formatter duplicated metadata and emitted only non-pretty
cache-key shapes. Its two existing entry points now delegate to default public
Debug. The full formatter preserves compact/pretty, lower/upper hex, pretty hex,
width/precision, Unicode fill/alignment, explicit sign and zero-padding options
through all nested nodes. Invalid native enum values or a SubQuery without its
required payload fail explicitly; the original Rust enum cannot represent those
malformed C++ states.

## Original and ownership evidence

A previous correct public-API syntax-only fixture failed at missing write_debug
for SelectQuery. The repaired production header makes that fixture compile. The
new oracle calls actual original derived Debug for every exposed type; it does
not copy formatting or traversal algorithms. Its 120 inputs contain 27,441 exact
formatted outputs. They cover all Value variants, NaN/signed zero, Decimal scale,
integer limits, empty/recursive JSON and containers, every Expr form, all enum
variants, absent/explicit-empty/configured query fields and complete nested
metadata. All outputs match the initial standalone native probe.

The default-PMR-disabled ownership contract calls qualified core::write_debug,
checks complete unmasked diagnostics, and confirms formatting consumes no new
managed identities. It actually deletes User, rebinds an explicitly retained
Global query, checks nested allocator ownership and reproduces its complete pretty
output in another User. Source/native malformed-state boundaries have explicit
checks. The initial standalone native ownership contract passes.

The old pre-change query API probe is retained separately for comparison with the
same 980 original query/Expr inputs. Fresh named native/MIPS probes and the full
native regression will establish that compact cache keys remain byte-identical.
The compatibility wrappers do not define a new cache-key algorithm.

## Integrated acceptance checkpoints

query_debug_contracts and query_debug_rust_parity are registered. The full native
build and regression completed successfully with 226/226 checks. Fresh native
and MIPS probes match all 120 original inputs and 27,441 formatter outputs. The
saved old native query probe and new native probe both match all 980 unchanged
original query API inputs. ASan/UBSan with leak detection passes all ten selected
checks. After correcting the cross-test invocation and target-width observations
below, OpenWrt MIPS/QEMU passes all ten selected checks, including 451 key inputs.

The final fixture edit makes its u64-to-size_t stream cast explicit, resolving a
MIPS aggregate-narrowing diagnostic without changing values. Fresh selected native/sanitizer/MIPS builds now also completed successfully
after that fixture-only edit, with the same ten checks passing 10/10 in each
configuration. The earlier 226-check full suite predates only that explicit
fixture cast; production formatter and interface code are unchanged.

This closes a public diagnostic implementation gap when integrated checks pass;
it does not establish complete query execution/default/API acceptance, arbitrary
serialization error parity, all six stages or physical-router execution. Never
use raw query diagnostics as safe log projections.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin query_debug_oracle --locked --offline
python3 tests/parity/generate_query_debug.py \
  --rust /home/philip/shared-cargo-target/debug/query_debug_oracle
python3 tests/parity/compare_query_debug.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/query_debug_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Target-width key observations

The first MIPS key comparison lacked the CMake emulator argument. After fixing
that harness error, 405/451 host-width observations matched. The differences came
from fixture u64 inputs cast to platform-sized usize: the host corpus retained
u64::MAX, whereas MIPS correctly represented its low 32 bits in stream size and
other usize fields. No key/formatter production algorithm was changed in response.

The new target-width Rust wrapper prepares only typed usize inputs before calling
unchanged pinned canonical/key methods. It walks typed Expr/SelectQuery fields;
it does not alter u64 IDs, limits, durations or arbitrary JSON payloads. Both
corpora retain all 451 original inputs. The default host wrapper still reproduces
every existing 64-bit observation; the compressed 32-bit corpus records actual
original calls with target-width prepared values. This is host Rust evidence for
the 32-bit input domain, not a claim of executing Rust on MIPS.

The old pre-change formatter compiled for MIPS and the new MIPS key probe both
match all 451 target-width observations. This independently verifies unchanged
compact key bytes; changing expectations did not hide a formatter regression.
CMake passes pointer size and the explicit emulator to the Python comparator.
Use --pointer-bytes=4 and query_keys_32_oracle for a live MIPS key comparison;
the original query_keys_oracle and 64-bit corpus remain the native evidence.
