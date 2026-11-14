# Reverse-relation type composition against Rust 5.0.6

This is a bounded G14/AP02 continuation for the `reverse_relation(json_type=...)`
attribute. It uses locked runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
generator `45e1572c4956ca963410d8ea436ead3c8857087a`, and the original
`syn 2.0.118` parser. It does not complete AP02, G14, G17 or P0-P7.

The earlier [source probe](parity/ap02_reverse_relation_type_composition_exploratory_5_0_6.json.gz)
contains 4,000 actual original Rust and current Java metadata observations.
All matched. Of these, 1,649 retain a typed reverse relation and later entity/
limit settings; 2,351 retain the earlier settings after the type boundary.
The 4,000 rows contain **2,495 distinct inputs**, with no inconsistent result
among duplicates. The new permanent
[generator corpus](https://github.com/teaql/teaql-code-gen/blob/cpp_support/generator/src/test/resources/cpp_reverse_relation_type_composition_metadata_5_0_6.json.gz)
retains the first original result for each distinct input: 805 reach the typed
relation and 1,690 retain the earlier settings. Its source-receipt hash pins
the unchanged full probe. The existing `CppAuditMetadataCorpusTest` and
`verify_audit_metadata_source_5_0_6.py` now execute this corpus.

The [fresh source gate](parity/ap02_reverse_relation_type_installed_source_5_0_6.json.gz)
rebuilds the locked Rust observer and current Java parser. All **82,680/82,680**
retained-plus-new metadata results match, with zero differences; **159/159**
Java tests pass. All seven process steps exit successfully, the 79 recorded
inputs remain unchanged, and the 48,917,773-byte task-owned build is removed.
The generator reactor's selected Maven test also passes **1/1**, with zero
errors, failures or skips in its Surefire report.
An [initial attempt](parity/ap02_reverse_relation_type_initial_mvn_failure_5_0_6.json.gz)
stopped after the original Rust run because `mvn` was absent from `PATH`.
That failed receipt remains separate; the successful run used the locally
available Maven 3.9.16 executable.

Two source cases then became actual generated callers in the
[compiler comparison](parity/ap02_reverse_relation_type_compiled_callers_5_0_6.json.gz).
`type_composition_0086` uses the entity type `Self` and keeps the typed relation;
`type_composition_0000` has an invalid nested type and keeps the earlier
metadata. The original Rust entities compile and run, and C++ generation,
native compilation and execution pass. All **six entity descriptors and 28
field-policy observations** match; all 18 process steps exit successfully.
The 173,408,192-byte task-owned build is removed. This comparison checks
generated descriptors and privacy classification. It does not execute reverse
relation loading or a SQLite Save for these type cases.

A separate [failed compiler attempt](parity/ap02_reverse_relation_type_u32_compile_failure_5_0_6.json.gz)
shows the distinction between attribute syntax and a valid generated model.
Rust accepts `json_type="u32"` as a type expression, then the derived entity
fails compilation because `u32` does not implement `Entity`. The runner
stopped before native comparison and removed its temporary build. This is
recorded as a model-boundary failure, not as a passing generated caller.

The [permanent receipt guard](../tests/parity/verify_ap02_reverse_relation_type_corpus_5_0_6.py)
binds all 4,000 original outcomes, the exact 2,495-case corpus, current source
inputs, the installed source gate, generated callers and the
[five-SDK reproduction](parity/ap02_reverse_relation_type_independent_sdk_reproduction_5_0_6.json.gz).
It rejects six corrupt source, corpus and execution variants. Its native CTest
passes **1/1** under ordinary Python and **1/1** under `PYTHONOPTIMIZE=1`;
the [control receipt](parity/ap02_reverse_relation_type_controls_5_0_6.json.gz)
pins all six rejections and the source, corpus and execution hashes.
Strict `reproduce_cpp_contract_fixtures.py --check` also reproduces **50/50**
contract SDKs with zero code or Assist changes, and
`generate_examples.py --check` succeeds. No retained generated file was edited.

The contract reproduction check initially rejected a provenance JSON file
whose member order differed from current filesystem traversal order despite
equal generated file hashes and equal provenance values. Its check now compares
complete parsed JSON values after checking every generated file hash. The
initial failure was a verification-script issue, not a generator output change.

The current native registry contains **1,087** tests and **605** registered
Python entry scripts; **77** still contain **680** direct `ast.Assert` nodes.
This is enumeration, not a full 1,087-test epoch. Full AP02 composition and
consumer signoff, SQ02, G13/G14/G17 and P0-P7 remain open.
