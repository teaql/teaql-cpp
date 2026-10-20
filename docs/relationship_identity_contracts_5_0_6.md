# Canonical relationship identities and C++ keyword adaptation

Runtime baseline: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator baseline:
`45e1572c4956ca963410d8ea436ead3c8857087a`.
This is a focused G14 identity checkpoint; complete G14/P0–P7 acceptance remains
open. Original producer sources and generated Rust artifacts are unchanged.

## Source contract and failure

A model relationship name is data. C++ identifier escaping must not change its
metadata, query selection, load-state key, JSON key, graph mutation key or E path.
The original generator's `RustFieldDescriptorNaming.rustMemberName` and
`RustReverseRelationNaming.rustMemberName` determine those identities. In
particular, the C++ keywords `namespace`, `class`, `operator` and `template`
remain their original relationship names. Source Rust identifier adaptation is
retained where that source helper requires it.

The frozen `entity/struct.rs.stg` emits reverse descriptor attributes from
`rustOneToOneRelations` followed by `rustOneToManyRelations`. Descriptor order
is observable and must retain this order without sorting comparator output.
It is separate from fixed-field indexes and the native storage/member layout.

The previous C++ generator reused its escaped `relationName` for identifiers
and literals. The complete original/native 168-input comparison initially
matched only 48 cases. After canonical literal repair, 96 matched; the remaining
comparison exposed reverse descriptor ordering. Both full failure records and
complete C++ SDK/source snapshots are retained under `docs/parity` and the fixture.

## Runtime first, then generated adapters

`relation_identity_kernel_tests.cpp` first exercises the existing real graph
builder and runtime relation lookup with keyword strings. Canonical names work;
escaped-name lookup remains absent, loaded NULL remains distinct, explicit Global
copies own nested text and checked handles expire after actual User deletion.
The default PMR resource is disabled. No runtime memory or graph algorithm change
is needed for this repair.

The producer separates `relationIdentity` from `relationName`. Native fields and
existing model/Q/E methods retain their C++ escaping. Metadata, indexed relation
availability, decoding, JSON projection, mutation extraction, selections/removal
and E traversal use the canonical identity. Reverse descriptors follow the
original one-to-one/one-to-many sequence.

E diagnostics derive repair method names from canonical paths. Prefixed methods
are legal C++ even when the unprefixed model name is a keyword. The producer
therefore emits thin canonical Q/E delegates, preserving its existing methods:

| Model identity | Existing C++ method | Canonical diagnostic method |
| --- | --- | --- |
| `namespace` | `selectModelNamespace()` | `selectNamespace()` |
| `namespace` | `selectModelNamespaceWith(child)` | `selectNamespaceWith(child)` |
| `namespace` | `unselectModelNamespace()` | `unselectNamespace()` |
| `namespace` | `getModelNamespace()` | `getNamespace()` |
| `class` | `selectModelClassWith(child)` | `selectClassWith(child)` |
| `class` | `getModelClass()` | `getClass()` |

These adapters retain ref-qualified request chaining, existing child-input
constraints and Context binding. They do not load data, allocate a private scope
or extend Context lifetime. Reverse relation removal is not introduced. Scalars
with escaped C++ identifiers retain their separate existing contract; this
checkpoint does not establish new scalar diagnostic-alias coverage.

## Observation scope

The fixture uses real forward optional/required, reverse one-to-one and reverse
one-to-many relations, with live/deleted rows and an optional NULL. It executes
complete generated APIs over an existing SQLite schema. It does not qualify
schema/bootstrap generation for this keyword model.

The 168 original/native inputs cover three entities; normal/minimal projection;
self, all and child selection; named and explicitly queried relationships;
replacement and forward removal; deleted-row inclusion; canonical dynamic JSON
filters; typed JSON/load-state and compact-row execution. Full metadata order,
SelectQuery Debug, rows/errors and ordered SQL/parameters/intent/trace are compared
without normalization. The Owner `remove_first` stimulus is a source `select_self`
control, not reverse removal acceptance. E execution and actual lifetime controls
are separate C++ tests; they are not added to or substituted for the immutable
original corpus. Audited graph-save execution for these keyword relationships
now has a separate [608-case source comparison](generated_ledger_composition_5_0_6.md),
with its own observer, immutable corpus and scoped acceptance gates.

## Generation and review gates

The two complete Rust SDKs reproduce unchanged. Before replacing C++ output,
`relation_identity_qualification.py` verifies a restricted repair against retained
SDK bytes: canonical relationship literals, source-ordered reverse descriptor
blocks, exact canonical Q/E forwarding methods and the corresponding Assist
paragraphs. Existing application method bodies are otherwise preserved. Pinned
formatting checks the output; empty declaration gaps left by removing an exact
adapter are ignored by the change-scope proof. Ambiguous escaped field/relation
literals are rejected for separate review.

The same proof covers the 24 normal generated workspaces and the contract SDK
families. Source/body hashes and historical G07 evidence remain intact; the new
predicate/projection review records the relation-only changes to broad generator
helpers in a separate artifact.

## Reproduce

Use the locked tools and successfully compiled producer classes. Reproduction
uses the existing classpath files prepared by the generator qualification tools.

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/generate_relation_identity_contracts_5_0_6.py --check
python3 scripts/verify_relation_identity_sdk_update_5_0_6.py
python3 scripts/verify_generator_parity.py
cmake --preset native
cmake --build --preset native --target teaql_relation_identity_probe \
    teaql_relation_identity_tests teaql_relation_identity_kernel_tests
ctest --preset native --output-on-failure -R '^relation_identity_'
```

Original replay adds `--rust` to `compare_relation_identity_5_0_6.py`, pointing
to the successfully built unchanged `rust_relation_identity_5_0_6` executable.
Module and selected sanitizer controls have their own targets and terminal logs.
Physical-router execution and full-suite acceptance are separate gates.

## Terminal verification

All **168/168** inputs match actual unchanged original Rust, native C++ and the
imported-module observer. All execute successfully: 84 typed-list inputs and
84 compact-row inputs produce 424 retained rows/snapshots and 286 ordered safe
SQL log records. The original corpus, both failure checkpoints and complete
repaired outcomes remain separate immutable artifacts.

Relevant native regression passes **9/9**, imported modules **3/3**, and selected
ASan/UBSan lifetime tests **2/2**, with leak scanning disabled. The compiler gate
passes **2 positive / 5 negative** controls, including canonical/escaped API use,
extra Context rejection, invalid/purposed child input and extra execution argument
rejection. Java regression passes **66/66** across generation, field-layout and
JSON-input metadata tests.

All 24 normal workspaces/288 files reproduce strictly. The restricted update
proof identifies one changed normal code file: the relation fixture's model
header, with source reverse-descriptor ordering. All 50 existing contract SDKs
reproduce with zero code or Assist changes. Both paired keyword SDKs reproduce,
including unchanged complete original artifacts and preserved historical C++
versions. The existing alias fixture's separate reproduction entry also passes.
The source/ledger lock, G07 follow-up body review and module paths verify;
canonical-module verification covers 765 consumer/fixture bodies. Pinned
format/check passes 1,606 handwritten C++ inputs.

The [terminal archive](parity/relation_identity_terminal_5_0_6.json.gz) retains
full logs, Java reports, source/artifact hashes, preparation failures and exact
gate scope. The [producer patch](parity/relation_identity_producer_change_5_0_6.patch)
and [restricted normal-workspace proof](parity/relation_identity_workspace_update_5_0_6.json)
make the change reviewable. This historical checkpoint keeps its scope; the
subsequent generated ledger-composition checkpoint adds keyword-relation audited
saves. G14 remains partial for broader model/privacy combinations and final
integration.
