# AP02 cross-context expression composition checkpoint

This checkpoint uses locked Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. It tests source expression
composition in audit-length attributes. It does not qualify the full Rust
grammar, G14, G17 or a P0-P7 phase exit.

## Source-derived inputs and execution

The [92-input fixture](../tests/parity/audit_cross_context_composition_5_0_6_inputs.json)
crosses ten expressions with direct, parenthesized, call-argument and closure
block contexts and both `audit_value_max_len` keys. Six malformed expressions
also exercise both keys. Each input sets an earlier entity and length, then a
later entity, table, service, mask, indexed layout and length. The actual locked
Rust parser determines whether later settings take effect; fixture labels do
not supply expected output.

The [source comparison](parity/audit_cross_context_composition_maven_5_0_6.json.gz)
builds the unchanged Rust observer against the locked `syn` dependency and
compiles the current Java metadata adapter. Its complete retained-plus-new
matrix matches **80,277/80,277** metadata results with zero syntax-witness or
retained-source differences. All 92 new cases match: 90 retain a parsed
metadata result and two refuse at the source parse boundary. Several malformed
expressions parse as a container but preserve only earlier metadata; this is a
partial-effect result, not a successful later setting. All six process steps
exit successfully, tracked inputs are unchanged and the 48,475,961-byte
task-owned build directory is removed.

The [first attempt](parity/audit_cross_context_composition_initial_5_0_6.json.gz)
stopped after Rust execution because `mvn` was absent from the active `PATH`.
Its incomplete receipt remains unchanged, and its temporary build was removed.
The successful comparison used the locally cached Maven 3.9.6 executable; no
new Maven installation or source change was required.

## Permanent metadata and generated callers

The [source-bound corpus](https://github.com/teaql/teaql-code-gen/blob/cpp_support/generator/src/test/resources/cpp_cross_context_composition_metadata_5_0_6.json.gz)
pins the successful source receipt and all 92 original outcomes. A dedicated
method in the existing `CppAuditMetadataCorpusTest` checks the current Java
adapter against them. A generator-only Maven invocation first reached a stale
`model` module class and failed to compile an unrelated test; rebuilding the
reactor with `-pl generator -am` passes the selected method **1/1**.

The [fresh generated-caller run](parity/audit_cross_context_callers_5_0_6.json.gz)
selects two later-setting cases and one malformed partial-effect case. It
generates complete original Rust and C++ models, compiles actual original
entities and the C++ facade, then compares **3/3 model pairs, 9 descriptors and
42 field-policy results**. The successful cases derive entity `After` and audit
length `31`; the malformed case retains the earlier length `13` and the
default Item identity. All 23 source/build/execution steps exit successfully,
all 524 tracked inputs remain unchanged, and the 174,402,561-byte task-owned
build directory is removed. This is descriptor/privacy evidence; it does not
run a new SQLite Save or sink publication matrix.

The [permanent provenance verifier](../tests/parity/verify_audit_cross_context_corpus_5_0_6.py)
checks the exact source receipt, original and Java process outputs, the corpus,
all current input hashes and actual generated-caller results. Its
[control receipt](parity/audit_cross_context_corpus_controls_v2_5_0_6.json.gz)
rejects **14/14** incomplete, changed, empty and truncated variants. The
verifier also passes under `python3 -O`. The new native CTest passes **1/1**;
the existing optimized-source and generated-Credential receipt checks pass
**2/2** after registration. The native registry now contains **1,078** tests;
there is no new full-suite result for that registry.

No C++ runtime algorithm, generator implementation, checked-in generated SDK
or ownership contract changed. The full AP02 review still needs the remaining
expression/type/pattern/statement/lexical composition and generated/protected
consumer signoff. The separate column-literal compatibility choice remains
pending.
