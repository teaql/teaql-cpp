# Metadata descriptor API and structural Debug audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/meta.rs`. Preserve the three descriptor types and their distinct
responsibilities; metadata is neither a persisted graph nor a safe log record.

## Fields, defaults and method responsibilities

| Source type | Preserved contract and C++ adaptation |
| --- | --- |
| PropertyDescriptor | All nine fields: name, DataType, nullable, column_name, is_id, is_version, max_length, numeric_precision and numeric_scale. New defaults column to name, nullable true, role flags false and three limits None. column/not_null/id/version/with_max_length/with_numeric_precision/with_numeric_scale retain source builders with collision renaming. A role flag does not force non-null. Limits are metadata; the builder does not invent precision/scale validation. |
| RelationDescriptor | All seven fields: name, target_entity, local_key, foreign_key, many, attach and delete_missing. Both default keys are id; cardinality is to-one and both flags true. local/foreign/to_many/attached/detached/remove_missing/keep_missing preserve the independent source setters and repeated toggle order. |
| EntityDescriptor | All eight fields: name, table_name, optional data_service, ordered properties/relations, ordered audit_mask_fields, declaration flag and optional audit_value_max_len. New uses original default_table_name, no service, empty collections, undeclared policy and no maximum. table/service/property/relation/mask_fields/audit_max_length preserve builders and explicit optional None/Some(empty)/Some(0). |
| Lookup/iteration | property_by_name/relation_by_name/id_property/version_property return the first match, including duplicates. writable_properties filters only is_id, retaining separate version properties. Required lookups remain explicit C++ consumer helpers, not replacements for source optional lookup. |
| Clone/PartialEq/Eq | Allocator-aware copies and default structural comparison preserve all fields, including explicit-empty columns and declared-empty policy. Independently edited clones do not change original snapshots. |
| Debug | New meta_debug.hpp overloads use the existing DebugWriter and inherited options. Every field and source declaration order remain visible; Optional and vector shapes retain original derived formatting. core.hpp now exposes these overloads, including qualified core::write_debug. |

The source types have no serde derive. Differential JSON is an observation
envelope, not a new metadata wire protocol. Diagnostic output is raw; safe
SQL/audit policy projections retain their separate privacy responsibility.
Naming also preserves the source suffix: UserProfile defaults to
user_profile_data. No date/schema/cache implementation replaces that rule.

## Direct original evidence

The 160-input corpus calls unchanged original constructors/builders and lookups.
Ten DataTypes combine all cardinality/role/nullable flags, missing and explicit
zero/max-u32 limits, empty column/local mappings, None/Some(empty)/nonempty service,
undeclared/declared-empty/duplicate mask names and independent maximum length.
Duplicate properties and relations retain different mappings; full lookup outputs
prove first-match selection. A second copied entity changes table/policy/maximum,
while the original and unedited Clone retain every field.

Eight descriptor snapshots per input use nine source Debug modes, producing
11,520 exact formatted outputs in addition to complete lookup/equality/writable
results. The unchanged Rust oracle uses actual derived Debug and public methods;
it does not copy builder/lookup/formatting algorithms. All 160 inputs match live
original Rust on standalone native and MIPS/QEMU probes. The complete corpus is
stored in deterministic gzip; equality of decompressed JSON was checked before
removing the temporary 9.4 MB raw artifact. Its approximately 210 KB compressed
representation loses no field, snapshot or formatter output.

The three unchanged original meta.rs tests pass 3/3. Their exact inputs and
assertions are ported in original_builder_contracts inside meta_api_tests.cpp;
the latest native contract including those ports passes. Fresh standalone
ASan/UBSan with leak detection and MIPS/QEMU builds now also pass the latest
contract, including every exact-source assertion. These terminal results close
the previously outstanding cross-platform source-port check. Named CMake/CTest registration and facade exposure are now complete;
the new qualified-facade assertion requires fresh integrated acceptance below.

## Ownership and remaining named gates

The default-PMR-disabled contract checks duplicate and role lookup, writable
filtering, optional/declared policy boundaries and complete raw fields. Explicit
Global descriptor and output copies remain usable after actual User destruction.
Every nested string/container uses Global; a later copy's policy/table edits do
not affect the retained snapshot. Formatting scratch consumes no new object ID.
The header adds no dependency, singleton or alternative lifetime.

After the owning Value build finished with 219/219 native checks, core.hpp
exposure and named meta_api_contracts/meta_api_rust_parity registration were
added. A new native assertion calls qualified core::write_debug using only the
public facade. Fresh integrated acceptance completed: native full regression 224/224 and ten
selected sanitizer/MIPS checks 10/10 each, as recorded below. Downstream runtime
metadata/registry/schema/privacy acceptance remains separate; completing this
model audit cannot establish all six stages or physical-router execution.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin meta_api_oracle --locked --offline
python3 tests/parity/generate_meta_api.py \
  --rust /home/philip/shared-cargo-target/debug/meta_api_oracle
python3 tests/parity/compare_meta_api.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/meta_api_oracle
```

Use --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf' for the cross probe. The generator
and original reference are unchanged; no new production component is introduced.

## Named integration checkpoint

The fresh ASan/UBSan build with leak detection and OpenWrt MIPS/QEMU build each
completed successfully and passed all ten selected checks: the five new
metadata/scalar checks plus core model, core API (native contract and Rust
comparison), Value model and Context reference contracts. Fresh MIPS probes
match all 160 metadata, 99 Timestamp and 115 Value inputs against live original
Rust. The metadata contract includes the latest qualified public-facade
assertion; the Timestamp contract includes both exact original source tests.
The full native build and regression completed with 224/224 checks passed.
Fresh named native probes also match all 160 metadata, 99 Timestamp and 115 Value
inputs against live original Rust. These are completed integration results;
remaining stage-wide API/privacy/source-test requirements are separate.

## Complete source-surface acceptance, 2026-10-07

The review covers the entire production bodies of `teaql-core/src/meta.rs` and
`teaql-core/src/naming.rs` at the pinned commit. The table above accounts for
all nine PropertyDescriptor fields, seven RelationDescriptor fields and eight
EntityDescriptor fields. Every constructor, builder, optional lookup and writable
iterator is mapped; no source API or responsibility is deferred within this group.

The descriptor factories are the source defaults. None of the three types derives
Default, Copy, Hash, Serialize or Deserialize. Scoped allocator-aware construction
supplies Clone; structural comparison supplies PartialEq/Eq, and the three raw
Debug overloads preserve source declaration order. Collision-renamed builders
mutate an explicitly owned C++ object rather than consuming and returning a Rust
value. Optional lookups and the lazy writable view borrow the descriptor, so vector
reallocation, destruction or Context closure invalidate them. Required lookups are
additional consumer conveniences. The optional convenience column constructor
uses an empty argument for the default mapping; `.column("")` explicitly sets an
empty mapping, and scoped copies preserve it.

The refreshed corpus retains all 160 earlier inputs and adds:

- 32 direct derived-equality observations: every individual public field, an
  unchanged clone of each type, nested property/relation edits, and ordering of
  each of the three entity vectors. Before/after raw Debug accompanies each
  result. None versus Some(empty), None versus Some(0), role flags and the policy
  declaration flag remain independently observable.
- 152 direct naming observations: empty names, acronyms, existing underscores,
  digits, every ASCII prefix, non-ASCII letters, multibyte prefixes, whitespace
  and an embedded NUL. Both `default_table_name` and EntityDescriptor construction
  call the original implementation. Only ASCII uppercase characters are lowered;
  every uppercase character after the first character receives an underscore,
  and `_data` is always appended. The C++ byte traversal preserves this rule for
  the source's valid UTF-8 strings because ASCII capitals cannot occur inside a
  multibyte encoding. Arbitrary invalid byte strings are outside Rust String inputs.

All 344 observations match live original Rust on native and OpenWrt MIPS/QEMU.
The named `meta_api_contracts` and `meta_api_rust_parity` selections pass 2/2 on
native (1.40 seconds), ASan/UBSan with leak detection (2.83 seconds), and MIPS/QEMU
(3.43 seconds). The new native contract tests self-aliased mask replacement,
borrowed property/view identity, a combined ID/version role and empty iteration.
All nested allocator locations and 1,024-byte metadata payloads are checked after
actual source User deletion, using explicit Global delegation through UserContext.
The default PMR resource is disabled throughout. The unchanged source's three
builder tests are retained in the native contract and pass 3/3 in an unchanged
original-source run using the repository-local `build/rust-original-roundtrip`
Cargo target. An initial run with the shared Cargo target failed on unrelated
derive-generated APIs absent from this pinned source. No reference files were
changed: the local target passes the same command, and a freshly rebuilt oracle
there matches all 344 inputs on both native and MIPS. Acceptance uses that local
source/oracle result, not the failed shared-target run.

This accepts the two-file core metadata/naming responsibility group. Runtime
registration, schema use, SQL mappings, policy application and log privacy are
separate responsibilities and remain subject to their own acceptance. The last
complete native regression remains 281/281; the new targeted results above do
not claim a new complete regression or physical-device execution.
