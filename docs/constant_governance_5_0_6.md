# Constant bootstrap registration, governance and audit

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared original/native matrix agrees; complete G05/P3 and generator
qualification remain open.

## Original entry point and runtime repair

The retained complete numeric/catalog Rust domains initialize their roots and
constants through `AuditedSaveExt::save`. For models without loaded or pending
dynamic fields, that generic source entry extracts/checks a graph and plans it
before reviewing Mutation Policy. Graph planning initializes the create version
and retains the graph's loaded-field marker in its policy values.

C++ bootstrap previously called its generated-business `AuditedSaveExt<Model>`
facade, which deliberately delegates generated models to the ledger save path.
That selected pending changes and original versions instead of the planned graph.
The first matrix recorded **576/960** differences, including missing version and
`_loaded_fields` in Create policy plans and governance summaries. Physical rows
alone had concealed this difference. The
[initial gaps](parity/constant_governance_5_0_6_initial_gaps.json) remain unchanged.

The shared `GeneratedBootstrapModelOperation` now delegates directly to the
existing generic `AuditedAutomaticSave<Model, false>` operation. This restores
the source bootstrap entry without duplicating save algorithms or changing the
generated-business ledger facade. The generic operation still selects ledger
saving when actual dynamic fields require it. No original source or generated
artifact is edited. The [entry-path review](parity/constant_governance_5_0_6_source_review.json)
records source anchors and hashes.

## Complete observation matrix

**960/960** original/native outcomes agree. Inputs cross two complete generated
domains, six stored states (empty, existing, corrupt, missing, deleted, extra),
one/two initialization rounds, four original public module factories (plain,
Checker, behavior, combined), five governance modes and audit delivery success
or failure. Governance modes are generated default, customer allow without exact
approval, customer allow with exact approval, denial of the root, and denial of
a constant. Actual metadata/schema, provider, ID allocation, executor, Policy,
approval and safe audit sink registrations supply the observations.

The matrix includes **1,440** initialization rounds, **1,380** immutable policy
reviews and **4,200** safe audit events. All database rows and ID floors before
and after each round, active root, exact errors, complete policy Debug values,
event fields/masking/lengths, trace, governance and bootstrap actor/category/
reason/resulting version are compared. Only nondeterministic bootstrap
`occurred_at_millis` is omitted symmetrically; errors and business values are
not normalized.

There are 792 successful rounds, 224 explicit Policy denials, 232 committed
operations with audit delivery failure, and 192 original UNIQUE failures for
soft-deleted fixed-ID constants. An audit-delivery error preserves the committed
change and must not be treated as rollback. Customer denial blocks the affected
write. Repeated initialization observes earlier committed work rather than
manufacturing atomicity across the entire bootstrap sequence.

The old **36-case** variant corpus keeps its output shape for inputs without
module/governance options. Its checks and original bootstrap/automatic-save
regressions remain independent. Raw Policy Debug is a trusted observation,
not a safe log-publication API.

## Ownership and remaining work

Each input actually creates and destroys Global/User Contexts. Policy and audit
callbacks are scoped resources; captured values use the User allocator and are
explicitly copied into the fixture scope for output. This observer does not
claim disabled default PMR allocation or physical-router execution.

This matrix qualifies these registration/policy/audit combinations for the
numeric/catalog domains. It does not close the separate unsigned/temporal,
invalid metadata, dependency topology, dynamic bootstrap, active-save cancellation
or custom checker/resource obligations. The default constant generation guard
remains enabled. See the [qualification matrix](constant_qualification_matrix_5_0_6.md).

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_constant_variants_probe
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_constant_governance_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/teaql_cpp_constant_variants_oracle \
  --cpp build_native_current/teaql_generated_constant_variants_probe --check
ctest --preset native --output-on-failure \
  -R '^(constant_governance_rust_parity|generated_constant_variants_rust_parity|generated_bootstrap_(rust_parity|contracts|model_contracts)|generated_constant_scalar_contracts|audited_auto_save_rust_parity)$'
```

Build the Rust observer with its existing locked test-only manifest
`tests/parity/rust_constant_variants/Cargo.toml` before the live-source command.
The temporary Rust binary path is a local build location, not a required install
layout. Terminal profile results are recorded separately after successful builds.

## Terminal verification

Relevant native checks pass **7/7**, targeted ASan/UBSan passes **3/3** with
leak scanning disabled, and module CTest checks pass **2/2**. The old 36-case
corpus also passes through the updated module consumer. Live Rust/native corpus
reproduction and old original/native comparison finish successfully. Canonical
consumer verification covers 156 bodies; export verification covers 1,479 names
and the seven-module dependency graph is acyclic. Pinned format and check cover
1,017 handwritten C++ inputs. See the
[terminal record](parity/constant_governance_5_0_6.json). No generator body or
generated artifacts changed in this repair; Java/reproduction of generation was
therefore not rerun. No MIPS or physical-device test was run.
