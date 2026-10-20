# Execution and pagination qualification matrix

The [generated result-marker review](generated_query_result_marker_5_0_6.md)
closes a public typing discrepancy: ordinary construction/conversions admit
non-Entity markers, but the complete original public purposed implementation
requires Entity for every execution entry and creation, including count/exists/
compact rows. Shared runtime, generated guards and compiler checks preserve
that distinction; unconstrained internal helpers do not imply public availability.

G09 is qualified with the explicit C++ adaptations in the
[complete acceptance review](execution_acceptance_5_0_6.md). All EX01-EX14
responsibilities have source/body and executed evidence, and the current gates
pass. Rust 5.0.6 and generator 45e remain the locked references. The
[current source review](parity/execution_responsibility_after_lookup_results_5_0_6.json) retains
37 complete original bodies, their line ranges and hashes, plus three diagnostic
scope bodies and current C++ execution inputs. Earlier reviews remain unchanged;
the later input refresh preserves the result-marker and positive stream guards,
then proves the exact shared lookup-result repair. Source identity alone does
not prove behavioral parity.

The requirement table states the current decision. Subsequent sections preserve
the sequence of earlier partial checkpoints and their then-open questions; the
acceptance review reconciles every one rather than treating those historical
statements as new gaps. G08/G10/G12-G17 and final phase exits remain independent.

| ID | Responsibility | Complete source behavior and C++ mapping | Existing executed evidence | Acceptance disposition |
| --- | --- | --- | --- | --- |
| EX01 | Purpose and authorization | The executable wrapper adds trace intent. `authorize_query` requires nonblank comment, removes the last Purpose frame and rejects missing/blank purpose. Native `EntityQuery::authorize` and derived reauthorization retain the explicit Context binding adaptation. | [Generated execution](generated_execution_5_0_6.md), [query policy](dynamic_json_policy_5_0_6.md), [private JSON intent and sinks](dynamic_json_privacy_5_0_6.md). | Qualified: complete public/authorization bodies, all-eight compiler/binding controls and protected pre-I/O intent/Policy observations; retain documented staging adaptations. |
| EX02 | Typed list | Materialize request metadata and child enhancements, pass relation aggregates separately, execute the root list, then facets, then attach facet results. Native shared list/facet operations preserve this sequence. | [Execution shapes](generated_execution_shapes_5_0_6.md), [relation analysis adapters](relation_analysis_adapters_5_0_6.md), [matching and statistics](matching_statistics_inputs_5_0_6.md). | Qualified: complete typed-list/facet sequencing, attached results, deferred errors/cancellation and captured-versus-later resource lookups. G08 owns relation-analysis algorithms. |
| EX03 | Compact rows | Materialize metadata and call compact smart-list execution. Do not apply the typed list's subsequent facet attachment or fabricate fully loaded entities. | [Relation analysis adapters](relation_analysis_adapters_5_0_6.md), [matching and statistics](matching_statistics_inputs_5_0_6.md), [numeric field SQL](query_field_sql_5_0_6.md). | Qualified: complete compact path and full raw/computed/readonly/dynamic row observations; no typed facet attachment or R decoding is fabricated. |
| EX04 | First and one | Replace the limit with one, preserve the existing offset and query shape, execute the typed list and return its first item or absence. `one` aliases `first`; it does not check uniqueness. | [Generated execution](generated_execution_5_0_6.md), [execution shapes](generated_execution_shapes_5_0_6.md), [return type](generated_return_type_5_0_6.md). | Qualified: complete first/one delegates, offset retention, empty/nonempty typed results and facet/failure/source-result composition. |
| EX05 | Count | Materialize metadata, clear normal/expression projections, ordering, slice and relations, append COUNT_ALIAS, execute compact rows and convert the first alias with `try_u64`. Keep preexisting aggregates and other options. Native shared scalar operation retains the same missing/non-numeric error. | [Execution shapes](generated_execution_shapes_5_0_6.md), [scalar Context contracts](../teaql_runtime/tests/generated_execution_tests.cpp). | Qualified: complete derived count transformation, alias conversion/errors, grouped options, source-result invariance and protected diagnostics. |
| EX06 | Exists | Materialize metadata, set limit one, clear relations and test compact-row presence. Keep offset, projections, aggregates and options. | [Generated execution](generated_execution_5_0_6.md), [execution shapes](generated_execution_shapes_5_0_6.md). | Qualified: complete exists transformation, retained slice/projections/aggregates, compact presence and protected/source-result observations. |
| EX07 | Ordinary page | Execute count before the offset/limit typed list; publish the count only with a successful list. Propagate either stage's failure. Native `GeneratedPageQueryOperation` owns the staged snapshot. | [Deferred ordinary pages](deferred_plain_page_5_0_6.md), [execution shapes](generated_execution_shapes_5_0_6.md). | Qualified: count-before-list/facet ordering, every named pending/failure/cancel/retry boundary, successful total publication and actual Context deletion. |
| EX08 | ID-set page | Fetch the offset/limit list first; retain a supplied total, including zero, otherwise count. Preserve cache-key, bounded-ID and fallback protocols through runtime services. | [Deferred ID-set pages](deferred_id_set_page_5_0_6.md), [pagination conveniences](pagination_convenience_5_0_6.md). | Qualified: list-first/present-zero/absent-total branches, bounded real stores/keys/build coordination, fallback, hot/deferred/replacement and protected execution. |
| EX09 | Continuous pages | Expose the source configuration without merging it with the ID-set protocol. Runtime services own cursor lookup, validation, seek/fallback and publication. | [Deferred continuous pages](deferred_continuous_page_5_0_6.md). | Qualified: complete separate cursor protocol, validation/expiry/seek/fallback/publication, immutable-Context interleavings and protected hot-store execution. |
| EX10 | Stream | Materialize metadata and obtain a typed stream; do not substitute typed-list relation/facet enhancement. Preserve chunk-size validation, supported scalar decoding, item errors, early close and connection reuse. Native start-time error delivery differs from Rust's lazy polling under the documented async adaptation. | [Dynamic JSON streams](dynamic_json_execution_5_0_6.md), [inherited streams](generated_inherited_streams_5_0_6.md), [return type](generated_return_type_5_0_6.md). | Qualified: complete supported/rejected shape and positive chunk contracts, typed decode/errors/cleanup/reuse and explicit source-lazy/native-startup diagnostics. |
| EX11 | Slice conveniences | `skip`/`offset_only` replace offset; `offset`/`offset_size` use page_offset; `top` uses limit; unlimited clears slice. Numbered pages saturate subtraction and multiplication; default size is ten. | [Pagination conveniences](pagination_convenience_5_0_6.md). | Qualified: all twelve complete delegate/arithmetic bodies and 8,448 original boundary/composition outcomes; current runtime input preservation and integration pass. |
| EX12 | Optimization configuration | Default/configured continuous and ID-set methods delegate to core query configuration. Top-N parent threshold delegates separately; zero selects the provider window plan. | [Execution shapes](generated_execution_shapes_5_0_6.md), [relation planning extensions](relation_planning_extensions_5_0_6.md). | Qualified: complete core constructors/five delegates, 152 original/native/module scenarios, exact defaults/guard precedence/replacement and separate physical-path evidence. |
| EX13 | Source/result model separation | Repository lookup remains the source entity; typed list/first/one/page/stream decode the result entity. Compact rows/count/exists do not decode that result type. | [Complete source/result review](execution_source_result_models_5_0_6.md), [return type](generated_return_type_5_0_6.md), [execution shapes](generated_execution_shapes_5_0_6.md). | Qualified: complete source/result bodies, all-eight typed/compact/scalar observations, full JSON/NULL/load state and current binding/retention controls. |
| EX14 | Context and async cleanup | C++ descriptions and results are explicit borrows. Snapshot nested data into the execution domain, reject foreign Contexts, forward wakes and cancel/join pending stages before releasing host memory. | [Scalar Context contracts](../teaql_runtime/tests/generated_execution_tests.cpp), deferred page/store contracts above. | Qualified: complete execution-domain copying/binding, deferred wake/cancel/join, actual deletion, checked dependency expiry and explicit source ownership dispositions. |

## Protected streaming checkpoint

The [protected stream-shape checkpoint](protected_stream_shapes_5_0_6.md) adds
13,104 actual original/native outcomes to EX10/EX12. It covers fourteen supported
or rejected shape families, positive/zero/default chunk guards, all three
pagination configurations, no/partial/full consumption, Policy, safe diagnostics
and actual subsequent connection reuse. It repairs the missing application-level
zero-chunk guard. The core SelectQuery zero value remains permitted.

There are 7,092 raw equalities and 6,012 explicit native-start/Rust-poll
dispositions. Rejected startup errors retain exact source item errors; an already
created native cursor emits the original DiagnosticStream zero-row Cancelled
record even when the lazy generated Rust wrapper never constructs that cursor.
The comparator validates complete Policy and diagnostic payloads rather than
dropping those fields. Raw observations remain retained independently.

The [current source review](parity/execution_responsibility_after_stream_shapes_5_0_6.json)
preserves all 37 complete original methods and earlier scope bodies, refreshes
the sole native guard delta and retains the complete lazy-wrapper/diagnostic
source files. The [previous ledger](parity/stream_shapes_ledger_before_5_0_6.json.gz)
archives every historical compound question. The active G09 questions now map
their remaining obligations to EX01-EX14; the deferred, replacement and protected
stream cases above are not missing work. Configured source/result reconciliation,
initialized-cache coordination and complete integration remain separate exits.

## Hot caches, lookup results and EX11 disposition

The [hot-cache and lookup checkpoint](execution_hot_cache_and_lookup_5_0_6.md)
keeps one real Context through 6,480 executions in 1,056 scenarios. Default
pagination stores are actual in-memory implementations in both languages.
The observations include initialized cache hits, cursor seek, changed Policy,
stale database membership, clearing and between-request store replacement.
All 912 denials precede SELECT/cache access and 78 original empty-IN failures
remain failures. These named protected/initialized-store scenarios are no longer
missing work; full configured source/result and assembly reconciliation remains.

The same complete entry-body review found seven repeated lookup throws, reached
through all eight public methods. A fresh unconfigured Context reproduces 32
original/native differences. Shared failure operations now return the original
Graph result instead of throwing synchronously; original query/service/provider
work is unchanged. The new source review retains every original method and proves
the complete later two-header delta against the preserved earlier checkpoint.

EX11 has no entity-specific source branch: limit and offset mutate independent
slice fields, paired aliases replace both, unlimited clears the option, and
numbered pages use saturating subtraction/multiplication. All twelve full bodies
match those native delegates and arithmetic. The existing 8,448 actual original
observations include zero/max-u64/overflow/zero-size boundaries and four compound
sequences; all 1,248 original SQLite binding errors remain errors. Native fluent
methods after purpose are the already declared language extension, not an
invented Rust public API. The exact later-header proofs preserve every one of
these bodies unchanged. This closes EX11 itself, without relabeling the older
test run as current or accepting the whole G09/P4/P7 capability.

## Complete source/result disposition

The [source/result review](execution_source_result_models_5_0_6.md) closes EX13
with 3,328 exact original/native observations and 64 explicit NULL witnesses.
Four source and result models vary independently; every typed snapshot retains
all target loaded flags, JSON and computed values. Rows/count/exists compare
complete outputs across target choices and do not decode R. Source query and
first Policy observations stay unchanged. Required string defaults under forced
SQL NULL are retained exactly rather than silently normalized.

The [complete body review](parity/execution_source_result_responsibility_5_0_6.json)
records original return_type, both native ownership overloads and the eight
source/decode responsibilities. It preserves all earlier 37 source bodies and
runtime hashes; no production or generated body changes. Current ownership and
foreign/expired binding tests and the retained 702-case source/result corpus
support the explicit Context adaptations. This closes source/result separation,
not the independent configured-model/storage capabilities or all G09/P4/P7 work.

## Historical acceptance checklist

These four original obligations are closed for G09 by the complete acceptance
review and its current gates; independent capability and P7 requirements remain.
The original wording is retained below for audit continuity.

1. Close the public typing matrix for all eight execution entries, including
   missing purpose, wrong argument types/count and foreign Context execution.
2. Reconcile source branch/model/configuration review with existing execution,
   return-type, constant/inheritance and dynamic-field acceptance. Use actual
   source observations for any demonstrated missing behavior.
3. Cover the remaining combined policy/privacy and deferred facet/page paths;
   distinguish source failure, supported outcome and documented C++ ownership
   adaptation.
4. Complete current native integration and artifact/compiler gates. Rerun
   module/lifetime checks where implementation changes require them. A full
   historical suite total or these method hashes cannot close G09 or P7.

The current native integration build is tracked separately from this source
review. No successful full-build/test result is inferred from a running process.

## Public execution typing checkpoint

The [terminal compiler record](parity/execution_public_typing_5_0_6_terminal.json)
now verifies all eight generated application entries. One positive scenario
instantiates their actual execution operations with UserContext. Twenty-four
negative scenarios independently reject missing purpose, an extra Context
argument and GlobalContext in place of UserContext for each entry. Each rejection
must mention the intended method, preventing an unrelated include failure from
passing the contract. The compiler job exited zero and its raw output and current
input hashes are retained.

This closes those three public typing categories. Foreign **UserContext** identity
is a runtime check, not a compiler refusal. The later composed checkpoint below
checks all eight entries against both a foreign and an actually deleted binding.
Dynamic blank intent, configured
model/result typing, runtime failure/privacy and combined facet/page obligations
remain separate; the native integration build has no terminal result yet.

## Composed policy, raw-query and diagnostic checkpoint

The [current composition contract](composed_general_queries_5_0_6.md) adds 4,864
actual unchanged-original/native/module/instrumented outcomes over four ordinary/
inherited private models and all eight execution entries. It closes the named
lack of shared raw/readonly/facet/grouped/missing-provider Policy/privacy coverage
for these models and ordinary/ID-set/continuous configurations. All 768 blank-intent
failures precede Policy/I/O; 1,536 Policy denials reach no SELECT callback.

Sixteen raw-statement stream/continuous-count logs exposed free-text intent before
shared runtime source-scope repairs. The complete new source review retains the
original 37 bodies and adds three original scope bodies; old reviews remain
historical. This does not qualify all configured models, protected sinks or
asynchronous interleavings, nor complete G09/P4/P7 or full native integration.

## Deferred facet/page and execution binding checkpoint

The [combined deferred matrix](deferred_facet_page_5_0_6.md) compares 1,944
unchanged-original/native outcomes for list/one/page with real ID-set/continuous
stores and count/root/facet suspension. It closes the lack of combined deferred
facet/page evidence for this ordinary Order/Person model: wake, drop/cancel,
actual table failures, typed availability, facets/totals and retries retain
complete stable diagnostics. Its 312 original empty-ID-set retry errors remain
errors. Original scheduling is gated only at the executor SPI, while original
query, decoding, store and SQLite algorithms remain unchanged.

Thirty-eight additional native scenarios actually delete UserContext at suspended
boundaries. A generated-API control rejects a foreign UserContext and an expired
source binding for each of all eight execution methods before any SQLite SELECT,
then verifies that an explicit Global copy/rebind executes successfully. These
controls preserve the C++ ownership adaptation; safe Rust does not permit deleting
a Context while a future still borrows it.

This checkpoint does not qualify all configured/protected models or suspended
registry/resource replacement, full optimization protocols or complete G09/P4/P7.
Earlier claims of pending integration describe their historical jobs; current
selected terminal evidence is attached to the new checkpoint.

## Suspended registration and resource destruction checkpoint

The [resource checkpoint](deferred_resource_replacement_5_0_6.md) closes the named
C++ adaptation gap with 143 real scenarios and 163 unchanged-data result snapshots.
Started work retains captured resources; later facet lookups resolve current
registrations and Policy. Destroyed provider/registry/store dependencies reject
checked access, clear pending callbacks and permit a replacement retry. Actual
Context close and explicit cancellation retain their separate boundaries.

The original compiler accepts mutation after dropping the future and independently
rejects three mutations with E0502 while the future still borrows Context. These
source refusals explain why the C++ adaptation is not an original Rust execution
matrix. Runtime algorithms remain unchanged. Complete EX01-EX14, configured/
protected models, stream/optimization and P4/P7 integration still require review.
