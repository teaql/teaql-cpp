# Generated container metadata and runtime identities

The reference runtime is Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the generator reference is
`45e1572c4956ca963410d8ea436ead3c8857087a`. This checkpoint addresses the four
remaining declared container-setting differences from the
[expression/pattern checkpoint](audit_expression_patterns_5_0_6.md).
It does not qualify complete G14, G17 or P0-P7.

## Reproduced failure and source responsibilities

The [initial compiler comparison](parity/container_metadata_compiled_initial_5_0_6.json.gz)
generates complete original Rust and C++ libraries, compiles unchanged original
entity files through the actual derive macros and compiles the complete C++
facade. All five pairs compile and execute. The control matches; four disagree:

- `entity` changes the source trait identity, real model entity key and field
  layout identity. The C++ class name must remain the model's generated type.
- `table` replaces the descriptor's table independently of the entity name.
- `data_service` replaces the selected service metadata.
- `reverse_relation` appends an ordered descriptor with independently specified
  name, target, local key, foreign key and cardinality. Its logical name also
  belongs to the materialized relation set without receiving a fixed field index.

The previous interpreter treated those settings as unknown assignments/groups,
stopping before both their effects and a subsequent audit-length override.
The initial receipt retains actual differing identities and relation tuples;
successful generation alone is not its observation boundary.

The complete unchanged `teaql-macros/src/attr.rs` and `derive_impl.rs` determine
the contract. Known string settings require a literal string expression;
non-string expressions panic at the Rust attribute boundary. Ordinary syntax
errors preserve earlier effects in the same attribute. Unknown flags can
continue, while unknown assignments/groups stop parsing. Indexed-layout flags
are applied before a trailing-value error.

Each reverse relation commits only after its entire nested callback succeeds.
Malformed nested settings discard that relation and stop later settings; earlier
relations remain. Repeated relations preserve order and duplicates. Missing keys
default to `id`; missing cardinality means to-one. Attach and delete-missing
retain the existing runtime defaults. A malformed string-encoded `json_type`
stops parsing; a non-string expression has the separate source panic behavior.

## Runtime first, then generated adapters

The [runtime prerequisite receipt](parity/container_metadata_kernel_prerequisites_5_0_6.json)
retains the successful **2/2** field-layout and graph-JSON contracts, build log,
JUnit and relevant source hashes before producer changes. Existing runtime
mechanics support arbitrary entity identities, separate logical relation names
and typed reverse JSON resolution. No new runtime algorithm or ownership domain
is introduced.

The shared Java metadata interpreter now retains ordered container effects.
The generator emits thin adapters for runtime `ENTITY_NAME`, table/service
selection, appended relation descriptors and materialized relation names.
Class names, Q entry names, original model metadata, fixed indexes/mappings and
layout revisions retain their existing responsibilities. Overriding a parent
descriptor does not rewrite a child's original model metadata or inheritance.

Typed injected reverse relations use the existing runtime JSON projection
helpers. An absent `json_type` contributes metadata without inventing a getter
or JSON read. A typed read requires the relation to be loaded and uses its
Context-bound graph view; it performs no implicit SQL. Declared `crate::` model
paths map to their C++ model types. `Self`, the owning type and its `self::` path
retain the original entity-module scope. A bare foreign entity name is not
imported by the original module: the
[initial comparison](parity/container_metadata_type_scope_initial_5_0_6.json.gz)
retains its actual Rust compiler rejection while C++ previously compiled and
executed. The adapter now emits an explicit compiler rejection for that case.
Unrecognized external Rust types are not guessed to be a local model.

The additional [embedded-null compiler witness](parity/container_metadata_null_initial_5_0_6.json.gz)
reproduces a separate emission defect with successful compilers on both sides.
Ordinary C++ string conversion truncates every tested identity at the first NUL,
including a Unicode entity suffix. It also turns the source mask
`name\0suffix` into `name`, incorrectly changing the real property policy from
Plain to Masked. Entity keys, layout identity, descriptor table/service and all
four relation strings are observed rather than inferred from emitted text.
The [corrected compiled witness](parity/container_metadata_null_final_5_0_6.json.gz)
matches all three descriptors and fourteen real field-policy tuples. The emitter
uses existing length-bearing UTF-8 views, then copies descriptor strings into
their existing Context allocator.

## Actual-source parser evidence

The [complete parsed-effect observer](../scripts/verify_container_metadata_5_0_6.py)
includes the unchanged source parser, resolves exact reference registry
versions/checksums and observes entity/table/service, masks/limits, indexed state
and complete ordered relation metadata, including typed-relation presence.
The [final receipt](parity/container_metadata_parser_renderer_final_5_0_6.json.gz) agrees on
**1,519/1,519** inputs, including **45** source rejections. Rejection agreement
retains separate language errors; it is not identical error text or staging.

The unchanged audit-only observers separately pass
[1,452/1,452 observations](parity/container_metadata_audit_renderer_final_5_0_6.json.gz):
**1,424** exact valid outputs and all **28** source rejections. The
[archived/current comparison](parity/container_metadata_renderer_regressions_5_0_6.json.gz)
shows **4 improvements, zero regressions and zero remaining differences** within
that matrix. Both previous actual-entity compiler observer bodies remain byte
identical; the harness gains optional independent scenarios/observers.

The [Java gate](parity/container_metadata_java_5_0_6.json.gz) passes **101/101**
tests across six complete classes. It includes ordered overrides, nested commit
boundaries, string/type failure distinctions and generated class/revision/naming
preservation. The first new reference lock selected newer transitive registry
versions and was rejected before comparison. Its
[failed receipt](parity/container_metadata_parser_initial_5_0_6.json.gz) is retained.
The [actual failure log](parity/container_metadata_reference_lock_failure_5_0_6.json)
retains the identity assertion, and the corrected lock uses the reference
registry identities.

## Compiler, artifact and integration gates

The [descriptor compiler gate](parity/container_metadata_compiled_final_5_0_6.json.gz)
passes **10/10** configurations and **30** actual descriptors. Their complete
original/C++ SDK bytes [reproduce after the final emission correction](parity/container_metadata_descriptor_reproduction_5_0_6.json.gz).
The separate embedded-null compiler pair also matches all descriptor/policy data.

The [typed-JSON compiler gate](parity/container_metadata_json_final_5_0_6.json.gz)
passes **nine** valid model pairs and **54** actual graph views. It covers
untyped metadata, to-one/to-many, qualified paths/type trivia, independent target
metadata/type lookup, overridden owner identity, duplicates and `Self`.
Absent edges, unmarked views, empty/nonempty lists, NULL and present objects
remain distinct. The tenth configuration retains the authentic unqualified
foreign-type compiler rejection; both diagnostics are checked explicitly rather
than accepting any unrelated compiler failure. The initial
[smoke failure](parity/container_metadata_json_smoke_5_0_6.json.gz) retains that
original name-resolution error and a separate observer-only C++ rvalue misuse.
Original entity files were not edited to repair either observer.

All **41** earlier successful compiler/execution pairs retain their complete
[original/C++ SDK bytes](parity/container_metadata_prior_reproduction_5_0_6.json.gz);
this is byte reproduction, not another 41 executions. The
[strict artifact gate](parity/container_metadata_sdk_refresh_5_0_6.json.gz)
reproduces **78 SDKs / 3,457 generated files / 2,201 protected artifacts**.
Only producer fingerprints change; all checked-in code/guidance/data bytes remain.

The final [gate controller](parity/container_metadata_gate_controller_5_0_6.json.gz)
retains Java **101/101** after the final emitter changes. Installed
[native 4/4 and module 3/3](parity/container_metadata_installed_regressions_5_0_6.json.gz)
pass with stable no-work builds and six unchanged native archives. The
[source-preservation proof](parity/container_metadata_source_preservation_5_0_6.json.gz)
retains every existing expression/pattern/literal parser body and the relevant
runtime, model inheritance and source-disposition bytes. Active structure and
baseline verifiers pass; they are not semantic parity proofs.

The [terminal scoped qualification](parity/container_metadata_qualification_5_0_6.json.gz)
joins these gates. All seventeen capability statuses remain unchanged:
**11 qualified / 6 partial**. Source dispositions remain
**39 accepted / 83 pending / 2 excluded_external**. No sanitizer, MIPS, full-suite
or physical-device run is claimed for this producer-only correction.

## Remaining acceptance boundaries

These finite observers do not prove complete Rust expression/type/pattern/body
and lexical grammar equivalence. Typed-presence observations do not independently
prove every Rust type can be bound to a C++ model or external application type.
Descriptor/key/layout observations do not independently qualify all custom-service
assembly, query, graph Save and protected logging combinations after an identity
override. Those remain part of complete AP02 and the adjacent privacy/provenance
responsibilities.

The unanswered AP03/R08 wire-naming decision remains separate. Existing G08
quoted-FK aggregation and G15 quoted-FK outcomes remain unaccepted. Preserve the
last full native epochs and failures; this checkpoint cannot relabel a focused
selection as a full-suite pass. No source disposition or capability is promoted
by the parser or emission counts.

## Reproduction

Use fresh receipt paths. The default descriptor command now includes the
embedded-null witness and real field-policy observations. The JSON command
requires both the nine valid outputs and the designated compiler rejection:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_container_metadata_contracts_5_0_6.py \
    --require-match --output /tmp/teaql_container_descriptors_fresh_5_0_6.json.gz
python3 scripts/audit_container_json_contracts_5_0_6.py \
    --require-match --output /tmp/teaql_container_json_fresh_5_0_6.json.gz
python3 scripts/verify_container_metadata_5_0_6.py \
    --extra-cases tests/parity/audit_expression_operands_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_grammar_next_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_paths_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_delimiter_literals_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_patterns_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_pattern_boundaries_5_0_6_inputs.json \
    --require-match --output /tmp/teaql_container_parser_fresh_5_0_6.json.gz
```

These scripts use original generated files and locked-source observers. They
do not hand-repair generated entities or substitute local implementations for
the runtime's graph, metadata or field-policy behavior.
