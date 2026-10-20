# G15 source and model acceptance review

Current status: G15 partial. The later complete-source audit reopens LS10 for
three-name typed-row decoding: a loaded native member alias originally yielded
no value. The [actual witness](parity/shared_state_three_name_alias_gap_final_5_0_6.json.gz)
preserves that failure. The later
[runtime-first checkpoint](typed_member_alias_runtime_5_0_6.md) qualifies the
metadata-aware lookup with native5/module4/sanitizer2 and 130 actual-source
comparisons per frontend. The subsequent
[generated continuation](generated_member_aliases_5_0_6.md) now emits the binding
and reproduces all 78 SDKs. Native44/module32/sanitizer3, 260 actual generated
comparisons per profile, Java80 and the complete193 compiler contracts pass.
The later [typed-batch/source review](generated_load_state_bulk_5_0_6.md)
accepts all nineteen original test responsibilities with complete ordinary and
inherited batches, owner-thread checks and actual deletion. Source status is
19 accepted/105 pending. Five unchanged direct quoted-FK differences remain
outside the original unquoted test fixture and keep G15 partial. The earlier
qualification below retains its original exercised scope
and the documented C++ ownership, owner-thread and diagnostic adaptations. Builds pass for
36 native and 27 module targets; native 41/41 and imported 29/29 checks pass.
The complete compiler gate passes 54 positive and 139 negative contracts.
The [terminal acceptance record](parity/shared_load_state_acceptance_5_0_6.json.gz)
retains the previous ledger, every requirement and the final evidence audit.
This review retains all fourteen
[requirements](shared_load_state_requirements_5_0_6.md).

References are Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. The active branch is not the baseline.

## Requirement dispositions

LS01 follows the complete original parent-first collector, validation and revision
bodies. The [78 direct metadata comparisons](shared_load_state_metadata_5_0_6.md)
exercise their successful and failing branches independently of frontend parsing.
`emitFieldLayout` emits that actual field order, canonical/storage/physical aliases,
relation names and revision; it adds no alternate numbering algorithm. C++ Java
wide-metadata tests and the retained Rust School revision remain additional,
separately scoped evidence.

LS02-LS06 follow all production `FieldLayout`, `LoadedSnapshot` and `LoadState`
methods. Construction validates complete geometry and alias mappings. Projection,
fully-loaded construction, mutable legacy conversion and immutable updates retain
their distinct branches. The current 44/427 original corpora and native/imported
contracts cover fixed/overflow boundaries, named selections, runtime-only
serialization, errors and component sharing. The 80-case initialized-cache matrix
adds actual primary/secondary identities, no-op reuse, dead hints and saturation.
Cache fingerprints are lookup keys; their collision buckets compare complete
state equality. They are not exposed as a portable wire format.

LS07-LS09 keep readonly type definitions, persistent values and availability
separate. A schema does not make a value loaded. Schema replacement resets row
memoization while preserving names/relations; value changes do not change shape.
The 176-case carrier matrix, 69-case cached u64 schema-hash matrix, 401-case public
CompactRow matrix and initialized-cache matrix cover these bodies. Raw initialized
diagnostics retain their actual Rust/C++ representations rather than a normalized
claim of text equality. The shared core snapshot/layout/row implementation bytes
still match the qualified initialized-cache checkpoint.

LS10 follows the original entity template and all relevant macro conditions:
indexed versus legacy layouts, load-state field present/absent, runtime state
present/absent, native versus physical aliases and scalar versus object fields.
Generated C++ entities always install indexed metadata and delegate common state
behavior through Model. Manual carriers retain the core defaults and explicit
legacy LoadState paths. They are not silently forced into generated ownership.
Constructor/setter/hydration behavior is covered by G01's ordinary, optional,
inherited and recursive-attachment matrices; direct core defaults remain separate.

LS11 follows the complete typed reconstruction body: remove trusted loaded/dirty
metadata independently, materialize the checker view, restore only the requested
boundaries, run typed logic and merge actual changes. The sparse before/after
comparison, required-property checks and materialization errors remain in shared
runtime code. The current Checker implementation bytes still match G02/G03's
accepted checkpoint; its generated models/fixes and private runtime behavior are
not replaced by a local observer implementation.

LS12 follows the original inline/shared row carrier, lazy ledger initialization,
per-key original dictionary, loaded-view replacement, composition precedence and
committed clearing. The 56 public original-row cases supplement the complete
current runtime-model and private graph/governance/save replays. Public returned
snapshots own independent values; availability geometry does not own row values.
Original rows strip query-only loaded relation sidecars before becoming originals.
The shared ledger, each view's original row and per-key dictionary remain distinct.

LS13 uses the actual decoding/service boundaries. The
[loaded graph matrix](loaded_row_graph_5_0_6.md) compares 2,032 full original
attachments and errors. [Typed source/result execution](execution_source_result_models_5_0_6.md)
adds 3,328 complete cross-model requests and 64 NULL/omission controls. The
[6,336-case composition matrix](dynamic_json_facets_5_0_6.md) observes loaded flags,
readonly values, persistent Value/NULL, compact facet metadata, Policy and SQL.
Typed aggregate and governed durable Save retain independent original replays.
These establish the availability/sidecar boundary without accepting every storage,
aggregate, intent or privacy responsibility of G08/G16/G17.

LS14 uses the existing ownership adaptation throughout: Global metadata and explicit
User result pools, allocator-aware nested data, explicit destination retention,
managed identity checks and actual Context deletion. Native/imported kernel,
generated carrier, recursive attachment, graph and aggregate lifecycle controls
exercise the real implementations. No new lifetime code or production C++ body
changes in this review; historical targeted sanitizer evidence remains scoped to
its unchanged implementations, with leak scanning disabled.

## Supported configurations and original limitations

| Configuration | Applicable source/evidence disposition |
| --- | --- |
| Ordinary and all scalar/optional field families | G01's 1,624 ordinary/relation and 1,548 optional observations cover generated getter/setter/eval and availability behavior. The layout algorithm has no scalar-type-specific numbering branch. |
| Wide layouts and model names | Direct metadata reaches 514 fields; kernel boundaries reach 141 and initialized layouts cover 2/65/130. Long/Unicode/NUL names, ordering and distinct equal metadata are retained in their explicit API scopes. |
| Ancestors, fieldless intermediates and redeclarations | G04 and the 512 constructor observations cover actual inherited entities. The direct collector matrix adds one-through-five ancestor numbering and compatible/incompatible overrides. No independent child-layout numbering is introduced. |
| Context roots, clocks, Business IDs and constants | The accepted G02/G03/G05 adapters use the same generated indexed Model. Their fixes mark actual repaired fields and retain separate mutation intent. Their full allocation/bootstrap/storage protocols keep their capability owners. |
| Scalar aliases and forward/reverse relations | Original macro alias dispatch, ordinary selection/removal, JSON alias/hydration and inherited graph matrices retain scalar-versus-object FK availability and explicitly installed reverse views. |
| Readonly and persistent fields together | Readonly `_` schema/value data and persistent `#` definitions/selections never acquire fixed slots. Real generated query/Save/stream and facet matrices retain source presence, NULL and unsupported attachment boundaries. |
| Manual/unindexed entities | Core defaults have no snapshot and do not claim dynamic carrier support. Legacy LoadState conversion remains explicit; generated construction does not bypass Context requirements to emulate an unmanaged manual entity. |
| Original generation/compiler failures | G01's original types/optional entity/Checker path harness preserves broken request/runtime scaffolds. Metadata API validation is not malformed KSML frontend acceptance. The pending invalid wire-profile decision remains G12/G14 and is not silently excluded from overall generator parity. |

An embedded relation containing only durable data need not retain its sidecar
through the original ordinary map path. Compact generated facets do not perform
the typed dynamic loader, and readonly relation subqueries can retain the original
two-column SQLite error. Those outcomes are preserved as source boundaries/errors;
they are not described as successful persistent facet hydration. Derived-property
declarations likewise do not turn NotLoaded into NULL or authorize persistence.

Rust's Arc/Weak/Mutex/OnceLock ownership and concurrency correspond to the approved
Context ownership/owner-thread adaptation. C++ wrapper destruction does not drop
the arena allocation; explicit destruction/Context close invalidates checked IDs.
Context pools do not share User state through a Global cache. The separate
ownership observation and raw diagnostics preserve those differences.

## Current combined gate

`verify_shared_load_state_integration_5_0_6.py` records the finite relevant
selection: 41 native tests and 29 imported checks, with their exact commands,
targets and terminal executable hashes. It includes the complete core corpora,
generated carrier/default/Checker paths, inherited constructors, actual SQLite
graph/durable/readonly/facet composition, lifecycle contracts and the native
generated API compiler gate. Imported checks execute the canonical bodies and
existing actual-source observers; the two frontend selections are not claimed
to contain identical registrations.

Both builds finish before execution. The G12 cohort also reaches terminal state
before these test selections start (native 51/51 and modules 40/40 pass).
No corpus partition,
normalization, timeout increase or discarded failure is permitted.

- [Native integration](parity/g15_integrated_native_5_0_6.json.gz).
- [Imported integration](parity/g15_integrated_module_5_0_6.json.gz).
- [Direct metadata comparison](parity/shared_load_state_metadata_5_0_6.json.gz).
- [Earlier kernel module checkpoint](parity/g15_source_module_checkpoint_5_0_6.json.gz).

The final audit rechecks recorded executable hashes, locked source hashes,
78 direct metadata outcomes, unchanged producer sources and all 77 SDK
destinations/3,468 files against their strict reproduction checkpoint. Java 75
is reused only with that unchanged-source evidence; it is not a new Java test run.
The full pinned formatter and check pass 1,649 files without changing timestamps.

During integration, the independent provider contract identified one missing
module export. Its repair adds only the existing `detail::mutation_audit_value`
name to the facade/catalog/lookup; removing that one declaration/entry restores
the previous interface bytes exactly. Provider source, header and contract
bodies remain identical. The new native/imported provider contracts and isolated
module import pass. G15's recorded module29 executable hashes remain unchanged;
the additive name does not alter their existing declarations or runtime bodies.
See the [export-only proof](parity/dynamic_provider_export_only_proof_5_0_6.json.gz).

G15 acceptance does not close G08/G12-G14/G16/G17, P1/P7 or the full generator
goal. Those retain their own query/storage/authority/audit/intent and release
requirements and the pending compatibility decisions.

## Whole `load_state.rs` source disposition

The current reconciliation accepts the complete 500-line
`teaql-core/src/load_state.rs`, including its two internal lifetime tests, in the
124-file source ledger. It does not automatically accept `eval.rs`, `query.rs`,
the macro implementation or the four adjacent core integration-test files.
The previous generic pending responsibility is retained in the ledger entry.
This is an explicit whole-file review following G15 acceptance, rather than a
promotion based on the existence of its declarations or earlier examples.

| Source responsibility | Reviewed C++ implementation and evidence |
| --- | --- |
| FieldLayout storage, `from_generated` and every getter | `load_state.hpp/.cpp` retain immutable entity/revision, ordered fixed names, canonical/member/physical aliases and independent relation names. Constructor guard order and messages cover invalid slots, duplicate names/indexes/mappings, noncanonical mappings, empty/ambiguous aliases, mandatory id/version, incomplete mappings and typed-member disagreement. The 44 unchanged original observations and current `field_layout_contracts` cover the public contract; source `Arc<str>` maps to a scoped immutable string borrow. |
| Snapshot fields, Clone and Debug | Bits, optional overflow, named selections, layout identity and readonly schema remain separate. Same-Context published copies borrow immutable component nodes; updates detach the changed component. Explicit destination copies retain nested payloads and metadata in the destination. Before publication, allocator-aware value copies can copy owned containers; literal Arc reference counts and allocation addresses are not the C++ contract. Actual initialized diagnostics are retained separately, including the Context-pool versus source Mutex/Weak representation. |
| `into_shared` fingerprint and full equality | `SnapshotPool::publish` hashes unordered geometry and optional schema, probes the matching collision bucket with complete equality, then performs miss cleanup and bounded admission. Fingerprints are private lookup keys; native `size_t`/standard hashing is not presented as the source u64 wire format. Full equality still requires layout identity, fixed bits and optional overflow/named/schema equality, including absent versus present components. |
| `into_shared` liveness, saturation and synchronization | The 512 limit counts retained hints per layout. Live hits precede cleanup; misses remove dead checked references, count all collision entries and return valid new state after saturation. Original poisoned Mutex recovery belongs to Rust's concurrency mechanism; C++ uses the approved Context owner-thread protocol and no shared Global pool of User results. Initialized-cache cases exercise 511/512/513/520 shapes, dead hints and primary/secondary identities. |
| `fully_loaded`, `projection` and snapshot getters | Fixed slots below 64, all-bits boundary, lazy overflow above 64, optional named selections and optional schema preserve their separate meanings. Projection ignores unknown/derived slots; full fixed loading does not imply relation/dynamic selection. The unchanged 427-case corpus includes fixed/overflow boundaries, row conversion, schema and legacy-load-state observations. |
| `with_property_schema` and `with_dynamic_property_definitions` | Internal attach/replace/clear and public attach retain fixed/named availability. Equal schema reuses the published state; changed schema follows the same interner. Row initialization and schema replacement are covered by the initialized-cache matrix and `readonly_schema` contract. Schema declarations never manufacture actual readonly values. |
| `with_dynamic_fields` | Owner validation precedes comparison. Compare selected codes before constructing names; value-only edits preserve identity. Genuine selection replacement removes only old `#` names and retains fixed bits, overflow and relation markers; an empty resulting set becomes absent. The 176 unchanged original carrier cases and `dynamic_carrier_contracts` cover selection, NULL/delete, schema and row-isolation paths. This does not accept every responsibility in the separate dynamic-field source or test files. |
| `is_loaded`, `with_loaded` and private `mark` | Relation and `#` names take priority over fixed aliases. Unknown mutation names preserve the source error; no-ops preserve identity. Fixed bits, lazy overflow and named sets detach independently, and removing the last auxiliary entry restores absence. Source body review plus the unchanged snapshot corpus and current pointer/identity contracts cover these branches. |
| `PartialEq` / `Eq` | Actual layout identity, bits and complete optional set/schema contents determine equality; equal metadata from another layout cannot silently replace the original identity. The initialized matrix and explicit incompatible-layout contract cover reuse and refusal. |
| Both internal lifetime tests | The unchanged original tests pass 2/2: bounded live hints and saturated row sharing; dead-hint pruning without retaining states/layouts. Native contracts exercise bounded admission, shared auxiliary nodes after explicit snapshot destruction, dead-reference rejection, explicit destination copies and actual UserContext deletion. Arc last-drop and arena close remain the documented ownership adaptation, not identical destruction timing. |

The new isolated observation binary compiles nine **byte-unchanged observation
functions** from the historical driver against the actual frozen core. A second
binary directly includes the unchanged dynamic-carrier driver. Production
algorithms are neither copied into the fixtures nor patched in the reference.
Both are automatic bin targets in the existing exact-dependency core workspace;
its Cargo manifest and lock are unchanged. The existing exact-dependency cache
observer is rebuilt independently. Registry versions/checksums must be a subset
of the frozen reference lock before comparisons run.

The [native replay](parity/load_state_source_native_5_0_6.json.gz) and
[imported replay](parity/load_state_source_module_5_0_6.json.gz) each reproduce
**727 exact semantic observations**: 44 layouts, 427 snapshot/schema/load-state
cases, 176 dynamic carriers and 80 initialized caches. The additional ownership
observation is checked against its two explicit expected outcomes. Randomized
or address-bearing cache diagnostics are stored as actual outputs and are not
normalized into an equality claim. Inputs and historical observations are
unchanged.

The [terminal gates](parity/load_state_source_gates_5_0_6.json.gz) record successful
no-work rebuilds, **native 8/8**, **imported 8/8** and the two original tests.
The [source disposition receipt](parity/load_state_source_acceptance_5_0_6.json.gz)
binds this whole-file table, previous disposition, current replays and final hashes.
The source replay verifies that all ten shared core/runtime implementation hashes
still match the earlier G15 responsibility review. There is no production C++,
module export, generator, generated-artifact or CTest-registration change in this
reconciliation. Earlier compiler/Java/lifetime evidence retains its stated scope;
no fresh full compiler, sanitizer, MIPS or physical-device run is claimed.

The separate `load_state_allocations.rs` remains pending: its cold/warm
allocation-count and byte-budget assertions require their own C++ measurement
mapping. Pointer sharing and semantic replay alone do not establish those exact
allocation properties. The other adjacent integration-test files likewise keep
their own whole-file source obligations.

Reproduce after building the existing probes and activating the toolchain:

```sh
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_core_baselines_506 \
  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_core_primitives_reference_5_0_6/Cargo.toml \
  --bin load_state_reference_5_0_6 --bin dynamic_carrier_reference_5_0_6
python3 scripts/verify_initialized_snapshot_cache_5_0_6.py \
  --cpp build_native_current/teaql_initialized_snapshot_cache_probe
python3 scripts/verify_load_state_source_5_0_6.py \
  --receipt /tmp/load_state_source_native_fresh.json.gz
python3 scripts/verify_load_state_source_5_0_6.py \
  --build-dir build_modules_native/modules --cpp-prefix teaql_module_ \
  --receipt /tmp/load_state_source_module_fresh.json.gz
```

Use fresh receipt paths; the verifier refuses to overwrite historical evidence.
This accepts one complete source disposition, bringing the total to **15 accepted
/ 109 pending**. Full G13/SQ03, SQ05/P7 and the generator goal remain open.

The later [allocation responsibility review](load_state_allocations_5_0_6.md)
reproduces and fixes per-row temporary metadata in same-shape CompactRow merge.
It qualifies the separate allocation test file with all four actual original
tests, the complete measurement matrix, current semantic/lifetime gates and
fresh 727-case replays of both frontends. That checkpoint brings the source
ledger to 16 accepted/108 pending; the earlier 15-file receipt remains historical.
For current source replay after that intentional `row.cpp` change, add
`--implementation-review docs/parity/load_state_allocation_implementation_review_5_0_6.json.gz`
to the verifier commands above. The original strict hash guard remains active.
