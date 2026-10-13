# Generated Business IDs for Rust 5.0.6

Status: supported-model implementation and focused source/native/lifetime/module
checks passed. G01/G03 and complete P1 remain open pending the full model/governance
matrix; this does not change the P0-P7 completion criteria.

## Runtime contracts and generated adapters

Each Business-ID field emits a distinct GeneratedBusinessId specialization, such
as ReceiptNumber and TicketNumber. Storage and mutation values remain text. The
application getter, setter, scalar predicates and E accessor use the distinct
type. create(memory_resource, text) trims source Unicode whitespace and rejects
blank text with the field-specific source error. It does not validate a profile:
the registered BusinessIdService performs that responsibility. Framework hydration
and getters can represent canonical empty storage independently of user creation.

The values support allocator-aware copies, equality, ordering, hashing, Debug and
explicit Value conversion. All nested text uses the destination resource; ordinary
copy construction and implicit string construction are unavailable. Raw values
are scoped to their allocation Context, like other allocator-aware kernel values;
they do not survive Context close. Retention requires an explicit destination copy.
Generated E wrappers retain their existing checked state/borrow contracts.

Generated Checkers delegate creation to GeneratedFieldChecks and the existing
BusinessIdService/profile/allocator layers. They require the trusted active root
before looking up the service. The slot reads stored text, while the source's
was_missing decision observes the mutation-aware getter. Successful allocation
sets the typed field and records Context evidence labeled businessIdAllocator;
validation of an existing value does not assign a replacement or invent evidence.
On update, predefined load checks precede the Business-ID dirty-field rejection.
That rejection preserves the source InvalidType rule and immutable-after-create
message. No new governance or allocation algorithm is emitted into a model.

setup installs the existing SQLite Business-ID allocator as both allocation service
and schema contributor when a model declares Business IDs. A daily-permuted-v1
application registers its explicit key provider through UserContext. C++ does not
read an environment key or introduce an implicit owner; this is the Context-owned
bootstrap adaptation of the source runtime's configured key provider. Assist lists
the actual typed methods, provider requirement and root setup.

## Producer metadata and validation

CppBusinessIdMetadata adapts the frozen producer's field/profile/default/root
metadata and KSML-BUSINESS-ID checks without changing other-language templates.
The Business-ID primitive and snake alias in system-types.xml are copied from the
frozen source. Required fields, supported profiles, prefixes, widths, date/reset,
separator, policy version and root-reference constraints reject before C++ output.
Definition tags live in generated_detail to avoid colliding with application
entity names. Ambiguous public wrapper names reject as a generated type collision.

## Source and integration evidence

scripts/generate_business_id_reference.py runs the clean producer at
45e1572c4956ca963410d8ea436ead3c8857087a on business_ids.xml. Its 25 original Rust
files are retained unchanged with a provenance manifest. The development oracle
compiles this crate against frozen runtime fe82a18982415f1746df9a2cf7c42f5e9abdc6d3.
No Rust/Java generator dependency is added to C++ production or ordinary builds.

The compressed 1,044-input corpus records wrapper creation, complete Debug and
definition metadata, direct generated Checker results in three naming profiles,
post-check values and availability. Inputs cover both profiles, missing and present
roots/services/keys, existing valid/invalid numbers, sparse/full fields, dirty and
clean updates, root mismatch and Unicode/long text. Raw original output is retained
in source_expected. The sole declared adaptation changes the exact missing-resource
message's Rust BusinessIdService type name to its existing C++ namespace. The
comparison checks that no other source field or diagnostic is altered.

Native contracts exercise real audited SQLite creation, typed query/readback,
root/Business-ID fix evidence, immutable rejection before the protected UPDATE and
unchanged database readback. Separate lifecycle tests explicitly copy a typed value
into Global storage, destroy its original User Context and inspect the surviving
value with the default PMR resource disabled.

Full inheritance/candidate composition, the complete field/model matrix, operation
trace/privacy/provenance authority and every P1/P7 exit gate still require evidence.
The direct Checker oracle does not claim a complete original audited-save/graph
pipeline comparison or physical-router execution.

## Checkpoint gates

Focused native checks pass 10/10, including the new source corpus and real audited
SQLite contract. Targeted ASan/UBSan checks pass 6/6 with leak scanning disabled.
Named-module import passes 1/1, all 1,324 catalog names resolve and seven facade
archives retain distinct module initializers without duplicated implementations.
Selected Java regressions pass 12/12, including invalid definition rejection.
The original 25-file Rust fixture plus provenance reproduces from its clean source.
All five eleven-file C++ fixture sets reproduce. Expanded compiler contracts pass
two positive and twelve negative cases, including raw text setters/predicates and
the wrong Business-ID specialization.
