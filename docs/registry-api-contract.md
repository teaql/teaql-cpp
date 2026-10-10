# Registry and runtime module source contract

Baseline: Rust v5.0.5 commit `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
registry.rs (590 production/macro lines). The complete source body and native
registry/module/behavior/decoder boundaries are reviewed. This source group is
accepted within the explicit Context ownership adaptations below. Acceptance
of this file does not accept the complete runtime, graph-save or audit layers.

| Source responsibility | Native mapping and evidence |
| --- | --- |
| Four decoder function types/maps | Independent compact, batch, list and option maps, selected by entity type name. Replacement installs all four functions; contains consults compact only. |
| Compact decode | One fresh weak graph state, contextual row decode, checked u64 identity and entity installation. Missing/decode/identity failures remain distinct. |
| Batch decode | One shared fresh weak graph state for the whole batch; ordered decode/identity checks and installation. Earlier installations survive a later failure, as in source. |
| List decode | Reserve exact row count, decode into final loaded SmartList, install only after all rows succeed. Empty loaded list remains distinct from missing relation. |
| Option decode | Consume only the first row, propagate its decoding failure, install Some/None. Extra rows are not decoded. |
| MetadataStore | Separate descriptor lookup/enumeration, metadata logging, diagnostic capture flags and observer defaults. Query observer depends on capture_execution_metadata; mutation observer is always present. |
| EntityRegistry | Independent name membership, not descriptor or behavior lookup. |
| RequestPolicy | Five independent mutable-command permissive defaults; explicit UserContext remains available for policy state. |
| EntityDataServiceBehavior | Five independent before hooks plus empty default relation-load list, separate from RequestPolicy. |
| EntityDataServiceBehaviorRegistry | Optional behavior lookup with replace-on-registration and explicit lifetime-checked borrowing. |
| InMemoryMetadataStore | Empty Default/new, ordered descriptor map, register/with_entity replacement, Clone, MetadataStore/SchemaProvider/EntityDescriptorStore roles and complete source-shaped Debug over actual stored descriptors. |
| InMemoryEntityRegistry | Empty Default/new, duplicate registration, contains/with_entity and Clone. Native set retains the source key/value identity map's membership and ordered source-shaped Debug. |
| InMemoryEntityDataServiceBehaviorRegistry | Empty Default/new, register/with_behavior, clone of selected references and replacement. No source Debug derive exists. |
| RuntimeModule Default/Clone and public metadata | Independent metadata, entity registry, behaviors, checkers, event sinks, optional language, initial/root graphs, schema callbacks and decoders. Destination-aware copying preserves each selected scope. No source Debug derive exists. |
| entity / entity_with_behavior / descriptor | Descriptor-name registration remains separate from decoder ENTITY_NAME keys. Only typed entity registration supplies decoders; descriptor-only registration does not. |
| behavior / checker / event_sink / language | Independent registration roles, no folding into metadata or entity membership. |
| initial_graph(s) / root_graph(s) / generated_schema_bootstrap | Ordered append semantics and separate reconciliation versus create-if-absent root data. Schema callbacks stay distinct from graph registrations. |
| apply_to / into_context | Replace each Context service/graph/callback registry; optional language leaves prior language unchanged when absent. Explicit target copies adapt source ownership-consuming installation. |
| module macro | Ordered construction and entity/optional-behavior builder chain. Native typed entity/behavior builders retain this role; generator work remains outside scope. |

The source decoder registry, behavior registry and RuntimeModule derive
Default/Clone, not Debug/serde/Eq. Only the metadata and entity registries derive
Debug. Source Arc copies are adapted to explicit lifetime domains and checked
resource borrows; they never silently extend User data. Output arrays containing
descriptor pointers do not own those descriptors. MetadataStore's entity borrow
is distinct from SchemaProvider's independently cloned descriptor snapshot.

Native apply_to prepares all destination copies before installing them. This
supports explicit deep copies and allocation/domain failure handling rather
than moving a source module's User buffers into a different Context. The source
consumes self; the native const module may be reused as an ownership adaptation.
Scope transfer, thread confinement, optional callback lifetimes and invocation
order must still be accepted at the runtime call sites. No cross-thread User
sharing or implicit shared lifetime is inferred from source Send/Sync bounds.

## Initial review and repair history

The existing actual-method corpora separately cover registry/observer/snapshot
behavior (321 cases), module/schema order (384), typed decoder/behavior
registration (182), checker registration/orchestration (1,362), and language
installation. These are complementary scopes, not a single completed contract.
Native ownership tests reject sibling retention and Global retention of User
borrows, retain deep Global descriptor/registry copies through actual User
deletion, and preserve source-schema snapshots across metadata replacement.
Decoder tests verify ledger grouping, weak graph resolution, partial batch
installation and loaded-empty relation states. Module tests verify copying into
a new User, replacing a prior module and callback suspension/cancellation.

A runtime-facade compile fixture reproduces missing Debug for
InMemoryMetadataStore and InMemoryEntityRegistry. The repair must preserve
`InMemoryMetadataStore { entities: ... }` and the source entity registry's
ordered `{ name: name }` map shape, even though native storage is a set.
It must use the explicit output allocator and preserve metadata descriptor
diagnostics. This remains an implementation gate; no acceptance is claimed.

Current implementation headers stay unchanged during the isolated full native
build. After that build's complete current regression, derived diagnostics and
remaining generic registration/default/graph-save/policy ordering gates still
need their direct source and relevant runtime evidence. Source reading and
green narrow selections cannot accept the complete group or stages 2–6.

An independently named registry_debug_scope oracle now records actual original
Default/Clone/Debug, ordering, duplicates, replacement and SchemaProvider
snapshot behavior in eighteen input scripts and nine formatter modes. Every
recorded observation is revalidated against a second live original execution.
It includes empty/Unicode/long names, reversed registration order, duplicate
replacement and cloned registries before a later replacement. The source entity
registry's empty-name entry is retained as an empty key/value pair. This is
source evidence only: native writers/probe/parity registration remain pending,
and no C++ match or group acceptance is claimed from generating this corpus.

A matching native probe and comparison runner are prepared as unregistered
fixtures. The probe calls real registry Default/clone/register/snapshot APIs and
retains every source observation field. A syntax check currently fails at both
missing model writers, confirming that the fixture does not substitute a local
registry implementation for the absent public capability. CMake registration
and the writers remain pending; the running full integration graph is unchanged.

The present isolated full native build subsequently completes, its final refresh
reports no work, all 374 compiled dependency records are nonempty, and complete
CTest passes 301/301 in 79.05 seconds. The prepared probe is unregistered and its
two missing writers are therefore outside that green suite. Those capabilities
still need implementation and direct parity/ownership acceptance.

The two source-shaped writers are now implemented and the prepared native
probe/comparer are registered. Metadata formatting reads the actual ordered
private descriptor map; entity-registry formatting retains the ordered
name-to-name map shape over native set storage. No enumeration getter, hidden
cache, data copy or lifetime extension is introduced. Existing descriptor
formatters preserve complete nested metadata and all output uses the caller's
explicit allocator. A narrow friend grants formatting access without making
private storage public.

All eighteen observations match live original Rust on the standalone native
probe. The default-PMR-disabled native ownership contract also passes: actual
source User deletion, retained Global metadata/registry diagnostics, source-map
shape, next-User output ownership and an explicit Global diagnostic copy after
deleting that formatting User. The new consumer builds are running. Earlier
301/301 whole-native results precede this writer/header registration; selected
sanitizer/MIPS and new complete native integration remain required. The source
group and stages remain open.

The first sanitizer run caught a test-fixture lifetime error: two local PMR
diagnostic strings remained alive after their allocating User was deleted.
The test now destroys those temporaries before deletion and retains only the
explicit Global copy for the next User to resolve. After rebuilding the test,
the registry ownership, existing registry parity and new diagnostic parity
selection passes 3/3 under ASan/UBSan (2.83 seconds, leak scanning disabled) and
3/3 on OpenWrt MIPS/QEMU (1.84 seconds). The formatter checks all 609 handwritten
C++ files and whitespace checks pass. The latest full native build remains
live; the previous 301/301 checkpoint precedes the new registry writers.


## Actual multi-entity macro and module snapshots

The pinned runtime-root test named `module_macro_registers_multiple_entities`
actually invokes the macro with one type. Its faithful port is retained. Sixteen
additional inputs now invoke the unchanged Rust `module!` macro directly with
multiple types, both registration orders, repeated descriptor/type keys,
entity/behavior pairs, mixed bare and paired entities, and trailing commas.
Long and Unicode behavior names exercise independent payload ownership.

The oracle records original installation, Clone/into_context, later module
replacement and unchanged previously installed Context snapshots. The native
counterpart uses the real typed RuntimeModule builder chain, explicit Global
behavior owners and independently materialized Users. It does not reproduce
registry or decoder algorithms. All 182 earlier observations remain unchanged;
all 198 inputs match live original Rust, and source outputs are checked twice.
The observation generator can reproduce the complete corpus.

The native lifetime contract additionally deep-copies a source User module into
Global, deletes the source User, decodes through the retained typed registry and
checks descriptor/type-name separation. A later module change does not alter
installed behavior snapshots. Applying an empty module removes metadata,
entity membership, behavior and decoder registrations independently. Default
PMR remains disabled in this contract. Fresh selected consumer builds complete;
the decoder/module selection passes 2/2 under ASan/UBSan with leak scanning
disabled (0.81 seconds) and 2/2 on OpenWrt MIPS/QEMU (1.26 seconds). The current
full native regression remains required. This proves the scoped macro and
module snapshot contracts, not complete runtime graph-save or governance parity.

## Initial and root graph registration composition

The graph oracle now calls all four original RuntimeModule registration entries:
initial_graph, initial_graphs, root_graph and root_graphs. Thirty-two inputs
interleave individual and bulk registration, reversed/repeated input, empty
extensions and all four graph operations. Public Context graph slices preserve
each collection's exact order and duplicates independently. Complete nested
payloads, repeated/empty relations, absent/empty comments, dirty-field boundaries
and original snapshots remain in the observations.

Clone/apply_to/into_context observations verify unchanged installed snapshots
after later registration. The native probe explicitly retains a Global module,
deletes its source User and materializes another User from that retained module.
It uses production registration and graph copying; it does not implement a
local bootstrap or graph saver. All 432 previous graph inputs are unchanged,
and all 464 inputs match live original Rust. Original observations are checked
twice by generate_module_graph_scope.py. Named graph ownership/parity selections
pass 2/2 under ASan/UBSan with leak scanning disabled (19.89 seconds) and 2/2 on
OpenWrt MIPS/QEMU (11.25 seconds). Full native consumer acceptance remains open.

The complete source-body review also traces checker installation through the
typed module/registry copy test, optional language through the module preference
test, and event-sink installation through the real Context event pipeline.
Independent sink-registry copying verifies duplicate dispatch, first-error
ordering, Global retention after source User deletion and rejection of User
retention across domains. Generated schema callbacks retain their own ordered
collection and lazy/cancellation scope tests. These are distinct responsibilities;
none is accepted as generic graph-save or audit/privacy completion.

## Integrated review and target-width recovery

The full 572-task native build completes, followed by a two-task graph-probe
refresh and a no-work final build. All 375 compiled dependency records are
nonempty, and the full native suite passes 302/302 in 77.68 seconds. The broader
twenty-check registry/module/checker/language/event/callback selection passes
20/20 under ASan/UBSan with leak scanning disabled (35.45 seconds).

The first corresponding MIPS selection passes 19/20. Audit event comparison
feeds a 64-bit Rust usize observation into a 32-bit native size_t parser, causing
the probe to reject it. The audit runner now uses the existing native count
preflight and the original oracle's actual u32 parsing for schema/seed counts.
Six inputs reject before execution; all 1,421 representable audit outputs match
live original Rust without changing recorded output or privacy fields. This
establishes the input-width boundary, not execution of Rust on MIPS.

The initial runner revision also passed non-event inputs into the event-only
preflight, which requires a kind field. It is corrected to validate only the
three count-bearing event constructors. No production code or corpus changes
are needed for this repair. The complete twenty-check MIPS selection then passes
20/20 in 14.27 seconds. Final rebuilt probe checks and a full native regression
after the runner repair are required before recording source-group acceptance.

## Source-group acceptance, 2026-10-08

The final native refresh reports no work, all 375 compiled dependency records
are nonempty, and the full final-state suite passes 302/302 in 67.69 seconds.
The twenty-check source-role composition selections pass on sanitizer and MIPS;
the rebuilt audit probe subsequently passes its affected check on both targets.
Its live MIPS comparison retains all 1,421 representable outputs and verifies
six rejections against the original oracle's actual u32 parsing. Final probe
refreshes report no work; all 130 sanitizer and 324 MIPS dependency records are
nonempty. Formatting, upstream baseline and whitespace checks pass.

The acceptance accounts for every item in the complete registry.rs body:
four decoder map/signature/dispatch roles, five independent registry/policy/
behavior interfaces, all three in-memory stores, RuntimeModule's ten fields,
all registration and installation entries, and both recursive/terminal macro
branches. Only the metadata/entity stores derive Debug; all five concrete
registry/module types retain Default/Clone. No source serde, Eq or Hash protocol
is invented. RuntimeModule has no Debug derive. Metadata borrow enumeration
and SchemaProvider's independently cloned descriptor snapshot remain distinct.

Explicit allocator arguments replace implicit owned allocation. Resource
borrows replace Arc lifetime extension; User data cannot be retained by Global
or a sibling without an allowed explicit copy. Global resources may be selected
by separate Users through checked borrows. Installation prepares destination
copies, replacing source ownership consumption. Thread confinement and bounded
allocation failures are the documented C++ ownership adaptations.

Generic graph saving, mutation governance, provider cancellation, complete
event/log privacy and physical-router execution have their own source groups
and stage gates. They are not claimed by accepting the registry file. The full
six-stage objective remains active.
