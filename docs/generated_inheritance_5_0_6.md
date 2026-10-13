# Generated inheritance qualification: Rust 5.0.6

Status: partial P2 evidence. Normal C++ generation still rejects inheritance.
The development-only `--qualify-inheritance` option generates the test fixture;
its manifest records `qualification_only: true`. These artifacts do not establish
production polymorphic query or graph-save support.

## References and representation

Runtime: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` (v5.0.6).
Original generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The unchanged original output and provenance are retained under
`tests/generator/parity/inheritance_rust`; its complete generated crate compiles.

The original generator flattens predefined ancestor fields into each descendant.
It does not express this model with Rust language inheritance. School, Academy
and ResearchAcademy have distinct entity identities and table names. C++ follows
that representation rather than introducing native base-class ownership.

The fixture exercises two ancestor levels, inherited Context-root foreign keys,
required strings and integers, descendant decimal fields, and inherited audit
masks. Fixed field indexes/revisions remain distinct from descriptor ordering:
the latter places scalar fields before foreign-key fields, as the source does.
Explicit audit-mask declarations, including empty declarations, retain the
original entity-macro metadata contract.

## Evidence and scope

The paired original/C++ probes compare 48 observations without normalization:
complete and sparse fields; getter/eval/changed overlays; independent clone
views; native serialization; fixed layout indexes and revision hashes;
descriptor order/nullability/table identity; and inherited audit masks.
The source corpus is `tests/parity/generated_inheritance_5_0_6_cases.json.gz`.

The separate native integration test creates and saves a ResearchAcademy through
generated Checker and audited SQLite execution, queries inherited predicates,
reads parent/child E fields, and preserves sparse availability. It verifies an
explicit deep copy to GlobalContext and expired expression access after actual
UserContext deletion with the default PMR resource disabled. This is C++ runtime
integration evidence, not a paired Rust polymorphic-save comparison.

The selected native gates pass 4/4 (inheritance parity/integration and existing
Checker/business-ID contracts). The targeted sanitizer integration passes 1/1
with leak scanning disabled. Selected generator Java tests pass 14/14, including
the default inheritance rejection and explicit qualification-mode generation.
The original inherited output reproduces all 32 files, including provenance.
All seven C++ fixture sets reproduce byte-for-byte (77 files). The inherited
integration also passes through runtime named-module imports (1/1).

## Reproduction

Activate `scripts/toolchains/activate.sh` before CLI checks. Generate original
references with `scripts/generate_entity_references.py` and C++ artifacts with
`scripts/generate_examples.py`; use their `--check` options for byte verification.
Build `generated_inheritance_5_0_6` from the `rust_field_layout` Cargo harness and
`teaql_generated_inheritance_probe` from the native CMake preset. Replay the pair
with `tests/parity/generate_generated_inheritance.py --rust PATH --cpp PATH`.
CTest retains the corpus comparison as `generated_inheritance_rust_parity` and
the application integration as `generated_inheritance_contracts`.

## Remaining P2 work

Qualify source-defined type filters, parent/child mappings, child projections and
enhancements, recursive descendant loading, and graph/save responsibilities.
Add actual original query/save comparisons and associated lifetime checks before
removing the normal generation guard. G04 and complete P2 acceptance remain open.
