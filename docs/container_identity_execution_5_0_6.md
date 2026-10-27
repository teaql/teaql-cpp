# Generated identity roles and public execution (Rust 5.0.6)

Status: the runtime-first correction is installed and passes focused native,
module and sanitizer gates. The conditional producer correction is now installed;
its current-generation reproduction has completed. The complete
877-test native result retains its earlier checkpoint. No source disposition,
capability or P0-P7 exit criterion is promoted by this work.

The references remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. Complete original generated libraries
are extracted byte for byte from the retained container compiler evidence;
no generated entity, request, runtime or manifest is hand-repaired. The outer
observer workspace patches TeaQL dependencies to the locked source and checks
all selected registry versions/checksums before executing its binaries.

## Distinct source identities

The source keeps several responsibilities separate, even when their default
strings happen to agree:

| Identity | Source responsibilities |
| --- | --- |
| Canonical generated model identity | Q request target and explicit repository lookup arguments; generated LocalSchemaProvider and standalone repository/behavior registry keys. |
| Entity trait/runtime identity | `TeaqlEntity::ENTITY_NAME`, descriptor/key/layout identity, typed RuntimeModule registration. |
| Storage/service metadata | Independently overridden descriptor table and data-service values. |

For a model named Item with `entity="Other"`, the actual source Q request and
repository argument remain `Item`. The typed Context registers `Other`.
Generated LocalSchemaProvider accepts `Item` and returns the Other descriptor;
it does not accept `Other`. Default public queries consequently return the
source's `Graph("missing entity data service for entity: Item")` error. These
are executed outcomes, rather than an inference that the override should work.
The approved JSON payload exception does not cover this identity distinction.

## Complete original SDK and current C++ comparisons

The [initial original run](parity/container_identity_execution_original_5_0_6.json.gz)
compiled and ran all five original SDKs, but its raw SQLite setup incorrectly
named the physical relationship column `platform_id` instead of `platform`.
Those transport errors remain recorded as an observer fixture failure. The
[corrected original run](parity/container_identity_execution_original_v2_5_0_6.json.gz)
retains all original files and executes default, entity, table, service and
combined override configurations. Default/service reads return row 11; the table
override returns row 22. Entity/combined overrides retain the source missing
service error. Context registration and independent provider lookups are observed
alongside actual typed SQLite reads.

The first [C++ observer build](parity/container_identity_execution_cpp_before_5_0_6.json.gz)
failed because its raw SQLite setup omitted sqlite3.h; no C++ operation ran.
The [corrected current producer/runtime comparison](parity/container_identity_execution_cpp_before_v2_5_0_6.json.gz)
uses the pinned SQLite header, current Java generator sources/resources and
successfully built native archives. All five complete facades compile and run.
Default/table/service configurations agree. **Two configurations differ**:
C++ merges the Q/provider lookup identities into `Other`, succeeds and returns
a row where the original fails. Error messages and SQL outcomes are not
normalized to hide this discrepancy.

## Temporary runtime-first separation

The [proposal](parity/container_identity_execution_proposal_5_0_6.json.gz) adds
an optional generated `QUERY_ENTITY_NAME` only when the descriptor identity
override differs from the canonical model name. The shared EntityQuery runtime
uses that canonical identity for construction and its seven execution lookups,
with the established `ENTITY_NAME` fallback for handwritten/unaffected types.
This checkpoint left new-entity ID allocation using the trait identity; the
[later creation comparison](generated_creation_identity_5_0_6.md) proves that
this differs from the raw declared model identity used by the original source. The generated
provider and standalone registry adapters use the original canonical keys;
typed RuntimeModule registration remains unchanged.

The [temporary native comparison](parity/container_identity_execution_candidate_5_0_6.json.gz)
regenerates complete C++ facades from a copied generator and compiles them with
an owned header overlay. All **5/5** complete observations now agree, including
both original errors and the three actual row results. Every allocation remains
in the existing Context, with default PMR disabled. Producer/runtime sources and
the six native archives are hash-checked and unchanged. This is not installation,
module/lifetime acceptance or complete identity/protected-consumer qualification.

## Seven execution entries and stream failure timing

The [expanded original run](parity/container_identity_entries_original_5_0_6.json.gz)
executes list, one, page, rows, count, exists and stream for all five complete SDKs:
**35** entry outcomes. Stream opening and item polling are recorded separately.
For the two entity overrides, the source returns a stream successfully and emits
the missing-service Graph error on its first item, then ends.

The [expanded temporary C++ comparison](parity/container_identity_entries_candidate_5_0_6.json.gz)
retains **two stream differences**; the other **33/35** entry outcomes agree.
The first identity proposal reports the lookup failure while opening its stream.
Its earlier 5/5 list success is not full execution acceptance.

The [deferred-stream proposal](parity/container_identity_stream_deferred_proposal_5_0_6.json.gz)
keeps the Context-bound query and defers repository lookup/provider opening until
first item polling. Both states live in the same UserContext arena; cancellation
must detach wake forwarding and cancel any started nested work before memory is
reclaimed. A missing repository yields one application Graph error item and then
EOF. The proposal does not introduce a query-private allocation region, shared
Context ownership, another runtime argument or a new dependency.

The [expanded deferred candidate](parity/container_identity_entries_deferred_candidate_5_0_6.json.gz)
now compiles and executes all five complete generated SDKs. **35/35** execution
entries and their surrounding identity observations match the original, including
successful stream opening followed by exactly one missing-service error item.
Source and native archive hash guards pass, and the owned build directories are
removed. These are temporary overlay results, not installed-source acceptance.

The [initial lifecycle observer](parity/container_identity_stream_lifecycle_candidate_5_0_6.json.gz)
failed to compile because it referred to an absent stream alias and instantiated
StreamPoll with a reference type. No operation ran in that attempt. The
[corrected observer](parity/container_identity_stream_lifecycle_candidate_v2_5_0_6.json.gz)
changes only those observer types and passes **16/16** real SQLite lifecycle
stages. It covers both a working service and the missing-service override:
unpolled/opened futures, early cancellation, partial reads and EOF; the working
service also suspends behind a real cursor lease, then either deletes its
UserContext while pending or resumes after the lease releases.

Opening a generated stream prepares and executes no SQL. Actual Context deletion
closes SQLite exactly once with zero prepared/busy statements remaining and zero
live User arena bytes. Escaped checked handles reject access after actual deletion
while GlobalContext stays alive. Cancellation and EOF permit subsequent queries.
The observer disables the default PMR resource. Its sources/archives remain
unchanged and 41,382,214 temporary bytes are removed after terminal execution.

The [targeted sanitizer replay](parity/container_identity_stream_lifecycle_sanitize_5_0_6.json.gz)
passes 16/16 with leak scanning disabled, unchanged sources/archives and complete
owned-directory cleanup. The [provider boundary comparison](parity/container_identity_stream_provider_candidate_5_0_6.json.gz)
matches all five actual original-SDK observations, including empty EOF,
unsupported relation and provider/decode failures. A shared stream adapter
affects more than repository lookup. Installed gates are recorded separately
below; complete producer/compiler/generated-artifact checks remain required. Broader model,
Checker/Policy/audit/private logging, custom-service assembly, typed override and
full AP02/G12/G13/G14/G17/P0-P7 requirements remain independent.

## Installed runtime-first correction

After the frozen native epoch ended successfully and the original/provider and
lifetime proposals completed, the two qualified runtime headers were installed.
The permanent runtime-query fixture now checks deferred missing-service errors,
single-error EOF, late trusted Context assembly, execution snapshots unaffected
by later Q mutation, real SQLite lease suspension/resumption, early cancellation,
partial/complete reads, actual Context deletion and checked escaped handles.
Its older shape-error assertion now expects successful opening and a first error
item, matching the original compiled public-stream observer.

Current installed gates pass [native1](parity/generated_stream_runtime_native_5_0_6.json.gz),
[canonical module1](parity/generated_stream_runtime_modules_5_0_6.json.gz) and
[ASan/UBSan1](parity/generated_stream_runtime_sanitize_5_0_6.json.gz), with leak
scanning disabled. Pinned formatting/checking, complete canonical-consumer
verification, exact input hashes and selected no-work rebuilds pass. The
[installed input audit](parity/generated_stream_runtime_installed_inputs_5_0_6.json.gz)
confirms exactly three changes since the frozen full-native checkpoint: the two
runtime headers and their existing permanent fixture. No generated SDK or
producer source has been edited. The full native 877 result is not relabeled
as a full run of these later changes.

The earlier targeted temporary sanitizer replay passes 16/16 and all five
original/provider stream cases match exactly: ordinary success, empty EOF,
unsupported relation enhancement, missing physical table and malformed version.
The original includes each complete error/Debug payload and the opening/item
boundary. These results close the scoped runtime discrepancy; they do not
qualify every custom executor, thread interleaving or full generator surface.

A [conditional producer follow-up](parity/container_identity_generator_conditional_proposal_5_0_6.json.gz)
preserves established generated bytes when canonical and trait identities agree,
while separating override keys when they differ. It is being regenerated and
compiled independently against the installed runtime. It must complete before
producer installation and permanent Java/generated-reproduction acceptance.

### Permanent producer tests and invocation correction

Two permanent Java identity tests are added in the independent cpp_support
workspace. The unchanged installed producer fails exactly the override-key
assertion, while the copied conditional producer passes both. The
[first existing-generation replay](parity/container_identity_generator_java_5_0_6.json.gz)
passes 73/75; two tests fail because the observer launches from the runtime
repository and cannot open a relative generator model fixture. This is an
invocation failure. The [corrected replay](parity/container_identity_generator_java_v2_5_0_6.json.gz)
uses the generator-module working directory with every assertion unchanged;
its terminal status must be checked before claiming the full Java gate.
No permanent producer implementation has been installed at this checkpoint.

## Conditional producer installation

The [conditional full-SDK candidate](parity/container_identity_generator_conditional_candidate_5_0_6.json.gz)
compiles current producer sources with the copied, reviewed correction and uses
the installed runtime headers rather than an overlay. All five original SDK
identity observations and **35/35** execution entries match. Complete default,
table-only and service-only generated file sets remain byte-identical to the
pre-change producer. Only distinct entity overrides require the additional
canonical Q key; the provider/standalone registry keys retain their independent
source responsibility. Typed RuntimeModule registration and new-entity ID scope
continue to use the entity trait identity.

The corrected Java replay passes the **two new identity contracts plus all 75
existing generation tests**. The old producer still passes its 75 existing tests
and fails exactly the new override-key contract, demonstrating the regression
witness. Every source hash remains unchanged during these comparisons. The
initial working-directory failure remains separate evidence.

The [installation receipt](parity/container_identity_generator_installation_5_0_6.json.gz)
records the exact producer before/after hashes and the completed prerequisites.
The [current generation gate](parity/container_identity_installed_generation_5_0_6.json.gz)
is refreshing generated provenance through the normal generation scripts and
then checking every active example/contract fixture strictly. Generated files
are never hand-edited. Its terminal result is required before claiming current
artifact reproduction; no source/capability or complete P0-P7 acceptance follows
from installing the correction.

## Current installed generation closure

The [normal generation receipt](parity/container_identity_installed_generation_5_0_6.json.gz)
passes generation and strict reproduction for25 example destinations and50
contract destinations. Only their75 provenance manifests change; generated SDK
implementations remain identical. Its global producer verifier then correctly
rejects a stale source hash in a separately maintained query-alias SDK. That
attempt remains a failed overall checkpoint, rather than being relabeled.

The [completed closure](parity/container_identity_installed_generation_closure_5_0_6.json.gz)
freshly reproduces all files of five additional active SDKs: query aliases,
relation identities, expression roots, hydration allocation and graph
serialization. Every file matches its retained byte hash before the new producer
hash is recorded. Original Rust and historical C++ fixtures remain unchanged.
This brings the reviewed provenance refresh to80 destinations. No SDK function,
model, Assist text or generated file set changes in this refresh.

The actual Maven-built installed producer passes all77 Java contracts and
freshly emits all five identity SDKs byte-identically to their already compiled
and executed candidate artifacts. The receipt claims source/artifact reuse
explicitly; it does not invent a second C++ execution. Locked generator parity,
upstream and module-path verifiers pass. The closure audit allows exactly the
five reviewed additional provenance records and removes its owned outputs.
The preceding native877 result remains its own checkpoint; current installed
runtime gates remain native1/module1/sanitizer1. A scoped evidence entry is added
to G06/G14 without changing any of the17 capability statuses or the116 accepted /
6 pending /2 external source counts. Complete P0-P7 remains outstanding.

### Final scope and artifact audit

The [final continuation audit](parity/container_identity_final_continuation_5_0_6.json.gz)
compares every current execution input with the earlier successful frozen native
snapshot. Its86 reviewed changes are exactly the two runtime headers, permanent
C++ fixture, conditional Java producer, new Java test, scoped capability-evidence
ledger and80 provenance manifests. All other execution inputs and both references
remain unchanged. No generated implementation or historical fixture is changed.

Thirteen completed probe/gate directories are absent after their owning processes
terminate. Their measured disposable bytes and retained compressed receipt sizes
are recorded in that audit. Reusable native, module and sanitizer preset outputs
remain in their stable directories. No shared cache, other session output,
source, database or deliverable is removed. The full877 checkpoint independently
reclaims1,207,478 owned log/state bytes. These cleanup counts describe final owned
trees at reclamation, not cumulative bytes written or a cache eviction claim.
