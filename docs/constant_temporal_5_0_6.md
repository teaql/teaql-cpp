# Temporal constant bootstrap and clock boundaries

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: this declared bootstrap matrix passes; full G05/P3 remains partial.

## Complete generated models

Two controlled models extend the existing Priority/Platform/Label constant
fixtures. Both producers generate complete domains rather than extracted working
fragments. Original crates remain unchanged; C++ headers are regenerated and
formatted with the pinned tool. Each output records its complete file set and
model/producer/runtime provenance. The generation script now accepts explicit
fixture selection and rejects unknown names before writing outputs.

`constants_temporal_required.xml` has required Timestamp, required/optional
`time()` fields, and createTime/updateTime fields on Priority and Platform.
The authentic producer maps `time()` to String and dateTime/createTime/updateTime
to Timestamp. No new runtime dependency or guessed unsigned recipe is introduced.
The locked type table provides u64 for IDs and foreign keys; broader customized
unsigned model configurations remain a separate obligation.

## Actual original/native bootstrap

The unchanged complete original crate and generated C++ domain execute public
module assembly and `UserContext::ensure_schema`. Provider bootstrap creates the
real schema and initial data. Controlled SQL then creates empty, existing, drifted,
missing, soft-deleted and extra-row states. Every subsequent round creates and
actually destroys its UserContext and ID allocator on the retained connection.
Native observations explicitly copy into Global; its default PMR allocator is
disabled. No observer implements seeding, reconciliation or persistence itself.

The matrix has **1,728** scenarios and **2,592** observed rounds. Inputs cross
six stored states, four original module factories, one/two rounds, logging on/off,
three nonnegative process wall clocks, three independent BusinessClock values
(including -1), and frozen/advancing wall time. Each advancing round moves the
existing test-only process clock by 1,000 ms; CLOCK_MONOTONIC remains untouched.
The hook is never linked into a production package.

All outcomes match without JSON/error/timestamp normalization: complete physical
rows and SQLite value kinds, versions, ID-space rows, root identity, full errors,
public last Fix evidence and SQL/parameter/comment/purpose/audit diagnostics.
Execution timing is not part of this observer's output. There are **1,728**
successful rounds and **864** failures: 648 required-clock-field errors and 216
soft-deleted fixed-ID UNIQUE errors. The log matrix compares **11,628** entries.

The observations preserve several source boundaries:

- dateTime constant seeds use current wall time rather than XML date literals.
  A frozen clock preserves no-op versions; advancing it increments constant row
  versions as their Timestamp seeds change. BusinessClock does not replace it.
- `time()` strings retain the XML values, including optional String values.
- Existing Domain Roots are accepted without reconciling their time fields.
- In the drifted Priority case, existing created_at/updated_at zero values remain
  zero after scalar reconciliation. This checkpoint does not claim automatic
  repair of those fields during bootstrap updates.
- The caller's `last_fix_evidence()` is empty in every observed bootstrap round.
  Empty-list agreement is not evidence of published, populated graph Fix records;
  ordinary audited business saves and custom checker/behavior configurations
  retain their separate qualification.

## Optional Timestamp source failure

`constants_temporal.xml` additionally declares optional dateTime. The complete
original crate fails **four E0308** checks: create and reconcile pass a bare
`Timestamp::now()` to `Option<Timestamp>` setters for both constants. All original
outputs and exact primary diagnostics are retained. The complete C++ domain wraps
the optional value and independently compiles, which does not supply an unchanged
original runtime oracle. See the
[compiler disposition](parity/constant_temporal_source_failure_5_0_6.json).

The negative crate has its own locked compiler harness. Neither a hand-inserted
`Some(...)` nor a working fragment replaces the failing original configuration.

## Shared clock failure repair

An initial pre-epoch process-clock input reproduced the locked original's
`chrono::Utc::now()` panic. Native Timestamp::now and UtcDateTime::now previously
returned negative values. Their shared time implementations now reject negative
system-clock durations with `std::out_of_range("system time before Unix epoch")`.
Signatures, allocation domains and explicit signed Timestamp/UTC constructors
remain unchanged. Stored dates before 1970 and application-owned fixed clocks
remain legitimate values.

The separate **15** boundary controls execute the actual original Timestamp and
UTC APIs and native counterparts at -1,001, -1, 0, 1 and a modern millisecond time.
Success values match exactly. Rust panic and C++ exception are distinct language
failure mechanisms; the record retains raw exit codes and stderr instead of
claiming identical process diagnostics. Explicit -1 values remain accepted even
when the test system clock is negative. See the
[clock record](parity/clock_epoch_boundary_5_0_6.json).

## Terminal verification and reproduction

Relevant native checks pass **5/5**, including existing Timestamp and BusinessClock
contracts. Module consumers pass **2/2** on the same source observations, and all
**266** canonical consumer/fixture bodies verify. Both original domains and both
C++ domains reproduce completely. The four-error source/compiler disposition
also reproduces. Pinned format/check covers **1,133** C++ files. No producer
implementation changed; no sanitizer, MIPS or physical-device run is claimed.
See the [terminal record](parity/constant_temporal_5_0_6.json).

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_entity_references.py \
  --fixtures constants_temporal constants_temporal_required --check
python3 scripts/generate_examples.py \
  --fixtures constants_temporal constants_temporal_required --check
python3 scripts/verify_constant_temporal_source_failure.py --check
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_constant_temporal_5_0_6/Cargo.toml --bins
cmake --build --preset native --target \
  teaql_constant_temporal_probe teaql_clock_epoch_boundary_probe teaql_sample_fixed_clock
python3 tests/parity/compare_constant_temporal_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/teaql_cpp_constant_temporal_oracle \
  --cpp build_native_current/teaql_constant_temporal_probe \
  --clock build_native_current/libteaql_sample_fixed_clock.so
ctest --preset native --output-on-failure \
  -R '^(constant_temporal_rust_parity|clock_epoch_boundary_contracts)$'
```

The two module probes have the `teaql_module_` prefix and use the same clock shim.
Default constant/inheritance guards and all other phase/capability requirements
remain in force. This does not complete custom registration, concurrent conflict,
broader type/configuration or full baseline integration gates.
