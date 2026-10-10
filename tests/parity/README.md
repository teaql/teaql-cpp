# Public API and behavioral completion audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

## Cross-platform probe launch contract

Every Python CTest that launches a C++ probe receives
`--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}`. An empty list runs native
executables directly. A cross list prefixes only C++ execution; `--rust` always
uses the native original-source oracle. The WNDR4300 cross build uses
`/usr/bin/qemu-mips;-cpu;74Kf` explicitly rather than relying on host binfmt or
its default CPU. The generated MIPS registration audit accounts for all 110
Python C++ probe commands, including isolated log-manager processes and the
three-probe log-formatter comparison.

Build selected probe targets before running their comparisons. A missing binary
is missing evidence, not a semantic difference. The shared observer now reports
that condition before invoking QEMU. Observations, target-width adaptations and
source baselines remain unchanged by launch configuration. Final native and
MIPS execution evidence is recorded in the module acceptance documents; command
registration alone is not a passing test or full target acceptance.
Root export names and module/source-test ports are not sufficient evidence for
stage completion. The audit must account for every public field, enum variant,
method, trait default, constant, conversion, comparison/hash capability and
serialized representation in each mapped source, including supported external
reexports such as Decimal. Allocation-domain and native-borrow adaptations must
be explicit; they cannot delete a source responsibility.

## Repairs identified on 2026-10-07

| Source contract | Current C++ evidence | Required action |
| --- | --- | --- |
| `teaql-core/src/time.rs`: Timestamp derives PartialOrd, Ord and Hash on its i64 millisecond value. | Timestamp now supplies total ordering and standard hash; native container tests compile and pass. | 67 original public-call inputs cover default, sorted values, hash scalar writes and 16,698 pairwise comparisons. |
| `teaql-core/src/meta.rs`: EntityDescriptor's audit_mask_fields builder accepts a runtime Vec of names, including an explicitly declared empty list. | The builder now accepts dynamic string ranges, snapshots into the owner and publishes fields/flag after successful copying. | 192 original public-call inputs cover policy/default/clone results. Native contracts cover failure consistency, aliasing and actual User deletion. |

See [the capability acceptance contract](../core_api_acceptance.md) for the 259
original-call comparisons and ownership boundaries. These repairs do not prove
complete model acceptance. C++ standard hashes need equal-value consistency;
the source's default hash algorithm is not a persistent wire format.

## Mutation and trace repairs

- `mutation.rs` derives PartialEq for the three distinct field-map wrappers and
  all six mutation/batch commands. Their C++ types now supply structural
  comparison. Original-call observations preserve NaN's source non-reflexive
  comparison rather than inventing total Value equality.
- The source provides Record conversions for all three wrappers and borrowed
  and consuming iteration for MutationValues. C++ exposes the map through
  `fields` and now supplies explicit wrapper/Record construction, retained and
  consuming map conversion, and direct MutationValues range iteration. Native
  contracts cover transfer, failed foreign copying and actual User deletion.
  The three roles remain distinct.
- `trace.rs` provides both untyped `TraceNode::new` (Entity kind by default) and
  `typed`. C++ now preserves both constructor flavors and their domain-aware
  copy behavior. BatchUpdateCommand also accepts dynamic field collections.

The [mutation/trace acceptance contract](../mutation_model_api_acceptance.md)
records 1,642 original public-call observations. These repairs are not a complete
list or proof of the remaining API requirements.

## CompactRow repairs

- Original `CompactRow::shared_columns()` always returns its immutable Arc
  layout, including rows built from a map and rows whose shape was changed.
  C++ now provides an owner-bound immutable layout in all published row states,
  with stable snapshots after shape changes. Same-owner copies share layouts;
  foreign copies own destination layouts. Physical-storage null checks have
  been replaced with actual layout/domain checks.
- Original `CompactRow::iter()` exposes paired column/value iteration. C++ has
  keys/values spans and now has a paired reference range, retaining ordering,
  duplicate columns and the row owner's native-borrow contract.
- The source's consuming `into_map` and same-layout Clone responsibilities need
  explicit acceptance alongside C++'s destination-domain copy API. Rvalue
  conversion and Record construction now consume only after complete preparation;
  same-domain nested data transfers, while foreign retention deep-copies.

The [row capability contract](../compact_row_api_acceptance.md) records 356
original-call sequences, including layout identity and every saved snapshot.
Native tests cover failed preparation and actual owner destruction. Complete
query/model API acceptance remains open.

## SmartList repairs

- Owning vector/range construction, push/set, local allocator-aware list moves
  and assignment, and arbitrary-range merge preserve the source's lack of Clone
  requirements on consuming elements. Foreign non-copyable sources are rejected
  explicitly; borrowed inputs snapshot into their owner before mutation.
- Newly produced mapper outputs and ordered/hash keys can be move-only.
  Borrowed callback results still explicitly copy into the destination domain.
  Single-pass input, move-only keys and actual User destruction have native
  contracts rather than being inferred from Value-only compilation.
- Set, keyed replacement and stable retain support non-assignable elements
  through noexcept construction; the source does not require an entity
  assignment operator. A separate construct-only fixture checks these paths.
- Named const/mutable iterators and slices retain borrowing. The complete six
  public fields and every generic/entity method are mapped in
  [the list contract](../smart_list_api_acceptance.md). Its 526 original-call
  inputs retain recursive metadata, optional results, duplicate behavior and
  exact entity-conversion callback order. The two unchanged original list tests
  pass independently. The separate model Debug boundary now supplies structural
  row/list output and borrowed custom element callbacks, with 169 unchanged
  original-call inputs and 3,042 exact formatting outputs.

These repairs do not complete all model or runtime acceptance.

## Entity registration, diagnostics and consuming capabilities

The [registration dispatch audit](../entity_registration_api_acceptance.md)
reproduced a source responsibility: the original macro invokes each entity's
`register_into`, while C++ bypassed its override. The negative control matched
12/42 original calls; an isolated dispatcher/default repair matched 42/42. That
repair is now in the production header, with a direct inherited default to avoid
recursive redispatch and both named checks registered.

The [BaseEntityData diagnostic audit](../base_entity_api_acceptance.md) repairs
optional Value Debug errors that previously used JSON projection. All 1,481
original inputs match native and MIPS, with exact roles, precedence and errors.
The [Entity Debug audit](../entity_debug_acceptance.md) supplies both derived
structures through core.hpp; all 5,202 formatter outputs match native and MIPS.
Their scoped ownership checks pass native, ASan/UBSan and MIPS. Raw core
information remains separate from safe log projections.

The [consuming wrapper audit](../entity_owned_api_acceptance.md) removes extra
Clone/assignment restrictions from Result, Audited and boxed relations. The
original non-Clone fixture executes actual public trait/wrapper methods. Current
production headers pass the new native contract with default PMR disabled,
including registered same-owner movement, rejected foreign non-copyable
retention and Global copying of an Optional<Value> expression after actual User
destruction. Const copy declarations now reflect payload capabilities.

Seven new named checks are registered. The previous Decimal full pipeline
completed successfully (205/205); the new full-current native consumer rebuild
and eleven selected sanitizer/MIPS checks are running. Fresh MIPS acceptance now passes 11/11, including all seven new contracts and
four relevant ownership consumers; rebuilt MIPS probes match the live original
registration, parsing and Debug corpora. Fresh ASan/UBSan with leak detection also passes the same 11/11 selection.
The final full-current native rebuild has now completed with 215/215 checks
passed, and the expanded sanitizer/MIPS selections pass 14/14 each. The [complete Entity method mapping](../entity_api_contract.md)
adds 93 original default/dynamic sequences matching native and MIPS, with every
intermediate state and exact default/no-op behavior. Its sequence probe is now registered and passes named native/sanitizer/MIPS
checks plus live original comparisons. Complete
Entity/API/default/privacy/source-test gates remain separate.

## Native UTC conversion

The [UTC Value capability](../chrono_value_acceptance.md) adapts the original
From<DateTime<Utc>> to an explicit-destination C++ system-clock factory. Negative
fractional milliseconds use floor; seconds bounds are checked before scaling.
All 64 original conversion inputs match native and MIPS, with scoped native,
ASan/UBSan and MIPS ownership checks passing. The new standalone header adds no
runtime dependency. Umbrella and named CTest registration are complete; final native regression
passes 215/215 and selected sanitizer/MIPS acceptance passes 14/14 each.

## DataType independent diagnostics

The [DataType audit](../datatype_debug_acceptance.md) repairs standalone derived
Debug that was previously available only inside TypedNull. Ten original types
compare 180 exact formatter outputs and 100 equality results. Native owner/facade
checks pass with default PMR disabled. TypedNull now delegates without a wrapped
intermediate string. Named checks pass after rebuilding: native full regression 217/217 and seven
selected ASan/UBSan with leak detection/MIPS checks 7/7 in each configuration.
Both named native and MIPS probes match all original scalar outputs. No new variants or serde are introduced.

## Owned Value input repair

The [owned Value audit](../value_owned_api_acceptance.md) restores direct storage
transfer for same-owner String/List/Object rvalues that previously selected a
borrowed copying constructor. Foreign retention still deep-copies and preserves
the source on failure, following the existing domain adaptation. All 171 actual
original constructor observations match standalone native C++. Native contracts
check storage/capacity identity, injected payload allocation failure and nested
Global retention after actual User deletion. Query IN-list factories use the
formal constructor. Named checks are registered. Fresh MIPS/QEMU acceptance passes all ten selected
checks and matches all 171 live original observations. ASan/UBSan with leak
detection also completed all ten selected checks successfully. Full native consumer rebuild and regression completed with 219/219; its
named probe matches all 171 live original owning observations.

## Complete metadata model audit

The [metadata contract](../meta_api_acceptance.md) maps all fields, defaults,
builders, lookups and derived traits of the three original descriptors. New
meta_debug.hpp overloads retain their full raw shapes and inherited formatter
flags. All 160 original public inputs and 11,520 formatting outputs match native
and MIPS. Scoped ownership checks pass native/ASan/UBSan/MIPS; the three unchanged
source tests pass; fresh standalone native, ASan/UBSan with leak detection and
MIPS/QEMU contracts also pass all exact C++ source ports. Named registration and umbrella exposure are complete after the Value
consumer build passed 219/219. Fresh integrated acceptance, including the new qualified-facade assertion,
completed with native 224/224 and ten selected sanitizer/MIPS checks 10/10 each. Runtime privacy/schema gates remain
separate from model presence.

## Value and Timestamp public contract mapping

[Value enum API acceptance](../value_api_acceptance.md) now maps all thirteen
variants, ten TypedNull types, every From responsibility and all eight extractors.
The additional 115 actual original inputs expose optional floating results and
13,225 pairwise equalities, preserving source NaN behavior. Native matches the
corpus; the MIPS rerun applies only the documented width-changing NaN contract.
No production conversion or variant was changed.

[Timestamp API acceptance](../timestamp_api_acceptance.md) adds 99 actual original
calls spanning signed/unsigned wrapping, chrono calendar fallback, Debug flags
and transparent raw JSON type/syntax acceptance. Native/MIPS probes match; both
unchanged source tests pass and their latest ownership/source ports pass native,
ASan/UBSan with leak detection and MIPS/QEMU. The five new metadata/scalar checks are registered. Fresh native full
regression passes 224/224 and ten selected sanitizer/MIPS checks pass 10/10
each. All named native/MIPS probes match live original metadata/scalar inputs. This does not prove generic serde error
wording, complete Decimal or all-stage acceptance.

## Public query and expression diagnostics repair

The [query Debug contract](../query_debug_acceptance.md) closes the previously
reproduced missing public formatter with twenty overloads through core.hpp.
Every Expr/query/Trace type retains its original fields and inherited formatter
options. Private non-pretty cache-key entry points now delegate to the same
kernel; complete old/new key comparison remains a required acceptance gate.
The initial standalone native probe matches all 120 original inputs and 27,441
formatted outputs. Native ownership contracts pass with default PMR disabled,
qualified facade calls and retained nested output after actual User destruction.
The full native regression passes 226/226; final fixture-adjusted native and
ten selected sanitizer/MIPS checks pass 10/10 in each configuration. Fresh named
native/MIPS query Debug probes match every output. Old/new native query probes
match 980 inputs; old/new MIPS key probes match all 451 target-width inputs.
The target-width wrapper preserves every original input and calls unchanged
pinned methods; it is not a Rust MIPS execution claim. Raw diagnostics remain
separate from safe logs.

## Mutation diagnostics implementation

The [mutation Debug contract](../mutation_debug_acceptance.md) supplies all ten
original derived model capabilities: three distinct wrappers, MutationKind and
six commands, with complete fields and inherited formatter options. The correct
negative public-core compile now succeeds. The initial native probe matches 120
actual original inputs and 9,810 exact outputs. Native ownership contracts pass
with default PMR disabled and retained nested batch/snapshot/trace/output data
after actual User deletion. Named full native regression passes 228/228; fresh selected sanitizer/MIPS
checks pass 8/8 each. Named native/MIPS probes match all 120 live original inputs. Source models have no
serde derive; diagnostic observations do not invent a mutation wire protocol.

## Request generic ownership and model repair

Original DateRange<T>::new(start, end) consumes arbitrary T without a Clone bound.
The C++ allocator-aware constructor currently accepts only const T& endpoints
and calls clone_in. The earlier correct public-core construction with a scalar non-copyable
MoveOnly fixture failed inside allocator construction of a const endpoint. The
owning and allocator-aware range move repair is now implemented, with conditional
copy declarations and owner-independent equality. The public request formatter also preserves all twelve derived model
capabilities, including scalar/vector non-Clone endpoints. Preserve explicit allocator
propagation and rejection of unsupported foreign retention while restoring the
same-owner non-Clone capability. The initial native model/ownership checks pass; named sanitizer/MIPS selections
pass 10/10 each. Native full-current model regression completed with 230/230 checks and all
120 live original inputs matched; sanitizer/MIPS selections pass 10/10 each. See
[the complete request mapping](../request_api_acceptance.md). Its additional 872
original helper inputs reproduce lost diagnostic information and Between check
order; an isolated repair matches 872/872. After the 230-check boundary finished,
production diagnostics and sequential Between preparation were repaired. Named full native regression passes 232/232; twelve selected sanitizer/MIPS
checks pass 12/12 each. Fresh named helper probes match 872/872 on native/MIPS. Actual User
deletion and nested Global helper/facet retention have native ownership contracts.

## Source input-graph and presentation model gates

Original entity_graph.rs derives Debug for EntityGraphOperation and EntityGraphNode.
The builder and whole EntityGraph do not derive it; do not invent a source trait
for those wrappers. The previous correct public-core node construction followed by
production debug_string(owner, node) failed at the missing formatter.
[The input graph contract](../core_graph_api_acceptance.md) now provides both
actual derived types and owning mutation forwarding; its initial native probe
matches 240 original inputs and 12,960 exact outputs. The original source test
and exact native port pass, as do ownership contracts after actual User deletion.
Fresh full native regression passes 234/234; ten selected sanitizer/MIPS
checks pass 10/10 each, and named native/MIPS probes match all original outputs. The annotated source input graph remains separate from the runtime
identity graph and saving; node diagnostics cannot establish graph persistence.
The previously reproduced missing WebStyle formatter is repaired by
[the seven-type presentation diagnostic header](../presentation_api_acceptance.md).
The original 452 model inputs and 8,127 formatting outputs match native/MIPS.
The expanded 524-input corpus includes 72 binding sequences: pre-repair C++
matched only 452/524; repaired native/MIPS match 524/524. Generic
response projection now accepts non-contiguous records and non-copyable owned
entity arrays; borrowed Clone constraints reflect the original requirement.
Native ownership and source ports pass. Final full native regression passes
236/236; ten selected sanitizer/MIPS checks pass in each configuration and both
named probes match all 524 expanded original inputs.
Web/Xls presentation types retain their original Debug/Clone/equality/serde
responsibilities. Their existing JSON observations do not supply every trait or
complete deserialization/error acceptance. Continue those exact source contracts
under the full six-stage objective.

## Remaining audit scope

The [core query capability audit](../query_api_acceptance.md) restores arbitrary
input ranges for Expr function/logical/IN factories and SelectQuery projection.
Standalone AggregationCacheOptions factories/propagation retain the public
fields with explicit C++ collision renaming. Its 980 unchanged original calls
compare complete query/Expr state and exact thresholds. Native single-pass,
aliasing and actual User-deletion checks complement the source observations.
These repairs do not establish complete query/default/execution acceptance.

Proceed through the existing 58 responsibility groups without narrowing their
contracts. Cross-check each source item against its concrete C++ API and current
original-call/source-test evidence. Trait implementations and defaults require
semantic checks beyond exported names. Serde derives, optional/default fields,
unknown fields, wire errors and destination ownership need explicit protocol
checks wherever the source defines serialization. Source types without serde
must not be assigned an invented serde protocol.

The [extended Decimal arithmetic audit](../decimal_extended_acceptance.md)
adds 8,056 direct original-call fixtures. It repairs result scale, intermediate
panic boundaries, division power-table/constructor failures and buffered
remainder window behavior. The scalar arithmetic implementation adds no runtime
dependency. This matrix does not replace complete Decimal API acceptance.

Remaining Decimal capabilities, complete query and save entry capabilities,
transaction/stream cancellation authority, log provenance/privacy and native
resource ownership remain acceptance gates. The retained legacy SQLite path
now has an explicit opt-in compatibility disposition; the simple examples use
the independent runtime/executor/provider layers. Backend/cloud/GPU implementations
outside the agreed scope and the unchanged generator do not become new work.

Module rows remain partial until their actual requirements are proven. Passing
200 regression checks and removing missing module rows does not prove all six
stages complete.

## Decimal inline string

The 484-input decimal_inline_cases.json.gz matrix invokes original
Decimal::array_string(), including its 50 buffer panic outcomes. Source unsigned
text, raw rescaled coefficient/sign/scale and the exact fixed-buffer boundary
remain separate from signed Decimal Display/JSON. The production method returns
32 inline bytes plus one length byte; its view borrows that carrier. Native
contracts verify no ordinary heap allocation for normal formatting/copy/move,
null default PMR, stable independent copies and actual source User deletion.

Reproduce live source comparison after building the registered targets:

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin decimal_inline_oracle --locked --offline
python3 tests/parity/generate_decimal_inline.py \
  --rust /home/philip/shared-cargo-target/debug/decimal_inline_oracle
cmake --build build --target teaql_decimal_inline_tests teaql_decimal_inline_probe
ctest --test-dir build --output-on-failure -R '^decimal_inline_'
python3 tests/parity/compare_decimal_inline.py --cpp build/teaql_decimal_inline_probe \
  --rust /home/philip/shared-cargo-target/debug/decimal_inline_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'. Source fixed-buffer
panics are C++ length_error boundaries, not an unsafe memory write or heap
fallback. This capability does not establish all-stage completion.

## Auxiliary core diagnostics

The grouped BusinessId, dynamic-search and Decimal-helper Debug contract has
94 original-call inputs and 34,216 observed strings. All eighteen actual derived
model/enum/error types retain their complete field order, inherited formatting,
optional values, source Clone equality and warning serialization. The core.hpp
facade supplies the new overloads; qualified core::write_debug includes search
models. The full native regression passes 240/240 and nine selected sanitizer
and MIPS checks pass in each configuration; both named probes match 94/94 live
original inputs. Native/sanitizer/MIPS ownership contracts validate allocator
propagation and complete Global output after actual source User deletion.

```sh
ctest --test-dir build --output-on-failure -R '^core_aux_debug_'
python3 tests/parity/compare_core_aux_debug.py --cpp build/teaql_core_aux_debug_probe \
  --rust /home/philip/shared-cargo-target/debug/core_aux_debug_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'. These are raw core
diagnostics, separate from safe log projection and complete runtime privacy
acceptance. The source-test disposition completion gate remains incomplete.

## SQL executor error diagnostics

`sql_executor_error_cases.json` records 144 direct pinned Rust error observations,
each containing complete Debug output in nine modes, Display and Error::source.
The fixture covers Compile, Transport and PersistedRecord, nested compile shapes
and custom transport Debug. The native probe uses the public error_debug.hpp
writer; it does not serialize the error or apply runtime log privacy.

```sh
python3 tests/parity/compare_sql_executor_error.py \
  --cpp build-root-integration/teaql_sql_executor_error_probe \
  --rust build/rust-original-roundtrip/debug/sql_executor_error_oracle
```

## Context model protocols

`context_model_cases.json.gz` records 234 actual pinned ContextEntityRef,
ContextRootError, FixEvidenceSource and FixEvidence observations. It preserves
complete Debug in nine modes, Clone/equality, root Display/empty cause and
evidence constructor validation precedence. Public-field mutation followed by
Clone is observed directly. The models define no new wire serialization.

```sh
python3 tests/parity/compare_context_model.py \
  --cpp build-root-integration/teaql_context_model_probe \
  --rust build/rust-original-roundtrip/debug/context_model_oracle
```
