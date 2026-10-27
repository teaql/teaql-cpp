# Delegated attribute grammar diagnostic, Rust 5.0.6

A fresh, isolated comparison finds **1,043/1,043 matching outcomes** between
the complete original Rust container-attribute parser and the installed Java
literal-metadata implementation. Of these, **801** preserve complete accepted
metadata effects and **242** produce checked refusals in both implementations.
This is a bounded diagnostic, not complete AP02/G14 or Rust grammar acceptance.

The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Inputs and comparison

The deterministic input matrix contains one absent/default input, 129 expression
shapes in six contexts, and 67 type shapes in four contexts. Duplicate complete
inputs are removed before execution.

| Family | Inputs |
| --- | ---: |
| Default | 1 |
| Direct expression | 129 |
| Expression followed by metadata effects | 129 |
| Parenthesized expression | 129 |
| Block expression | 129 |
| Table expression | 129 |
| Reverse-relation name expression | 129 |
| Cast type | 67 |
| String-encoded reverse-relation type | 67 |
| Reference type | 67 |
| Generic type argument | 67 |

The cases include attributes, literals, postfix expressions, operators, ranges,
blocks, control flow, closures, items, comments, type bounds and malformed
tokens. Following metadata checks whether the parser consumes the expression
boundary correctly and preserves partial effects. These cases do not enumerate
every delegated child rule or model configuration.

For an accepted original parse, the comparison checks every returned field with
the shared type-preserving JSON comparator. This includes limits, masks,
declaration state, entity/table/service identities, indexed layout and relation
metadata. A checked original refusal requires an actual Boolean `false` refusal
marker from Java; error text and diagnostic stages retain their existing
cross-language distinction. No expected values are synthesized from the Java
implementation.

## Executed sources and dependencies

The owned replay contains the full **610-file runtime archive** and the unchanged
`rust_container_metadata_reference_5_0_6` manifest, lock and observer. Cargo uses
`--offline --locked`; the selected registry identities/checksums remain within
the reference lock. The observer executes the full unchanged `attr.rs` and its
locked `syn`/`proc-macro2` dependencies. Its actual compiler artifact has Debug
assertions enabled.

Java compiles the installed `CppRustLiteralMetadata`, `CppRustXidMetadata` and
`CppJsonInputMetadata` sources together with the unchanged
`ContainerMetadataProbe`. Existing Maven dependencies and model/generator class
directories supply compilation dependencies; their hashes are retained before
and after execution. This does not claim a fresh compilation of the complete
Java generator or actual generated model execution. No parser overlay or
worktree implementation change is used.

Both execution streams contain one result per input and are retained completely.
Original sources, copied files, dependency identities and the frozen native
epoch's execution inputs remain unchanged.

## Evidence and remaining work

`docs/parity/audit_delegated_grammar_diagnostic_5_0_6.json.gz` retains the complete
matrix and controller, source/dependency hashes, archive map, commands, metadata,
compiler artifact, raw outputs and both result sets. `full_grammar_qualified`
and `semantic_accepted` remain false: successful diagnostic execution must not
promote the complete grammar or capability status.

A separate [nested signature and pattern diagnostic](audit_nested_signature_diagnostic_5_0_6.md)
adds 4,973 matching observations without changing this run's scope or counts.
AP02 still requires the complete branch/child-rule reconciliation and its joins
to generated metadata consumers. The separate column-literal/derive-attribute
decision in [field_attribute_boundary_5_0_6.md](field_attribute_boundary_5_0_6.md)
is unchanged. Complete protected publication, current integration and all
P0-P7 exits remain independent. The current native epoch continues separately
with frozen inputs; these results are not C++ runtime or native CTest results.

The task-owned Rust target, archive, Java classes and classpath file were removed
after all child processes exited: **61,581,060 bytes** measured. The compact
receipt retains **1,086,983 bytes**. Existing source, shared caches and stable
native build outputs are preserved.

## Later permanent continuation

The [current source-derived checkpoint](audit_delegated_nested_checkpoint_5_0_6.md)
adds these same 1,043 inputs and complete original expectations to the installed
parser's permanent corpus. Its fresh original/current replay and 150 Java tests
pass. The diagnostic above remains a separate historical execution and does
not become complete AP02 grammar acceptance.
