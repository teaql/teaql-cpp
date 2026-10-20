# Constant dependencies, foreign keys and relation reads

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: ten complete generation/compiler configurations reproduce; the five
source-compilable domains' declared execution matrix passes. G05/P3 and
complete generator acceptance remain partial.

## Model and parser contracts

The fixtures use an ordinary Platform root, Category constants and Status
constants referencing Category. They exercise ordered/reversed declarations,
reference targets without ID 1, explicit foreign seed values, optional/required
self-reference, optional forward reference, two/three-entity cycles and excessive
dependency depth. Complete original and native SDKs are retained with provenance;
no generated source is edited to make an original failure compile.

Five complete original crates compile: ordered, reversed, missing-target-one,
foreign-value-one and required self-reference. All seven emitted C++ umbrellas
compile independently. Optional self-reference and optional forward reference
each retain **two E0308** errors in original entity helpers that compare
`Option<u64>` with `u64`. Their C++ compile successes are not paired runtime
oracle evidence.

Both multi-entity cycles and depth beyond **20** fail identically in the shared
authentic parser, before SDK emission. Full exceptions include the actual element
chain and XML reference attributes. Immediate self-reference is skipped by the
parser's depth traversal; it is not equivalent to the rejected multi-entity cycle.
The parser failure outputs contain only observations, not a partial SDK.

## Bootstrap preserves order and fixed reference values

Original bootstrap writes the ordinary root first, then constant descriptors in
their actual domain order. It does not topologically sort those writes. With
SQLite foreign-key enforcement, Status declared before its Category dependency
can fail initialization, leaving only earlier committed work. Native must retain
that result rather than silently reorder the model.

Generated object-reference seed assignments use ID **1**. Explicit XML seed data
such as `category="22"` does not replace that fixed reference. A Category catalog
containing only IDs 22/33 therefore leaves Status initialization subject to a real
foreign-key failure. Source compile success does not prove successful startup.
The ordered and foreign-value-one fixtures verify explicit input variations
through actual physical rows and generated relation reads.

## Typed graphs and distinct E outcomes

After schema/bootstrap, generated Q loads Status scalar fields and its selected
Category. Required self-reference adds Category's selected parent. E then reads
the scalar code, related code and self-parent code without issuing SQL.

A non-null foreign identity whose target is absent from the live selected graph
can produce **NotLoaded**, including when its target row is soft-deleted. It must
not become a legitimate NULL or an empty business conclusion. The native observer
catches only `ExpressionNotLoadedError`. The original observer catches only the
generated missing-preload panic signature and resumes unrelated panics.

The comparison records Value/Null/NotLoaded, exact scalar values, full database
rows, versions, ID floors, active root, errors and SQL/binding/intent records.
Original panic diagnostics and native structured diagnostics are retained
separately. Their process framing and generated preload syntax differ by language;
this checkpoint compares E outcomes, not raw diagnostic-text identity. Existing
diagnostic text/framing qualification remains its separate contract.

Every round uses a fresh UserContext. The connection and explicit observation
copies belong to GlobalContext; the observer actually deletes the UserContext
and rechecks database state. Default PMR allocation is disabled. Bootstrap and
relation execution remain in the existing runtime/service/executor/provider
layers. The observers contain only setup, fixture data and observation logic.

All **480** scenarios agree over **720** Context rounds, **864** returned typed
rows and **3,444** SQL diagnostic entries. All **720** typed queries succeed.
Bootstrap succeeds in **360** rounds; **288** preserve real foreign-key failures
and **72** preserve soft-deleted Category fixed-ID UNIQUE failures. Every database
snapshot agrees before and after actual User deletion.

Related Category reads return Value **792** times and NotLoaded **72** times.
Required-self-parent reads return Value **264** times and NotLoaded **24** times.
No legitimate Null result occurs in this dataset; that branch is not qualified
by these counts. The remaining parent reads are explicitly NotApplicable for
models that have no parent field.

Native and module CTest selections each pass **5/5**. All ten complete artifact
and compiler configurations reproduce, as do the five live original execution
corpora. **442** canonical consumer/fixture bodies and pinned format/check of
**1,349** C++ files pass. See the
[terminal record](parity/constant_topology_contracts_5_0_6_terminal.json) and
[separate raw runtime diagnostics](parity/constant_topology_contracts_5_0_6_runtime_diagnostics.json.gz).

## Reproduce

```sh
source scripts/toolchains/activate.sh
python3 scripts/audit_constant_topology_contracts.py --check \
  --report docs/parity/constant_topology_contracts_5_0_6_initial_audit.json
python3 scripts/verify_constant_topology_execution.py --check
ctest --preset native --output-on-failure -R '^constant_topology_.*_rust_parity$'
ctest --preset modules_native --output-on-failure -R '^module_constant_topology_.*_rust_parity$'
```

Build each `teaql_constant_topology_<case>_probe` or
`teaql_module_constant_topology_<case>_probe` first. The original/native complete
file sets, model/observer/producer hashes, compiler outcomes and source corpus
remain checked. The [source review](parity/constant_topology_contracts_5_0_6_source_review.json)
records the actual parser, ordering and fixed-reference boundaries.

The missing-row fixture deletes an unreferenced Category after its chosen Status
row; it does not disable foreign keys to force impossible physical state. Deleted
targets are represented by version changes, preserving their referenced rows.
Default generator guards remain. Broader configurations, optional-reference
source failure dispositions, extensions and full P0-P7 acceptance remain separate.
No sanitizer, MIPS or physical-device result is inferred.
