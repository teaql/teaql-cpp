# G16 source and model acceptance review

Status: G16 qualified for all sixteen requirements, with the documented C++
Context ownership and source-supported-shape boundaries. The
[final acceptance record](parity/dynamic_storage_acceptance_5_0_6.json.gz)
preserves the previous ledger, complete requirement audit and terminal gates.
The finite scope is all sixteen [G16 requirements](dynamic_storage_requirements_5_0_6.md),
not just an ordinary SQLite text-field roundtrip.

References are Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. Preserve the original source,
historical observers, corpora and initial failures.

## Definitions, row state and generated carriers

DS01-DS02 retain the entire original definition, selection, typed validation,
batch construction and mutable carrier responsibilities. Full definition
equality includes the optional storage binding; `same_schema` deliberately
does not. Selection validation, unknown-code failure, typed NULL, incoming-value
precedence and absent-versus-NULL are independent branches. Selected geometry
can share while values remain private. The original hash buckets compare full
selected sets, and merge unions retain source identity. C++ uses checked scoped
identities rather than retaining Rust `Arc` owners past Context close.

The complete definition corpus has 296 observations; the carrier corpus has 176.
The imported complete contracts additionally check private payloads, shared
geometry, explicit destination copies and invalidation. G15's accepted indexed
state remains a dependency, including its initialized-cache and immutable-view
responsibilities. A schema describes availability geometry, never the values of
another entity.

DS03 preserves every readonly validator branch separately from persistent
validation. Unknown readonly names are accepted; declared readonly JSON-shaped
presentation values follow the original type rules. Persistent unknown codes
fail, and a persistent Json declaration requires the Json variant. Neither
validator silently coerces Object/List into durable Json. The original cached
u64 readonly schema hash, conflicts, subset reuse and presence distinctions are
covered by the accepted G15 schema and row contracts.

DS04/DS13 retain the original core Entity defaults and the macro's indexed versus
manual branches. Generated installation checks owner and canonical layout before
changing the carrier. Value-only edits retain availability; selected-name changes
publish a new snapshot. Manual entities do not gain indexed loading by accepting
JSON. Generated hooks delegate to runtime and do not become an independent
storage implementation. The accepted G01/G04/G10/G15 contracts supply ordinary,
optional and inherited model evidence; the combined gate retains generated
durable and readonly contracts and native model-specific observer checks.

## Provider, executor and physical storage

DS05-DS08 preserve distinct store, provider, Context, executor and cursor-plan
boundaries. The 49-observation pure SPI corpus checks plan and batch failures
without I/O. The 72-observation provider corpus separately checks actual provider
resolution and failure precedence. The default custom provider does not support
graph writes or stream enhancement; opting in requires the corresponding methods.
Optional schema initialization without a provider remains successful.

Provider metadata and stream preparation require identity before native cursor
execution. Ordinary provider loading obtains store values before checking the
returned store identity, matching the source's failure order. Graph preparation
checks the configured definition schema, actual transaction binding, ambient
binding key and expected view provenance. Sharing a raw connection is insufficient.
Replacement of a Context registry entry does not destroy an older arena object;
managed resources remain alive until Context shutdown.

DS09 follows the complete SQLite `validate`, `apply`, `decode` and `load` bodies.
All ten types, exact finite f64 bits, Decimal strings, widths, explicit NULL,
Delete and 400-ID chunks retain their separate behavior. The original 123-case
store corpus and 34 additional corruption/selection cases run against original
SQLite. Empty IDs avoid querying a missing table; empty selection alone does
not. Driver column extraction precedes selection filtering, so a malformed BLOB
can fail even when a malformed TEXT payload would be skipped. Namespace and
selection validation precede table access. Stable error codes omit stored data.

DS10 preserves ambient read/schema access and exclusive transaction writes.
Whole-batch validation precedes the first statement. Active transaction handles,
the connection lease, statement reset/cache lifetime and commit/rollback remain
provider responsibilities. Existing complete SQLite, transaction and graph Save
contracts exercise actual database contents after success and rejection; they
do not replace storage with a simulated map.

## Loading, cursor execution and governed changes

DS11 follows `resolved.rs` typed loading and `relation.rs` child and batched
embedded loading. Empty batches skip providers; a nonempty batch validates owner
IDs and complete returned cardinality, selections and payload types. A failed
private result must not be published, including when a later row merge fails.
The retained 250-case root/decode/stream and 138-case relation corpora preserve
the original sidecar behavior. Flat/compact graph loading retains supported
sidecars; ordinary embedded maps need not retain a dynamic-only sidecar. Compact
generated facets do not invent persistent loading. The accepted G10 simultaneous
readonly/persistent/facet matrix retains actual restricted-subquery SQLite errors.

DS12 also includes the SQL executor's owner/selection checks and the SQLite
transport's store and chunk-size checks. The enhancer loads on the cursor's
own connection and merges a complete private chunk before publication. The
existing stream contracts check early close, item failures, subsequent reuse
and native-worker cleanup. Relation/aggregate stream enhancement retains its
unsupported contract. C++ pending operations forward wakes and cancel/join before
their arena is reclaimed; source future ownership is not translated into shared
Context ownership.

DS14 follows the runtime graph save orchestration as well as the storage SPI.
Generated IDs and foreign-key rebinding precede dynamic preparation. An audit
scope, graph-capable provider, matching definitions and actual transaction store
are required before native DML. Every pending key participates in governed
preflight, including keys outside the represented root graph. Native writes
precede extension writes; the same transaction then reads authoritative values.
Readback checks owner cardinality, selection, Set/Delete presence and types before
enriching queued audit facts with actual values and first originals.

The 24-case governance corpus and complete preflight/graph Save contracts retain
non-root Checker/Fix, immutable Policy plans, generated IDs, rollback and retryable
pending intent. The 1,152-case typed durable Save matrix covers all ten dynamic
types, three mutations, customer approval/denial, conflicts, extension failures
and committed audit failure. A post-commit sink failure does not undo the database.
The 147 ledger and 14 graph diagnostic observations cover their separate
public-state responsibilities rather than being presented as persistence tests.

## Authority, ownership and acceptance boundary

DS15 keeps request data and presentation JSON separate from trusted definitions,
namespace, registered executor, provider identity and application authorization.
G10's provider-present/missing, denied-I/O and presentation composition matrices
are relevant accepted dependencies. A decoded object does not authorize a Save.

DS16 preserves opaque binding diagnostics, stable storage errors, destination
allocator propagation and actual Context deletion. Explicit Global copies retain
their own payloads. Raw references are documented borrows; registered objects
must not be manually destroyed while operations still borrow them. Provider and
store futures must use the calling Context, and shutdown completes native memory
access before cleanup. Thread ownership and checked liveness adapt Rust's owned
futures and immutable shared metadata without extending UserContext lifetime.

The combined gate is implemented by
`scripts/verify_dynamic_storage_integration_5_0_6.py`: 46 native and 45 imported
checks, including the isolated module export contract. Two model-specific
observers are native-only; their complete imported generated contracts and shared
kernel tests remain in the module selection. Full generator compiler validation
and SDK preservation were already completed for G15; no generator or generated
artifact changes are part of this integration.

Complete G14/G17 protected-I/O, authorization/provenance and final P0-P7 acceptance
retain their own requirements. G16 does not claim arbitrary provider support,
additional database backends, cross-thread Context access or physical-router
execution. Broader phase acceptance cannot be inferred from this finite gate.

## Reproduction

Activate the locked tools and use fresh record paths; failed or historical
records must not be overwritten. Build each profile to successful completion
before running its tests. The driver resolves missing pre-build direct commands
from canonical contracts and confirms them against CTest after linking.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_dynamic_storage_integration_5_0_6.py --profile native --stage build --record /tmp/g16_native.json.gz
python3 scripts/verify_dynamic_storage_integration_5_0_6.py --profile native --stage test --record /tmp/g16_native.json.gz
python3 scripts/verify_dynamic_storage_integration_5_0_6.py --profile modules_native --stage build --record /tmp/g16_module.json.gz
python3 scripts/verify_dynamic_storage_integration_5_0_6.py --profile modules_native --stage test --record /tmp/g16_module.json.gz
python3 scripts/verify_dynamic_protocols_reference_5_0_6.py --record /tmp/g16_protocols.json.gz
python3 scripts/verify_dynamic_protocols_reference_5_0_6.py --save-audit --record /tmp/g16_save.json.gz
```

The source drivers use independent locked Cargo workspaces and the unchanged
original observer files. All registry versions and checksums must match the
reference. Save/audit additionally uses the retained original generated Product
library and the original private event/privacy modules. The initial observer
dependency failure is retained; adding its existing reference `sha2` development
dependency changes neither C++ dependencies nor original source.

## Terminal evidence

The complete combined selection passes native **46/46** (41 targets, 167.01
seconds) and imported **45/45** (40 targets, 146.38 seconds). Every compiler and
linker exits successfully before execution. Registered commands and executable
hashes are checked again after completion:

- [Native integration](parity/g16_current_native_integration_5_0_6.json.gz).
- [Imported integration](parity/g16_combined_module_integration_5_0_6.json.gz).
- [918 exact-dependency protocol/loading observations](parity/dynamic_protocols_reference_5_0_6.json.gz).
- [5,144 exact-dependency Save/audit observations](parity/dynamic_save_reference_5_0_6.json.gz).

The retained 157 store and 72 provider reference observations supplement these
new replays. Outputs match both actual original Rust and the unchanged corpora;
no value/error normalization is introduced. All nineteen locked source bodies,
reviewed implementation hashes, observer inputs and executable hashes pass the
final audit.

Eight existing complete contracts and six unchanged observers now have canonical
imported consumers. Their first builds exposed four existing helper declarations
missing from the runtime facade and a global fixture helper named `sql` conflicting
with an imported namespace. The adapter qualifies calls while keeping its original
declaration. The [export-only proof](parity/g16_module_export_only_proof_5_0_6.json.gz)
restores the original facade/catalog/lookup exactly after removing four additions;
source headers and function bodies are unchanged. Initial compiler, pre-build
driver and observer-dependency failures remain in the acceptance record.

The current catalog verifies **1,503 exports** and **820 canonical bodies**.
[Full pinned formatting/check](parity/g16_final_format_5_0_6.json.gz) passes
**1,649 files**, preserving every byte and timestamp. The
[producer/SDK preservation record](parity/g16_producer_preservation_5_0_6.json.gz)
retains all 77 destinations and 3,468 file hashes. Existing Java75 and complete
54-positive/139-negative compiler evidence is reused explicitly; no producer or
generated artifact changed. Existing sanitizer evidence retains its original
scope; this export/test-registration work adds no sanitizer or MIPS run.

G08/G12/G13/G14/G17 and the overall P0-P7 exits remain open. This qualification
closes dynamic storage and readonly-property responsibilities, not complete
generator or release acceptance.
