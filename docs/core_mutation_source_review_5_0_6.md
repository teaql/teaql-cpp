# Complete core mutation model source review

Status: MU01-MU14 are accepted for the complete named source with explicit
C++ Context/interface adaptations. Whole capabilities and P0-P7 remain open.
Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The finite source scope is the complete 291-line `teaql-core/src/mutation.rs`:
three distinct map roles, six command models, MutationKind, fourteen methods and
all explicit/derived map/model protocols. This file has no local unit tests;
actual original-call observers and the unchanged core crate tests provide
executable evidence.

The [historical model review](mutation_model_api_acceptance.md) and
[raw Debug review](mutation_debug_acceptance.md) retain their 5.0.5 checkpoints.
Current trace source qualification and the
[data-service request review](data_service_source_review_5_0_6.md) are separate
dependencies. Model construction is not mutation execution, graph Save,
authorization or safe log publication.

## Whole-source responsibilities

| ID | Original responsibility | Native mapping and evidence |
| --- | --- | --- |
| MU01 | Three independent map roles | MutationValues contains intentional mutation input, GeneratedValues contains provider output and EntitySnapshot contains previously persisted values. All six direct cross-role assignments are rejected by the original compiler. Native compilation checks implicit conversion and assignment in every direction. Explicit Record conversion remains possible and deliberately visible. |
| MU02 | Empty constructors and map defaults | Each wrapper has empty new/Default, ordered map storage, mutable/shared delegated map access and separate Debug/Clone/PartialEq. Native construction names its Context allocator; no hidden default resource owns the fields. |
| MU03 | Record conversion and iteration | All three wrappers support owned Record conversion in both directions. Only MutationValues explicitly implements owned/shared IntoIterator. Native fields/copy_map/rvalue-only into_map preserve the map surface and scoped ownership; mutable native ranges are an extension. GeneratedValues/EntitySnapshot do not acquire the source's absent IntoIterator implementations. |
| MU04 | MutationKind | Insert, Update, Delete and Recover remain four distinct values with Debug/Clone/Copy/PartialEq/Eq. Batch command models do not add a fictitious Batch variant. No source Default/Hash/serde is invented. |
| MU05 | InsertCommand | Entity, values and trace_chain retain their separate fields. New preserves the complete entity string and starts empty. value inserts/replaces the supplied key without creating intent, metadata, IDs or traces. |
| MU06 | UpdateCommand | Entity, arbitrary typed id, optional expected version, values, trace_chain and optional old snapshot remain distinct. Constructor starts with absent version/snapshot; version and value builders replace only their named fields. None, Some(empty) and Some(populated) snapshots remain different. |
| MU07 | BatchInsertCommand | Entity, batch_values and independent trace_chains are preserved; both vectors start empty. The model does not align lengths or invent a trace for each row. |
| MU08 | BatchUpdateCommand | Preserve entity, IDs, optional versions, values, ordered update_fields, trace_chains and optional old snapshots. Constructor keeps duplicate/empty update fields and initializes every other vector empty. Irregular lengths are valid model state; executor validation is independent. |
| MU09 | DeleteCommand | Preserve entity, typed id, absent expected version, initial soft_delete=true and empty trace. Version replacement accepts every i64; repeated hard_delete remains false without modifying other fields. |
| MU10 | RecoverCommand | Preserve entity, typed id, mandatory i64 expected_version and empty trace. This version is not optional and receives no inferred default or validation. |
| MU11 | Structural Clone and PartialEq | Every source field participates. Value variants remain distinct; NaN-containing values/commands stay non-reflexive. Allocator identity is outside value equality. C++ syntactic equality availability is not a promise of Rust Eq or total reflexive equality. Explicit copies recursively allocate keys, payloads, versions, snapshots, batch fields and traces in the destination. |
| MU12 | Complete raw Debug | Preserve role names, declared field order, all option/container shapes and nine formatter modes. Formatting does not normalize irregular batches, replace mutation input with snapshots or perform privacy projection. Invalid native enum casts remain an explicit C++ extension outside valid Rust states. |
| MU13 | Generic capability and interface boundaries | Original wrappers have Default; all six commands do not. One positive compiler example exercises wrapper derives, sorted shared/owned iteration and explicit Record roundtrips; fourteen negative examples cover six wrong-role assignments, two unsupported owned iterations and six missing command Defaults. Native scoped construction/copy/consumption declarations retain their stronger explicit allocator contract. |
| MU14 | Ownership, failures and current integration | Complete existing default-PMR-disabled bodies cover same-owner node transfer, foreign deep copies, copy-failure recovery, all command kinds, nested allocators, actual User deletion and Global rebinding. Existing function bodies and production algorithms remain unchanged; model compile assertions now cover all six role directions. Full canonical imported consumers must execute these bodies rather than reduced examples. |

Rust's consuming builders map to in-place native builders returning the current
command. Scoped move/copy and the `expected_version_value` field avoid implicit
ownership extension and C++ field/method collisions. The explicit destination
form permits cross-domain retention without retaining the source Context.
Failed foreign map consumption copies before clearing its source; a failed
attempt cannot partially consume the original map. Same-domain moves preserve
native nodes and leave a valid empty source wrapper until Context destruction.

The native command audit_as convenience is an extension that adds a diagnostic
frame. It does not validate an audit reason, supply request-owned mutation
intent or establish governance. Ordinary pointers, map iterators and views are
borrows with normal invalidation rules; Context close invalidates their storage.
The source has no wire serialization protocol for these models.

## Actual original observations and compiler controls

The isolated `rust_core_mutation_source_5_0_6` observer uses unchanged original
API/Debug drivers and exact reference registry versions/checksums. The retained
1,688 model and 120 Debug observations remain immutable. These cover all model
fields independently, constructors/builders, source map methods, ordering,
conversion, borrowed/owned iteration, derived equality/Clone and raw diagnostics.
Trace observations inside that retained corpus remain trace dependency evidence;
this review does not promote the trace source again.

Additional 240 cases contain 72 typed-wrapper comparisons, 24 complete builder
chains and 144 command comparisons. They use actual NUL bytes rather than the
literal four characters `\x00`, empty/Unicode/control/long names, nested NaN,
version extremes and preserved batch lengths. Finite-valued command witnesses
change one active field at a time so existing NaN inequality cannot hide an
omitted equality field. Builder states show every intermediate result.

The six role rejection examples use actual assignments. The two iteration
rejections use actual for loops, and the six Default examples request the
original missing trait. Required error codes/type names are checked; an unrelated
compiler failure is not acceptance. Native all-direction conversion/assignment
assertions complement the unchanged ownership and default-construction controls.

## Acceptance boundary

The [terminal acceptance](parity/core_mutation_source_acceptance_5_0_6.json.gz)
joins the [actual original/native/imported source replay](parity/core_mutation_source_final_5_0_6.json.gz)
and [installed selections](parity/core_mutation_source_installed_5_0_6.json.gz).
Required positive compiler/linker/test processes are terminal and successful;
expected source compiler rejections are explicitly recorded with their exact
error codes and type/trait names.

| Gate | Completed evidence |
| --- | --- |
| Actual original tests | All 69 unchanged core crate unit tests pass. mutation.rs has no local tests; the crate total is not presented as 69 mutation-specific tests. |
| Original API and Debug | All 1,688 API and 120 complete Debug outcomes reproduce on both C++ frontends against immutable corpora. |
| New boundaries | All 240 authored wrapper/builder/command cases agree; actual NUL bytes, preserved batch shapes, NaN and finite one-field differences remain explicit. |
| Exact corpus replay | All 2,048 outcomes per original/native/imported frontend pass again, reproducing every retained/new corpus byte without normalization. |
| Source compiler controls | One positive example compiles and executes; fourteen negatives reject six wrong-role assignments, two unsupported owned iterations and six missing command Defaults. |
| Native type capabilities | Both frontends compile conversion and assignment assertions for all six distinct-role directions. Existing scoped Clone/default/iteration checks remain. |
| Installed contracts | Native 5/5 and imported 5/5, exact case sets, no skips and no-work builds before and after execution. |
| Canonical modules | 942 complete consumer/fixture bodies verify; four full existing contracts/probes are newly registered. |
| Formatting | Apply/check of 1,683 handwritten C++ files passes with the pinned formatter. |
| Preservation | All production bodies and existing test/observer function bodies are unchanged. The model test changes only its all-direction static assertions. Six native archives, 78 SDKs/3,457 files and 2,201 protected artifacts retain their hashes. |

Exactly `teaql-core/src/mutation.rs` advances to accepted. Source status is
**46 accepted / 76 pending / 2 excluded_external**. All seventeen capability
statuses remain unchanged: eleven qualified and six partial. One native
comparison brings discovery to 748; this is not a whole-suite pass. Historical
735/736 and 718/721 failed full epochs retain their original outcomes.

The [disk recovery record](parity/core_mutation_source_disk_recovery_5_0_6.json.gz)
retains a post-test ENOSPC while creating the temporary acceptance controller.
Only three task-owned Cargo incremental caches were removed. Source, models,
corpora, receipts, logs, qualified executables and archives were preserved;
source/corpus/binary and SDK identity checks completed before promotion. The
empty temporary controller was rewritten after space was restored.

No production algorithm, generator or generated SDK body changed. Whole
mutation execution, graph Save, policy, audit/privacy publication and
G13/G14/G17/P0-P7 retain their independent obligations and unanswered behavior
decisions. No new production dependency, Java/generation, sanitizer, MIPS,
external-provider, GPU or physical-device result is claimed.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_mutation_model_api_tests teaql_mutation_model_api_probe \
    teaql_mutation_debug_tests teaql_mutation_debug_probe
cmake --preset modules_native
cmake --build --preset modules_native --parallel 2 --target \
    teaql_module_mutation_model_api_tests teaql_module_mutation_model_api_probe \
    teaql_module_mutation_debug_tests teaql_module_mutation_debug_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_core_mutation_source_5_0_6.py \
    --module-build-dir build_modules_native --record "$replay_directory/source.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(mutation_model_api_contracts|mutation_debug_contracts|mutation_model_api_rust_parity|mutation_debug_rust_parity|core_mutation_boundaries_rust_parity)$'
ctest --preset modules_native --no-tests=error --output-on-failure \
    -R '^module_(mutation_model_api_contracts|mutation_debug_contracts|mutation_model_api_rust_parity|mutation_debug_rust_parity|core_mutation_boundaries_rust_parity)$'
```

The reproducer verifies the original checkout and exact registry identities,
waits for successful builds before executing their products, checks each
expected compiler rejection and compares every observation/corpus byte. The
installed receipt retains exact commands, complete output, JUnit results and
stable builds. Generated compiler/Java evidence is unchanged and is not
presented as newly executed work.
