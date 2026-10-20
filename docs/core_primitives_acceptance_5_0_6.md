# Core primitives: complete Rust 5.0.6 source review

Status: qualified for five complete source files: `business_id.rs`,
`entity_graph.rs`, `naming.rs`, `time.rs` and `trace.rs` in `teaql-core/src`.
The source is Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
These files are byte-identical to 5.0.5, but their current acceptance also
requires the dependency-locked source replay and current native/imported
contracts below. Historical source hashes alone are insufficient.

This group does not accept runtime Business-ID generation/allocation, graph
Save, query trace orchestration, privacy publication or the complete G13/P7
scope. Their independent abstractions and obligations remain in place.

## Complete responsibility mapping

| ID | Complete source responsibility | Current mapping and evidence |
| --- | --- | --- |
| CP01 | Business-ID constants, all seven error codes and error fields/protocols | The four constants, every code, code/message equality, Clone, Debug, Display and empty Error source retain the complete [historical body review](business_id_api_acceptance.md). Current source replay retains all code pairs and exact error text. |
| CP02 | Definition factories and sequence bounds | Default/legacy factories preserve every field. The default profile requires six digits; other profiles admit 1–19. The native multiplication cannot overflow on that admitted range. Unknown profiles are not rejected by this core entry. |
| CP03 | Scope, request, plan, allocation and value models | Every field participates in complete structural comparisons and diagnostics. Canonical keys escape percent before pipe, in source field order. None differs from Some(empty). Plan validation only enforces initial <= maximum. Explicit request snapshots replace immutable source borrows under the existing C++ ownership adaptation. |
| CP04 | Separate Allocator, Profile and Slot contracts | Native abstract interfaces retain allocate, plan/format/validate and current/new/assign responsibilities. Current native/imported positive/negative constraints and actual dispatch exercise them. Result allocation selects an explicit Context. Shared implementations retain the source Send/Sync responsibility; this is not a C++ compile-time thread-safety guarantee. No allocation/profile implementation or persistence is inferred. |
| CP05 | Input graph operation, all five node fields and source traits | Save/Delete remain distinct. Node Clone and complete Debug retain entity type, MutationValues, optional comment, operation and ordered relation-name/node pairs. There is no invented source Default/Hash/serde/Eq protocol. The [complete historical graph review](core_graph_api_acceptance.md) remains the responsibility map. |
| CP06 | Every input-graph constructor/builder body | Entity construction consumes values and reads descriptor.name, independently of ENTITY_NAME. Repeated comments replace; empty differs from absent. Deletion is independent/idempotent. Children append with duplicates, empty names and order intact. Build consumes the builder without persistence. Native move/storage-identity, explicit foreign copy, actual User deletion and Global retention contracts run through both frontends. |
| CP07 | Entire default-table-name transformation | Only ASCII uppercase is folded, with an underscore before each uppercase character after the first character; `_data` is appended. Byte-preserving UTF-8 iteration has the same first-character predicate. No Unicode case conversion, underscore collapsing or SQL identifier normalization is introduced. Twenty-seven actual source controls cover empty/acronym/mixed names, punctuation, Unicode, NUL and long text. |
| CP08 | Timestamp tuple value and all derives/conversions | The mutable signed scalar, zero default, Copy/Clone/equality/order, Debug, transparent signed JSON and complete signed Hash input retain the [Timestamp contract](timestamp_api_acceptance.md). u64 conversion wraps; out-of-chrono-range datetime conversion falls back to epoch. Standard bucket hashes are not Rust DefaultHasher wire values. |
| CP09 | Current clock and both original Timestamp tests | Original unit tests execute unchanged. Current native/imported tests retain the original conversion/bounded-now assertions. Fresh exact-dependency replay preserves all fifteen existing clock controls and adds twenty-one range controls. The [clock disposition](constant_temporal_5_0_6.md#shared-clock-failure-repair) retains panic/exception failure and explicitly signed values. The range repair below restores valid future-clock results. BusinessClock is separate. |
| CP10 | All nine TraceKind variants and both TraceNode constructors | New/typed construction retains all four fields; new defaults to Entity. Every kind, absent/zero/maximum ID, short/Unicode/control/long text and all nine formatter options are observed through actual source and native calls. |
| CP11 | Trace Clone, structural equality and ownership | Every field is independently changed in each typed case; a clone remains equal and a changed field differs. Destination constructors implement explicit scoped Clone, while ordinary owning copy stays disabled. Current complete core-model and request/trace contracts exercise allocator propagation, real User destruction, Global retention and checked bindings. Raw Debug is not a safe log sink. |
| CP12 | Public interfaces and explicit implementation deltas | Header/import consumers execute canonical bodies. Eight missing historical consumers are registrations, not rewritten tests. Public interfaces remain unchanged. The only production function changes are the two shared-clock callers described below; new helper code does not add an ownership or service layer. |

No source declaration, method, derive, private helper or original test from the
five files is removed from this mapping. Native extra invalid enum/byte states
are not fabricated Rust inputs. Rust String's valid UTF-8 domain and the agreed
explicit Context ownership/owner-thread adaptation remain the source boundary.

## Actual source and dependency replay

The isolated [observer workspace](../tests/parity/rust_core_primitives_reference_5_0_6/Cargo.toml)
points directly at six byte-preserved historical observer files and the existing
clock driver. It links the complete frozen core crate. Registry names, versions,
sources and checksums must belong to the exact reference lock. It replaces no
historical manifest, lock, source driver or corpus.

The old shared observer has dependency drift; the older auxiliary verifier selected
chrono 0.4.44. That verifier now delegates to this locked workspace and checks
registry identities before its offline `--locked` build. This project uses chrono **0.4.45**,
rust_decimal **1.42.1** and serde_json **1.0.150** from the reference. Existing
successful historical results are retained, not relabeled as new execution.

| Actual source family | Observations per frontend |
| --- | ---: |
| Core Business ID | 1,283 |
| Core API/Timestamp Hash inputs and metadata callers | 259 |
| Auxiliary diagnostics, including all Business-ID models | 94 |
| Annotated input graph | 240 |
| Timestamp API/wire/field controls | 108 |
| Chrono-backed conversion | 64 |
| Default naming and complete Trace model controls | 162 |
| **Exact source/native or source/imported comparisons** | **2,210** |
| Existing and added clock success/failure boundaries | **36** |

The new 162-case corpus contains 27 naming inputs and 135 finite Trace inputs.
It does not manufacture a combinatorial requirement for unrelated models.
Complete typed JSON comparisons preserve cardinality, number kind and negative
zero. JSON Lines use LF framing, including literal Unicode separators in data.
No SQL, text, result or error normalization is introduced. Clock failures retain
their raw Rust panic and C++ exception diagnostics as distinct mechanisms.

## Reproduced clock range defect and repair

Source/body review found that `Timestamp::now` and `UtcDateTime::now` first used a
native `system_clock` duration in signed nanoseconds. Its representation ends
near year 2262, while the source chrono UTC range is much larger. The
[initial actual source comparison](parity/core_timestamp_clock_range_initial_5_0_6.json.gz)
preserves three inputs: 9,223,372,036,854 milliseconds succeeds in both runtimes;
the next millisecond and 2300-01-01 succeed in Rust but fail in C++ as apparently
pre-epoch clocks. This is an implementation defect, not an accepted adaptation.

The shared private helper now reads standard `timespec_get(TIME_UTC)` seconds and
nanoseconds separately. It preserves pre-epoch rejection, validates the chrono
date bound before multiplication, and returns the original nanosecond component.
Timestamp then computes milliseconds in the validated range; UtcDateTime retains
the separate components. Public APIs, Context ownership and explicit signed-value
construction are unchanged. The standard C++/libc dependency set is unchanged.

The test-only process clock also exposes `timespec_get`, so the new path receives
the same controlled time as the original Rust `clock_gettime` path. The 36 paired
controls retain the original fifteen and add nanosecond-representation boundaries,
2300, 9999, the chrono maximum, its next millisecond and signed-i64 maximum input.
Valid values compare exactly; out-of-date-range failures retain raw language
diagnostics. Source failure recognition rejects unrelated process failures.

Targeted sanitizer checks cover the new arithmetic and existing Timestamp
ownership. A minimal pinned OpenWrt/MIPS/QEMU probe verifies the new standard
clock API and eight-byte seconds ABI. It does not establish complete TEAQL
MIPS regression or physical-router execution.

## Reproduction

Build and wait for the observer/native/imported targets before running:

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_core_baselines_506 \
  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_core_primitives_reference_5_0_6/Cargo.toml --bins
cmake --preset native
cmake --build --preset native --target teaql_core_primitives_probe
python3 scripts/verify_core_primitives_source_5_0_6.py \
  --receipt /tmp/core_primitives_native_fresh.json.gz
```

Use a fresh receipt path. For imports, build `teaql_module_core_primitives_probe`
and the six historical probe targets, then pass
`--build-dir build_modules_native/modules --cpp-prefix teaql_module_`.
The module presets include the new targets. Ordinary native comparisons consume
the checked-in source corpus and require no Rust or generator invocation.

The initial imported replay exposed three absent historical probes and five
absent complete contract bodies. Their unchanged sources are now canonical
consumers. A later alias used a nonexistent auxiliary-comparator filename;
its failed CTest/JUnit result is preserved before the corrected registration.
Neither failure was a runtime semantic defect or a passing integration result.

## Terminal acceptance

The [complete acceptance receipt](parity/core_primitives_acceptance_final_5_0_6.json.gz)
records native **20/20** (the eighteen-family selection plus two BusinessClock
contracts), imported **15/15**, targeted ASan/UBSan **2/2** with leak scanning
disabled, and actual unchanged original core tests **69/69**. Both frontends
match **2,210/2,210** actual original calls and all **36** clock boundaries.
All **844** canonical consumer/fixture bodies and active module paths verify.
Pinned formatting/write/check covers **1,656** handwritten C++ files.

The initial module alias failure remains in its JUnit/log record. The acceptance
receipt also corrects one earlier diagnostic label: its temporary missing-probe
log path had already been reused for a successful replay. No failed diagnostic
is inferred from that later log; the original receipt is preserved separately.
Source results and terminal gate outcomes are unchanged by this collector fix.

Reconstructing both original clock functions and includes restores their complete
prior file hashes: all other bytes in those production files are unchanged.
The shared helper and test clock interposer are explicit additions. The generator
source tree is byte-identical to the previous qualified Java/reproduction
checkpoint, so no new Java or generated-SDK pass is claimed. Historical source
entries remain in the receipt. Current source status becomes **14 accepted /
110 pending**, without promoting G13/G17 or any complete P0-P7 phase.

No complete sanitizer, TEAQL MIPS regression or physical-device execution is
implied; the production memory/lifetime implementation is unchanged.
