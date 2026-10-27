# Nested signature and pattern metadata diagnostic, Rust 5.0.6

A fresh isolated comparison matches **4,973/4,973 outcomes** between the
unchanged original container-attribute parser and the installed Java metadata
parser. The results contain **3,301 accepted metadata effects** and **1,672
checked refusals**. No implementation or generated artifact changes were needed.
This is bounded diagnostic evidence; AP02/G14 and P0-P7 remain incomplete.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Input matrix and observation boundary

| Family | Construction | Inputs |
| --- | --- | ---: |
| Default | No attribute value | 1 |
| Nested signatures | 50 argument lists, ten qualifier spellings, four item containers and two metadata consumers | 4,000 |
| Generics and where clauses | Six generic lists, eight where clauses, four item containers and two metadata consumers | 384 |
| Patterns | 49 patterns, six expression contexts and two metadata consumers | 588 |

The item containers are a block, trait, implementation and foreign block.
Signature inputs exercise receivers, reference/lifetime spellings, destructured
and attributed arguments, variadics, qualifiers, malformed arguments and typed
generic paths. Pattern contexts are match arms, guarded match arms, local let,
let-else, for loops and closures. They include bindings, or-patterns, references,
tuples, slices, structs, paths, macros, literals, ranges and malformed forms.

Each expression is observed both as an ignored audit-length expression followed
by metadata effects and as a table-string consumer followed by an audit limit.
These contexts expose partial parsing, subsequent assignments and the distinct
string-literal refusal boundary. They do not require the nested function or
pattern to be semantically valid Rust code: the source contract parses an
attribute expression with Syn, rather than compiling or executing that nested
code. This diagnostic is not a generated entity compiler gate.

Every accepted original outcome is compared in full with the shared
type-preserving JSON comparator. An original refusal requires an actual Boolean
`false` Java refusal marker; diagnostic text/stages retain their existing
cross-language distinction. No expectations come from the Java implementation.
The earlier 1,043-case delegated diagnostic remains a separate run; these counts
are not a deduplicated cumulative corpus claim.

## Actual execution and provenance

The owned replay copies the complete **610-file runtime archive**, the unchanged
`rust_container_metadata_reference_5_0_6` manifest, lock and observer, and the
unchanged `ContainerMetadataProbe`. Cargo runs `--offline --locked` with a
private target directory; registry identities/checksums remain within the
reference lock. The actual compiler artifact has Debug assertions enabled. The
observer executes the complete unchanged `teaql-macros/src/attr.rs`.

Java compiles the installed `CppRustLiteralMetadata`, `CppRustXidMetadata` and
`CppJsonInputMetadata` sources with that probe. Existing Maven dependencies and
model/generator classes supply compilation dependencies, with hashes checked
before and after execution. This does not establish a fresh compilation of the
complete Java generator or actual model generation. Both observers emit one
result per input; their complete raw outputs are retained.

The controller verifies all copied sources, the reference archive, Java
dependencies and both reference identities after execution. It also confirms
that all execution inputs frozen by the separate native epoch remain unchanged.
No C++ runtime, sanitizer, MIPS/QEMU or physical-device execution is added by
this diagnostic.

## Evidence and remaining work

`docs/parity/audit_nested_signature_diagnostic_5_0_6.json.gz` retains the entire
matrix and controller, exact source/dependency hashes, archive, manifest/lock,
commands, compiler artifact, original/current results and raw outputs.
`full_grammar_qualified` and `semantic_accepted` remain false. These outcomes
support the corresponding nested child-rule review; they do not prove every
grammar branch or join metadata to every generated/protected consumer.

Complete AP02 branch/configuration/caller reconciliation, the separate
[column-attribute compatibility decision](field_attribute_boundary_5_0_6.md),
custom/protected provenance and current full integration remain independent.
Capability status stays **14 qualified / 3 partial**.

After all child processes exited, the replay removed **61,581,005 bytes** of
owned archive, target, Java classes and temporary data. The compact receipt
retains **1,138,504 bytes**. Stable preset outputs, shared caches and source
worktrees are preserved.

## Later permanent continuation

The [current source-derived checkpoint](audit_delegated_nested_checkpoint_5_0_6.md)
adds these same 4,973 inputs and complete original expectations to the installed
parser's permanent corpus. Its fresh original/current replay and 150 Java tests
pass. The diagnostic above remains a separate historical execution and does
not become complete AP02 grammar acceptance.
