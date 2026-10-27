# Generated creation identity and Context state (Rust 5.0.6)

Status: G06's reopened creation boundary is corrected and requalified. The
installed public API matches 90/90 original observations, current native 10/10,
ASan/UBSan 1/1, modules 1/1 and all 196 compiler contracts. The current capability
status returns to 14 qualified / 3 partial. G13/G14/G17 and P0-P7 remain open;
the frozen 877-test full native result retains its earlier scope.

The references are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`, locked by
`upstream.lock.json` and the independent producer's `cpp_generator.lock.json`.
The [observer](../scripts/verify_generated_creation_identity_5_0_6.py) extracts
complete original SDKs byte for byte from retained source evidence, checks their
hashes and registry identities, and executes their public requests. C++ SDKs
are freshly generated from the same models with current producer sources.

## Source contract

The original generated request creates the source model even after adapting
its result type. It constructs fresh mutable entity state sharing the Context's
immutable graph, calls `next_id` with the model's raw declared name, updates the
ID only for a successful result, and always marks the entity new. A returned
allocation error leaves ID zero and its field unloaded; a successful zero loads
the field. This error handling does not swallow thrown exceptions or authorize
persistence without its normal checks. Later graph freezing remains visible to
previously created entities, while their mutation and load state stay independent.

These are entity internal IDs passed to `InternalIdGenerator`; arena reference
IDs and modeled Business-ID values retain their separate protocols.

Three identities must remain distinct:

| Identity | Purpose |
| --- | --- |
| Raw declared model name, such as `item` | New-entity ID allocation through the replaceable allocator. |
| Canonical generated model name, such as `Item` | Query target and explicit repository lookup. |
| Trait/descriptor name, possibly overridden to `Other` | Runtime metadata and typed module registration. |

The raw name comes directly from `objectDescriptor.name` in the original entity
constant. It is not inferred by case conversion or pluralization. C++ exposes
this responsibility as optional `ID_SPACE_ENTITY_NAME` metadata; older and
handwritten model types retain their `ENTITY_NAME` fallback. The shared runtime
owns creation behavior; generated metadata does not introduce another allocator
or duplicate the entity-state algorithm.

## Initial observations

The [first run](parity/generated_creation_identity_initial_5_0_6.json.gz)
compiled and executed the original control SDK but failed compiling the observer's
C++ JSON adapter. No C++ creation ran. The corrected
[before-repair run](parity/generated_creation_identity_before_5_0_6.json.gz)
compiles and executes both sides successfully for all five configurations:
ordinary, descriptor override, storage override, service override and combined.

Each SDK has 18 cases: successful/nonzero, successful/zero and returned-error ID
allocation; no graph, graph frozen before creation and graph frozen between two
creations; original and adapted result markers. Complete output includes both
entities' values, loaded/dirty/new state, actual allocator arguments, graph reads
and a separately observed Context graph. All 90 original/current pairs differ.
The original calls the allocator with `item` in every configuration. C++ calls
with `Item` or `Other`, throws on returned allocator errors and leaves created
entities detached from the Context graph. Those differences are not approved
compatibility exceptions.

Default PMR allocation is disabled in C++ observations. Managed objects, graph
values, allocator diagnostics and JSON use UserContext memory. Each successful
native run checks actual UserContext destruction and zero live UserContext bytes.
Owned temporary sources, targets and binaries are removed after child processes
exit; compact receipts retain commands, full results, hashes and observer source.

## Runtime-first correction

The installed runtime constructs the entity in the bound UserContext, attaches
its existing recursive runtime-state protocol to a fresh Context state, and
updates ID only when allocation succeeds. Query authorization, result-marker
Entity guards and the callback quiescence guard remain in their existing layers.
The optional ID metadata has an explicit legacy/handwritten fallback.

The [initial focused gates](parity/generated_creation_runtime_initial_gates_5_0_6.json.gz)
pass native **1/1**, modules **1/1** and ASan/UBSan **1/1**, with leak scanning
disabled. The permanent test crosses three ID outcomes and three graph timing
states, checks actual raw allocator arguments, independent loaded/dirty/new
state, foreign Context refusal, disabled default PMR and actual Context deletion
with zero retained UserContext bytes. Unused fixture decoders throw if called;
these tests do not replace provider or graph decoding algorithms.

Only `EntityQuery::newEntity` changes in the production runtime header. Every
other header body remains identical. The former complete native 877 result is
historical after this header change; it is not current-code validation.

The [runtime-only dual run](parity/generated_creation_identity_runtime_5_0_6.json.gz)
rebuilds and executes both original and native callers for all 90 cases, before
producer changes. Every entity state, error outcome and graph value agrees.
All 90 pairs still differ in allocator arguments, as expected for old generated
models without the raw-name constant. The
[explicit residual classification](parity/generated_creation_runtime_residual_5_0_6.json.gz)
records this bounded state agreement separately; full unmodified results remain
in the receipt and no full-parity claim is made for that intermediate run.

## Thin producer metadata

After those runtime gates, the producer emits `ID_SPACE_ENTITY_NAME` directly
from `od.getName()`. Query identity, overridden descriptor identity, generated
newEntity delegates and all provider registration responsibilities retain their
existing implementation. Model-specific Assist explains graph binding and the
difference between a returned allocation error and a successful zero.

A permanent Java test covers raw regular, multiword, human, uncountable and
inherited model names, a distinct descriptor override and canonical query name,
and the unchanged thin creation delegate. Whole-SDK regeneration permits only
one ID constant per model and the exact new Assist paragraph. Removing those
additions and running the pinned formatter must reproduce every previous byte;
all other files and historical/original fixtures are protected.

The [installed comparison](parity/generated_creation_identity_installed_5_0_6.json.gz)
now matches **90/90** complete observations. It explicitly reuses the terminal
runtime-only run's immutable original results after checking identical observer,
inputs, complete SDK/model hashes, locked archive and registry verification.
All five current C++ SDKs are generated, compiled and executed again. Full JSON
values, allocator arguments and before/after producer/runtime input hashes are
retained; no ID-name or state normalization is applied.

The [current Java gate](parity/generated_creation_java_gate_5_0_6.json.gz)
passes **86/86** tests across the five selected suites, including the new
raw-name/descriptor/guide regression and explicit invalid-wire-profile refusal.
It recompiles current producer and test sources into an owned directory. The
previous 115-test result remains its original broader checkpoint.

The [whole-SDK update](parity/generated_creation_id_sdk_update_5_0_6.json.gz)
reproduces **80/80** active destinations. Exactly **132** implementation/guidance
files change: **188** raw-name constants and **188** exact Assist paragraphs.
All previous generated bytes are recovered after removing only those additions
and pinned formatting. Every file set, model hash and previous manifest/file hash
is checked before staged installation; **2,467** original/historical protected
artifacts remain unchanged. Generator fingerprints are refreshed in all active
manifests, without changing the locked reference commits.

The [post-installation hash check](parity/generated_creation_id_sdk_installed_5_0_6.json.gz)
confirms all **3,560** generated files agree with the 80 installed manifest maps.
The [owned-artifact cleanup](parity/generated_creation_cleanup_5_0_6.json.gz)
verifies seven one-off directories have been removed after their children exited.
Measured removed files total **641,053,991 bytes**; the initial failed observer
SDK staging and native compiler-contract trees were cleaned without separate
size measurements. Stable
preset outputs and shared caches remain available for compatible rebuilds.

## Remaining integration scope

The [embedded-observer formatting proof](parity/generated_creation_observer_format_5_0_6.json.gz)
keeps the script's C++ readable with the pinned formatter and proves every
compiled C++ observer remains byte-identical to the installed five-SDK run.
The source observer and all comparison behavior remain unchanged.

This correction addresses the reproduced public creation boundary. Complete
protected Save/Policy/audit caller reconciliation across metadata overrides is
still an AP04/G14/G17 obligation. The exact source/runtime responsibilities and
independent provider layers remain in scope. Neither the 90-case matrix nor
whole-SDK body preservation accepts those broader capabilities or P0-P7.

## Current integration and acceptance

The [post-regeneration integration](parity/generated_creation_identity_integration_5_0_6.json.gz)
builds every selected native executable before running **10/10** tests. It covers
creation and actual SQLite Save, generated APIs, entity helper lifetimes,
source/result markers, inherited return mapping and Save, asynchronous sample
Save and the complete generated-API compiler gate. Its
[55 positive / 141 negative contracts](parity/generated_creation_compiler_contracts_5_0_6.json.gz)
all pass; the latter include the expected diagnostic checks. The compiler test
took 2,029.98 seconds and completed successfully; no shortened timeout or skipped
contract is counted as a pass.

The same controller rebuilds and passes targeted **ASan/UBSan 1/1**, with leak
scanning disabled, and imported **modules 1/1** after SDK installation. All
recorded runtime/test inputs and all generated file hashes stay identical through
these current gates. Final pinned formatting and source/path verification are
recorded separately at the milestone.

GQ02's public creation responsibility and GQ12's metadata/guidance/compiler/
Context requirements are requalified by the actual source comparison, runtime
first gates, thin producer delta, complete 80-SDK byte proof and current
integration above. Every other production header body and every prior generated
function body remains unchanged. Their earlier accepted responsibilities retain
their exact scope. This restores G06 without promoting G13/G14/G17 or SQ05/P7's
full current native integration requirement.

Final verification formats and checks all **1,805** handwritten C++ files with
pinned clang-format 18.1.8. Runtime/generator locks, all17 capability records,
active module paths and both workspace diffs pass their respective checks. The
[milestone receipt](parity/generated_creation_identity_milestone_5_0_6.json.gz)
retains current hashes, bounded evidence,14 qualified/3 partial status and the
explicitly incomplete whole goal. All seven one-off artifact trees are removed;
compact creation evidence occupies about514 KiB. Compatible preset outputs remain
available, and the seven paths are verified absent after the final checks.
