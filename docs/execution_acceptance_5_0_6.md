# G09 execution and pagination acceptance review

Current status: **qualified** after [resolved-service requalification](resolved_g09_requalification_5_0_6.md). The original EX01-EX14 contract below remains unchanged. Earlier gate counts retain their original checkpoint scope; the current requalification records the new regression, compiler and producer gates.

G09 is qualified for the complete locked execution responsibilities EX01-EX14,
with the explicit ownership, startup-timing and source-failure dispositions below.
Every requirement has complete source/body review and executed evidence. References remain
Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The [preserved previous entry](parity/execution_options_ledger_before_5_0_6.json.gz)
contains every previous G09 question, the plan and the requirement matrix. The
following dispositions retain those requirements and identify their owning source
branches and actual observations. Earlier checkpoint records remain unchanged.

## Complete responsibility review

| Requirement | Source responsibility, implementation and evidence |
| --- | --- |
| EX01: intent and authorization | The public Entity-bound purposed facade adds the Purpose frame; authorization checks comment and extracts the last Purpose. Native checked bindings and shared authorization preserve rejection before service/Policy/I/O. [Public typing](parity/execution_public_typing_5_0_6_terminal.json), [composition](composed_general_queries_5_0_6.md) and [binding controls](deferred_facet_page_5_0_6.md) cover every entry, missing/wrong Context, blank intent, Policy denial and private diagnostics. |
| EX02: typed list | Materialize query options/children, pass relation aggregates, fetch the typed list, then execute/attach facets. [Execution shapes](generated_execution_shapes_5_0_6.md) and [deferred facet/page](deferred_facet_page_5_0_6.md) preserve empty/nonempty facets, order, failures, cancellation and full attachment results. [Resource replacement](deferred_resource_replacement_5_0_6.md) distinguishes captured root resources from later facet lookups. |
| EX03: compact rows | Materialize metadata and execute compact smart-list rows without the typed list's subsequent facet attachment or entity decoding. [Grouped execution](generated_execution_shapes_5_0_6.md), [composed raw/readonly queries](composed_general_queries_5_0_6.md), [dynamic projections](dynamic_json_facets_5_0_6.md) and [result-model invariance](execution_source_result_models_5_0_6.md) preserve complete rows, NULL, errors, Policy and diagnostics. |
| EX04: first and one | First replaces the limit with one, retains offset/shape, runs the typed list and takes its first element; one aliases first. [Generated execution](generated_execution_5_0_6.md), [source/result models](execution_source_result_models_5_0_6.md) and [deferred facet/page](deferred_facet_page_5_0_6.md) cover presence/absence, facets and failures. This is not a uniqueness check. |
| EX05: count | Clear normal/expression projections, order, slice and relations; append the count alias while retaining preexisting aggregates/options. Execute compact rows and use the first alias's `try_u64`, retaining missing/non-numeric/provider errors. [Grouped execution](generated_execution_shapes_5_0_6.md), [scalar Context contracts](../teaql_runtime/tests/generated_execution_tests.cpp), [private composition](composed_general_queries_5_0_6.md) and [source/result review](execution_source_result_models_5_0_6.md) cover the transformation, conversion and complete diagnostics. |
| EX06: exists | Set limit one and clear relations; retain offset, projection, aggregates and options, then test compact-row presence. [Generated execution](generated_execution_5_0_6.md), [grouped execution](generated_execution_shapes_5_0_6.md), [composition](composed_general_queries_5_0_6.md) and [result invariance](execution_source_result_models_5_0_6.md) cover empty/nonempty, slices, provider errors and pre-I/O denial. |
| EX07: ordinary pages | Count precedes the offset/limit typed list; publish totals only after success. [Deferred ordinary pages](deferred_plain_page_5_0_6.md) and [facet composition](deferred_facet_page_5_0_6.md) compare count/root/facet waits, wakes, resume/drop, actual table failures, retry, exact totals and actual native Context deletion. |
| EX08: ID-set pages | Fetch the list first; preserve a present total, including zero, and count only when absent. [Deferred ID-set pages](deferred_id_set_page_5_0_6.md), [canonical keys/stores/build locks](query_optimization_5_0_6.md), [hot stores](execution_hot_cache_and_lookup_5_0_6.md) and [facet composition](deferred_facet_page_5_0_6.md) cover bounded builds, hit/miss/fallback, read/write errors, suspension, replacement and protected repeated execution. Original empty-IN errors remain failures. |
| EX09: continuous pages | Keep a separate cursor protocol with validated keys/boundaries, direction, expiry, seek/fallback and publication. [Continuous execution](deferred_continuous_page_5_0_6.md) includes 480 sequential scenarios and 160 immutable-Context interleavings; [hot stores](execution_hot_cache_and_lookup_5_0_6.md) add real seek SQL, between-request Policy/store changes and actual reuse. ID-set and continuous configurations remain separate rather than being merged. |
| EX10: streams | Materialize metadata and obtain the typed stream without typed-list relation/facet enhancement. [Protected stream shapes](protected_stream_shapes_5_0_6.md), [inherited streams](generated_inherited_streams_5_0_6.md), [dynamic JSON streams](dynamic_json_execution_5_0_6.md) and [full typed results](execution_source_result_models_5_0_6.md) cover supported/rejected shapes, chunk validation, decode/item errors, no/partial/full polling, privacy, release and next-request reuse. Native startup timing has the explicit disposition below. |
| EX11: slicing conveniences | All twelve complete delegate/arithmetic bodies preserve independent offset/limit replacement, unlimited, saturating page arithmetic and default size ten. [Pagination conveniences](pagination_convenience_5_0_6.md) retain 8,448 original executions, including 1,248 provider errors, boundaries and composed modifiers. Later header proofs preserve those bodies. |
| EX12: optimization configuration | The [constructor review](execution_options_5_0_6.md) adds 152 exact source/native scenarios and complete core bodies for defaults, all guards, coexistence and replacement. Existing store/seek, SQL transport and relation-planning evidence establishes the downstream branch responsibilities; setting a hint is not a physical-plan guarantee. |
| EX13: source/result separation | The [complete review](execution_source_result_models_5_0_6.md) reconciles the generic bodies with 3,328 full query/result/Policy/diagnostic observations and 64 NULL witnesses. Source selects the repository; five entries decode R; rows/count/exists ignore R after the public Entity guard. Model decoding retains its independent capability contracts. |
| EX14: Context and asynchronous cleanup | Context-owned snapshots, nested allocators and checked borrows preserve exactly two ownership scopes. All-eight foreign/expired binding controls, explicit Global copy/rebind, [deferred stages](deferred_facet_page_5_0_6.md), [suspended resources](deferred_resource_replacement_5_0_6.md) and [lookup failure operations](execution_hot_cache_and_lookup_5_0_6.md) cover wake forwarding, cancellation, unpolled handles and actual deletion. No shared owner or request-private allocation region is introduced. |

## Model and configuration responsibilities

The complete [37-method source review](parity/execution_responsibility_after_lookup_results_5_0_6.json)
contains the original bodies, including all execution wrappers and configuration
delegates. The execution bodies have no conditional branch on model field type,
inheritance, candidate status or constant status. The source model contributes
the literal repository identity; R controls typed decoding. Observable execution
branches are facet presence, ID-set page ordering, count conversion and errors,
intent guards, delegated provider protocols and streaming restrictions.

The [constructor review](parity/execution_options_source_review_5_0_6.json)
preserves current execution input hashes and adds the complete core option
constructors. The [source/result review](parity/execution_source_result_responsibility_5_0_6.json)
retains both native ownership overloads and each entry's source/decode role.
These bodies are reconciled with the actual outcomes above; inventory alone is
not the acceptance argument.

G01/G04/G05/G06/G07/G11 retain their accepted entity, inheritance, constant,
candidate, field/conversion and expression contracts. Existing scalar/temporal,
readonly, persistent dynamic-field and inherited-stream comparisons are included
in the current integration selection. They exercise those values through the
shared execution path without replacing their downstream contracts with an ID-only
observer. The cross-result observer retains full JSON, loaded flags and NULL.

Unfinished G08/G10/G12-G17 requirements remain intact: relationship-analysis
algorithms, JSON translation, complete runtime/sample/Assist workflows, broader
model qualification, metadata/privacy, indexed state, dynamic storage and trace
integration are not declared complete by accepting their execution delegates.
In particular, the quoted-Line analysis difference remains with G08; no expected
result is normalized here. Actual configured service lookup and replacement are
covered; complete application assembly remains G12. These are distinct source
responsibilities, not additional unbounded model permutations of the same G09 body.

## Explicit source and C++ dispositions

- Rust consuming/purposed wrappers map to reusable Context-bound native value
  wrappers and explicit execution snapshots. Runtime dependencies come only from
  the supplied UserContext. Copies into Global and rebinding are explicit.
- Native methods after purpose are the existing fluent extension. Missing purpose
  remains a compiler error; blank intent rejects before Policy/provider work.
  Non-Entity construction markers must regain an Entity result before execution.
- Rust generated streams defer startup until polling. Native startup is eager.
  The protected matrix preserves 7,092 raw equalities and 6,012 checked lifecycle
  dispositions, retaining exact Policy/errors and the complete source cursor's
  zero-delivery cancellation diagnostic when that native cursor already exists.
  This does not claim equal scheduling or silently remove additional records.
- Safe Rust rejects Context mutation/deletion while pending work borrows it.
  Native resource replacement, checked expiry and cancellation/delete controls
  qualify that ownership adaptation separately; they are not invented Rust runs.
- Rust panic boundaries map to native exceptions; missing runtime-service lookup
  maps to the original Graph result. Source provider errors, empty-IN failures,
  grouped count behavior and unsupported SQLite functions are preserved.
- Cursor-generated wall-time IDs/absolute expiry retain the documented format/
  validity observation in the original cursor matrix. Private values, SQL,
  business results and errors are not normalized to obtain parity.

## Current integration gates

All build, compiler, generation and test processes have exited successfully.
The [current native gate](parity/execution_current_integration_5_0_6.json.gz)
rebuilds 29 exact executables and passes **37/37** checks. It covers every execution
entry, three pagination protocols, protected Policy/privacy, full source/result
models, scalar/temporal and persistent dynamic reads, inherited streams, actual
stores, source errors and lifetime controls. The recorded CTest commands and full
per-test output identify the concrete comparison and native contracts.

[Imported modules](parity/execution_current_modules_5_0_6.json.gz) rebuild seven
consumers and pass **10/10**. The current
[compiler gate](parity/execution_current_compiler_5_0_6.json.gz) passes **1 positive
and 24 independent negative** contracts across all eight entries. The independent
producer passes [**71/71 Java tests**](parity/execution_current_java_5_0_6.json.gz)
in three suites. [Strict generation reproduction](parity/execution_current_reproduction_5_0_6.json.gz)
passes **24 workspaces / 288 files**. Earlier contract-SDK reproduction retains
its own recorded scope; it is not relabeled as a new run.

All 37 complete frozen execution methods and the current runtime input hashes
match their reviewed bodies. Pinned format/check covers 1,641 handwritten C++
files; 792 canonical consumer/fixture bodies, 1,495 module exports and both source
locks verify. There is no production, generator or generated-artifact change in
this acceptance checkpoint. Existing exact header-repair and sanitizer/lifetime
records remain applicable; no new sanitizer/MIPS run or full-suite claim is made.

The [acceptance record](parity/execution_acceptance_5_0_6.json.gz) binds all fourteen
requirements, complete prior questions, source/artifact hashes and terminal gate
outputs. G09 qualification does not complete G08/G10/G12-G17, P5/P6/P7 or the
overall migration goal. No physical-device or external-provider result is inferred.
