# Request model API, generic ownership and Debug acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/request.rs`. Builder-side RelationAggregate/ObjectGroupBy remain
separate from their core/query counterparts; conversion crosses that boundary.

## Repaired DateRange capability

Original DateRange<T>::new consumes endpoints without a Clone bound. The old C++
const-reference constructor tried to copy a non-copyable scalar endpoint; the
correct public-core negative fixture failed inside allocator construction.
The owning overload now moves allocator-aware endpoints into the explicit owner.
Same-owner allocator-aware range movement transfers endpoints. Const endpoint and
whole-range copies are constrained by the payload's domain-copy capability.
Foreign retention of a non-copyable range rejects before consuming either endpoint;
copyable retention deep-copies into its destination. A private owner marker makes
the domain explicit, but source equality compares only start/end.

Native endpoint types obey the existing Context ownership contract: allocator-aware
owned data or non-owning/trivial scalar storage. Foreign RAII internals require
appropriately scoped resource adapters. The ordinary range move's noexcept now
follows the endpoint move capability rather than unconditionally promising it.
The original non-Clone fixture and vectors of non-Clone endpoints compile and run.
Diagnostics borrow elements and add no Clone requirement; optional/vector endpoints
retain their nested source shapes.

## Complete model and conversion mapping

| Source responsibility | C++ mapping and observations |
| --- | --- |
| Constants | COUNT_ALIAS, TYPE_FIELD and TYPE_GROUP_FIELD retain exact values. |
| FieldOperator | All eighteen unit variants retain names/copy/equality and standalone diagnostics. |
| DateRange<T> | start/end, consuming construction, explicit-domain Clone when available, structural comparison and generic diagnostics. |
| EntityReference | Consuming Value/u64 adapters and a non-Clone custom implementation exercise the public concept and allocator-aware result relocation. |
| QuerySelection | All five fields; SelectQuery construction/From, copies, complete diagnostics and into_query. |
| RelationSelection/RelationFilter | All six fields, including preserved filter metadata; construction from builder selection or core query. RelationSelection's into_query retains the source conversion contract. |
| QueryOptions | All eight fields and empty default; None/Some(empty)/nonempty comment/raw SQL and duplicate criteria remain distinct. |
| UnsafeRawSqlSegment | Trusted construction, consuming into_sql and derived raw diagnostics. The formatter is a friend reading the private string; it neither consumes nor copies the wrapper. |
| RawDynamicProperty/RawProjection | Both distinct types retain property_name/raw_sql_segment and original trusted construction. |
| Builder RelationAggregate | relation_name, alias, QuerySelection and single_result; conversion remains distinct from runtime/core aggregation. |
| FacetRequest | facet_name, relation_name, QuerySelection and include_all_facets. |
| Builder ObjectGroupBy | property_name, storage_field and QuerySelection, converted separately into core ObjectGroupBy. |
| Conversion helpers | apply_relation_selections, apply_runtime_metadata, runtime_relation_aggregates and merge_outer_filter_into_facet_aggregates invoke actual production functions and preserve complete before/after snapshots. |

request_debug.hpp exposes all twelve derived model capabilities in the request
namespace, reached through the public core facade. All original field names and
declaration order are preserved. Nine formatter modes propagate through recursive
builder/core queries, raw wrappers, options, ranges and generic endpoints. These
are raw diagnostics, not safe log projections or an invented serde protocol.
The original request types have no serde derives.

The source conversion intentionally does not apply relation_filters through
QuerySelection::into_query; the full incoming diagnostic retains those fields.
Likewise builder relation aggregates/facets remain separate from the fields
projected by apply_runtime_metadata. This audit follows the original behavior,
not a locally invented transformation of every builder field.

## Evidence and pending integration

The original-call corpus retains 120 inputs and 21,222 exact formatted outputs,
plus constants, Clone comparisons and complete consuming entity-reference results.
Expectations come from actual original constructors/helpers/derived Debug. Scalar
and vector non-Clone range fixtures are actual Rust instantiations, not a copied
algorithm. Initial native comparison matched the original 120-input model matrix;
the latest vector fixture requires fresh named acceptance.

The default-PMR-disabled native contract checks consuming non-copyable endpoints,
honest copy traits, same-owner movement, rejected foreign retention, exact long
string transfer and explicit Global copies after actual User deletion. A separate
builder contract retains nested selections/options/facets/raw strings and reproduces
complete pretty output in another User before converting to the core query layer.
The latest standalone contract passes; fresh named sanitizer/MIPS results remain
required.

request_api_contracts/request_api_rust_parity are registered. The pinned formatter check completed for all 495 handwritten C++ files. Fresh
native full regression completed with 230/230 checks; rebuilt ASan/UBSan
with leak detection and OpenWrt MIPS/QEMU selections each pass 10/10. Fresh named
MIPS probes match all 120 inputs and 21,222 formatted outputs. Fresh named native and MIPS probes match all 120 inputs and 21,222 outputs.
All three model pipeline handles returned successful terminal results. The previous completed
228-check regression predates this header and generic constructor repair.

## Remaining whole-module and stage gates

The direct helper audit below supplies original-call/panic/error evidence for
attach_facets, field operators, required value/text, filter removal and dynamic
JSON helpers. Its production repair and named integration are underway; those
terminal results remain required by the whole request audit. Existing dynamic-search normalization observations are a
different source module and cannot prove these helpers. Presentation/source-graph
traits, complete Decimal, runtime API/defaults, log privacy/provenance and source-test
acceptance still gate the unchanged six-stage objective. Physical-router execution
is not established by a cross-build/QEMU result.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin request_api_oracle --locked --offline
python3 tests/parity/generate_request_api.py \
  --rust /home/philip/shared-cargo-target/debug/request_api_oracle
python3 tests/parity/compare_request_api.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/request_api_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Direct helper audit in progress

The additional original-call matrix contains 872 inputs for all eighteen field
operators, property-to-property support, required value/text failures, dynamic
JSON conversion/operator/filter/unsigned-field access, recursive filter removal
including NaN, and facet replacement. No formatter or filter-removal algorithm
is copied into the Rust oracle. Its initial native comparison matched 160/872:
production errors lost operator/index/full raw Value details, and Between's
unspecified C++ argument evaluation order needs an explicit lower-before-upper
check. A fixture initialization mismatch for SmartList loaded state was corrected
to call the corresponding owning vector constructor; that was not a kernel repair.

An isolated request.cpp repair restores original diagnostic text and sequential
Between preparation, matching all 872 native observations. After the model integration finished with 230/230 native checks, the production
source and named request_helpers checks were updated. Native ownership contracts
pass with null default PMR, borrowed-input preservation, explicit Global dynamic
values/filtered expressions/facets and actual source User deletion. Full native regression completed with 232/232 checks; selected ASan/UBSan
with leak detection and MIPS/QEMU builds each pass 12/12. Fresh named native/MIPS
helper probes match all 872 live original inputs, closing the reproduced helper
diagnostic/order implementation gate. Remaining stage-wide contracts are separate.

## Helper integrated checkpoint

Fresh ASan/UBSan with leak detection and OpenWrt MIPS/QEMU builds completed
successfully and each passed all twelve selected checks. Fresh named MIPS helper
observations match all 872 original inputs; generic request model observations
also remain accepted. The native full-current build and 232-check regression completed successfully.
Its fresh named helper probe also matches all 872 live original inputs. All three
pipeline handles returned successful terminal results.
Production repairs preserve exact operator/index/raw-value diagnostics and the
original lower-before-upper Between failure precedence. Dynamic/filter/facet
algorithms themselves were not replaced or simplified.

## Complete request/search source review in progress, 2026-10-08

The entire pinned production bodies of request.rs and dynamic_search.rs have
been read alongside their C++ declarations/implementations. The builder models
in the table remain distinct from core SelectQuery/RelationAggregate/ObjectGroupBy
models. Relation-filter metadata is retained but intentionally not applied by
QuerySelection/RelationSelection conversion, following the original source.
Runtime metadata overwrites raw SQL, criteria, projections, grouping and child
enhancements while an absent comment leaves the base comment unchanged.
Same-entity facet aggregate merging and ordered facet replacement remain separate
helpers; neither query building nor facet attachment becomes graph-save execution.

The dynamic-search portion adds these separate responsibilities:

| Source item | Reviewed contract |
| --- | --- |
| SearchModel / SearchModels | Two trusted ordered string maps, empty model Default, scoped Clone and raw Debug. The source derives no model equality or serde. |
| DynamicSearchWarning | code/entity/clause/field_path, scoped Clone, structural Eq and raw Debug. Its sole serde protocol uses camelCase fieldPath; custom sinks see the actual path while the default structured stderr projection replaces it with omitted text and never includes submitted values. |
| DynamicSearchFilter / DynamicSearchOrder | Preserve field path/operator/JSON value and field path/descending respectively. Operator is renamed operator_name only because it is a C++ keyword. Filter has PartialEq and order has Eq; neither derives serde. |
| NormalizedDynamicSearch / DynamicSearchResult | Ordered filter/order/warning vectors versus scoped query/warnings remain distinct. Clone/Debug retain every field; the source does not derive equality or serde for these wrappers. |
| DynamicSearchError | Static diagnostic text, Clone/PartialEq/Eq/tuple Debug and exact Display. The source Error default supplies no cause; the missing C++ source() declaration was reproduced and repaired. |
| normalize_dynamic_search | Trusted model/nonzero bound validation; exact root/filter/order control shapes; supported operator validation; 1,000-value IN limits; 16-segment safe paths; relation metadata errors; original scalar type/date/decimal/safe-integer limits; ordered results and warnings emitted only after complete validation. |
| merge_dynamic_search | Normalize silently at the source fixed limit 100, bind all filters before orders, short circuit errors without warning output, copy the base scoped query, compose filters and extend ordering, then emit warnings. Warning callback exceptions propagate and are not input errors. |

The C++ implementation retains private path/decimal/scalar/warning-emission
helpers rather than merging them with request JSON heuristics. The permissive
request dynamic_json helpers are not substitutes for strict local UI search.
Raw diagnostics and warning serialization remain distinct from default safe
stderr logging. There is no source warning Deserialize contract to invent.

An actual compiler probe showed DynamicSearchError::source was absent. The
repaired default returns nullptr without allocation or a hidden cause chain.
Four new live-original error observations retain exact Display, tuple Debug,
Clone equality and the default empty Error source. All 491 existing search
observations remain; the corpus now has 495 inputs. No model generator or source
reference file is changed. The eight fresh native request/search/helper/diagnostic
checks pass 8/8 in 28.66 seconds. Native probes match all 495 search, 120 request
API and 872 helper observations against freshly rebuilt local Rust oracles.
The eight rebuilt MIPS/QEMU checks pass 8/8 in 34.56 seconds; all three MIPS
probes also match the same live original corpora. The complete native consumer
build/regression is running. The combined sanitizer selection initially failed
all 14 checks because LeakSanitizer reports a fatal ptrace/tracing incompatibility
in the newly restricted environment. A direct helper run completed all 872
observations before the identical leak-scanner shutdown error; it reported no
address/undefined-behavior defect. ASan/UBSan is rerunning with detect_leaks=0,
retaining explicit arena all-block reclamation assertions. This cannot establish
a successful leak scan and does not overwrite the failed evidence.
The ASan/UBSan rerun passes all 14 selected checks in 175.21 seconds, including
the corrected SmartList trait declarations. Leak scanning remains unavailable
under the current tracing constraints. The runtime/resource/privacy and
per-field trait/serialization acceptance must be completed before accepting this
two-file group or any stage; source reading alone is not a completion verdict.

## Independent request-field equality evidence, 2026-10-08

Fifteen further direct-original observations isolate the eight QueryOptions
fields and five QuerySelection fields, with an unchanged Clone of each type.
Every case observes actual derived equality and complete before/after raw Debug.
None versus Some(empty), an empty vector versus one empty criterion, each raw
projection/aggregate/group/facet collection and each selection's stored relation
filter/enhancement/options remain independently observable. The fixture changes
one field at a time and does not implement equality or query conversion.

The 120 existing model observations are retained; the request API corpus now
has 135 inputs. A freshly rebuilt original oracle supplies the expectations.
The native probe is compiled separately into the temporary directory against
current production archives, keeping the active native CMake build untouched.
Fresh native/live-original, named sanitizer and MIPS comparisons are running.
The first 135-input native probe matches live original Rust, and two named checks
pass on ASan/UBSan with leak scanning disabled (35.88 seconds) and MIPS/QEMU
(31.24 seconds). These results precede the further field expansion below.

The next expansion adds independent field changes for RelationSelection,
RelationFilter, RelationAggregate, FacetRequest, ObjectGroupBy, RawDynamicProperty,
RawProjection, DateRange and trusted SQL. There are now 54 equality observations:
all 43 fields across eleven structs plus one unchanged Clone per struct. The
complete 174-input native probe matches live original Rust; the two named checks
pass on ASan/UBSan without leak scanning (32.31 seconds) and MIPS/QEMU (29.15
seconds). The source enum's eighteen variants retain their earlier Debug/copy
observations; no enum or model field is omitted from this mapping.

Fourteen further search observations isolate all ten equality fields across
DynamicSearchWarning/Filter/Order/Error plus unchanged clones. Warning cases
also compare actual original serde JSON before/after each field edit, preserving
camelCase fieldPath and keeping this raw protocol separate from safe stderr
projection. The search corpus retains all previous 495 inputs and now contains
509. The fresh native standalone probe matches all 509 live original inputs;
its two named ASan/UBSan checks pass 2/2 in 1.43 seconds with leak scanning
disabled, and MIPS/QEMU checks pass 2/2 in 1.25 seconds. This preserves the
documented leak-scanner environment limitation.

The complete native build is still running. Request probe object timestamps
are current; the search probe was already built before the last test-only field
change, so it requires an explicit post-build refresh before final complete
acceptance. This is a fixture integration gate, not a source/runtime semantic
repair. No public header was changed during the ongoing build. Completing these
field observations does not accept the group without the remaining complete
source/default/trait/ownership/privacy and final integration audit.

The full native run reached 280/281 with only the anticipated stale search-probe
mismatch. Its two-task incremental refresh completed successfully, and the final
fresh full regression passes 281/281 in 69.60 seconds with no pending Ninja work.
All 174 request and 509 search observations also match live original Rust on
MIPS. This closes the named field-fixture integration gate; complete request/
search responsibility-group acceptance remains a separate audit.

## EntityReference receiver-category repair, 2026-10-08

The generic review reproduced a declaration mismatch: the EntityReference
concept checked `std::move(value).entity_id_value()`, while the adapter called
`std::forward<T>(value).entity_id_value()`. An rvalue-only custom implementation
was therefore advertised for an lvalue receiver even though the function body
could not invoke it. The constraint now checks the exact forwarded receiver used
by the adapter. This preserves Value/u64 adapters and custom owned implementations,
and also permits an explicitly supplied lvalue-only custom implementation when
actually invoked as an lvalue. It introduces no Clone, context or provider bound.

Positive/negative compile assertions cover owned, lvalue-only, const and builtin
receivers. Native runtime checks consume a non-Clone custom ID with a 4 KiB
scoped string, preserve its transferred payload address, and invoke a separate
lvalue-only adapter. They run with default PMR disabled. The unchanged original
eight-test dynamic-search integration file passes 8/8 in a fresh local Cargo run.
The source reference and model generator remain unchanged. The six selected
native checks pass 6/6 in 8.78 seconds and MIPS/QEMU checks pass 6/6 in 10.35
seconds. The refreshed ASan/UBSan selection with leak scanning disabled passes
6/6 in 17.73 seconds. The complete native consumer build/regression is running;
the prior 281/281 native
checkpoint predates this final public declaration change.

## Complete request/search responsibility-group acceptance, 2026-10-08

The complete pinned production bodies of request.rs and dynamic_search.rs are
accepted on the reviewed tables above and terminal evidence below. The source
test files and downstream runtime are separate responsibility groups.

| Acceptance dimension | Complete reviewed scope |
| --- | --- |
| Public surface | Three exact constants, eighteen FieldOperator variants, eleven request structs/all 43 fields, the generic EntityReference trait and both builtin implementations, every constructor/conversion/helper, seven distinct search models/all fields and both public normalize/merge entries. Private path, scalar, decimal, warning and ID/filter helpers are reviewed through their public effects. |
| Defaults and traits | Only QueryOptions and SearchModel derive Default, with all options absent and containers empty. Request/search Clone follows explicit destination ownership. Generic DateRange equality/copy follows payload capability; request FieldOperator retains Copy/Eq/Debug. All source PartialEq/Eq/Debug responsibilities are preserved without adding Eq to recursive query/filter wrappers or equality to SearchModel/NormalizedDynamicSearch/DynamicSearchResult. EntityReference constraints follow the actual receiver and add no Clone bound. DynamicSearchError retains exact Display/tuple Debug and default empty Error cause. |
| Conversion semantics | Builder/core aggregation and grouping remain distinct. Stored relation filters are not implicitly executed by into_query. Raw metadata overwrite, absent comment preservation, relation composition, same-entity facet filter merge, attaching facets, operator diagnostics/Between precedence, recursive AND-only filter removal and permissive dynamic JSON heuristics follow original source. Strict UI search is a separate decoder. |
| Protocol and privacy | DynamicSearchWarning alone derives Serialize, with exact code/entity/clause/fieldPath semantic JSON. The source supplies no warning Deserialize or request-model serde. Default warnings omit submitted path/value, custom sinks receive the actual model, and warnings appear only after full normalization/binding success. Raw Debug/trusted SQL helpers do not become safe audit logs. |
| Ownership | Every retained string/container/query/value/facet/model uses its explicit destination resource. Native tests disable default PMR, verify no-Clone endpoint/ID consumption, same-domain pointer transfer, rejected foreign noncopyable range retention, independent long Global copies and complete output after actual User deletion. Callbacks are synchronous borrows, never shared owners; pure builder/compiler helpers remain independent from runtime/transport/provider behavior. |

Native and MIPS match all 174 request model, 509 search and 872 helper inputs
against unchanged original code. The separate raw model corpus retains the
original request/search Debug responsibilities and formatter flags. The fresh
eight unchanged original search integration tests also pass. The final six-check
selection passes native (8.78 seconds), MIPS/QEMU (10.35 seconds) and ASan/UBSan
(17.73 seconds) with leak scanning disabled because of the documented tracing
constraint. This is not a claim of a successful current leak scan; explicit
arena block-return and lifetime assertions remain active.

The complete final native consumer rebuild and regression passes 281/281 in
70.86 seconds after the final receiver declaration change. The preceding stale
search-probe fixture mismatch was explicitly refreshed and closed before this
checkpoint. No implementation or fixture generation modifies the model generator
or the pinned source. No additional runtime dependency or ownership scope is
introduced. This accepts the two-file request/search group, not the complete
query/entity/runtime/save/log/privacy or six-stage objective. Physical-router
execution remains unverified.
