# Block ownership and array child rules against Rust 5.0.6

Status: BC01-BC04 are installed and qualified for the recorded original-source
matrix, permanent regression and generated descriptor/privacy callers. Complete
AP02 and G14 remain partial.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. The observer executes
the unchanged original attribute parser with locked `syn` 2.0.118. Its crate
checksum and the archived/unpacked `expr.rs`, `stmt.rs`, `item.rs` and `path.rs`
members are checked before execution.

## Source responsibilities

An ignored metadata expression still has to consume the same syntax as the
source. A nested expression error preserves preceding assignments, while a
successful non-string expression supplied to a string setting causes a separate
refusal. Incorrect block or delimiter handling can therefore change subsequent
entity identity, table/service routing and privacy declarations.

| ID | Source responsibility and shared Java correction |
| --- | --- |
| BC01 | `Block::parse` parses statements without admitting leading inner attributes. Its owners include `ExprIf` then/else blocks, `ExprAsync`, `ExprTryBlock`, explicit-output closure bodies and local `let ... else` divergence blocks. These call `block(false)`. |
| BC02 | `ExprBlock`, const/unsafe blocks, loop/while/for bodies and ordinary/trait/impl function bodies explicitly collect inner attributes before statements. They retain `block()` and its existing inner-attribute support. A nested expression block has its own permission; an enclosing plain block does not suppress its child's attributes. Macro token groups remain opaque. |
| BC03 | `array_or_repeat` selects repeat syntax only after the first expression. Once a comma has selected an element list, a later semicolon cannot turn the list into a repeat. The shared delimited-expression parser tracks that one decision. Tuple and call argument lists retain their existing comma rules. |
| BC04 | The source returns after parsing a repeat's length expression without requiring EOF in its bracket buffer. Some later balanced tokens therefore coexist with subsequent metadata effects. After a successful length parse, the adapter consumes the remaining token group without inventing another expression or rejecting the source's earlier effects. A failed length parse still fails at its original boundary. |

These are parser responsibilities, not evaluation or Rust type checking.
Preserving BC04 does not authorize arbitrary application input or establish SQL
execution behavior. No new compatibility exception is introduced.

## Source comparison and candidate

The [initial fixture](../tests/parity/audit_block_child_contract_inputs_5_0_6.json)
contains 618 expression shapes in eight numeric/string metadata contexts, for
4,944 new contracts. The [preceding parser](parity/audit_block_child_before_5_0_6.json.gz)
reproduces 1,200 differences: 960 block-owner and 240 array/repeat observations.
The [isolated candidate](parity/audit_block_child_candidate_5_0_6.json.gz)
matches all 31,938 complete original/current observations and preserves the
270 existing AST witnesses.

The [reviewed fixture](../tests/parity/audit_block_complete_contract_inputs_5_0_6.json)
retains those input strings and adds 200 repeat-prefix/list shapes. It also
corrects preliminary ownership annotations for async/try blocks: both source
implementations parse a plain `Block`; an implicit closure body or nested
`ExprBlock` remains a distinct owner. The final matrix contains **818 shapes /
6,544 new contracts**. The [expanded preceding parser](parity/audit_block_complete_before_5_0_6.json.gz)
reproduces **1,944 differences**. The same [candidate source](parity/audit_block_child_proposal_5_0_6.json.gz)
matches **33,538/33,538** [original/current results](parity/audit_block_complete_candidate_5_0_6.json.gz),
with no differences in retained expectations or AST witnesses.

## Installed verification

The permanent producer corpus retains all 6,544 expectations from unchanged Rust
execution. The [candidate/preceding Java gate](parity/audit_block_complete_candidate_regression_5_0_6.json.gz)
passes **125/125** tests with the corrected parser. The preceding parser fails
only the new regression, reproducing all 1,944 differences while preserving the
other 124 tests.

The [installed source gate](parity/audit_block_complete_installed_metadata_5_0_6.json.gz)
matches **33,538/33,538** complete metadata observations: 29,928 metadata effects
and 3,610 checked refusals. Its focused Java suite passes **125/125** tests. The
independent [AST/source replay](parity/audit_block_complete_installed_grammar_5_0_6.json.gz)
retains every existing source expectation and all 270 AST witnesses.

Twelve [fresh installed original/C++ caller pairs](parity/audit_block_complete_installed_callers_5_0_6.json.gz)
match 36 entity descriptors and 168 privacy classifications. Cases include
plain-block refusals, permitted nested/loop/implicit-closure attributes, a
misplaced repeat semicolon and source-preserved repeat tails. Execution disables
default PMR allocation and actually destroys `UserContext`. The
[sixteen strict controls](parity/audit_block_complete_installed_caller_controls_5_0_6.json.gz)
reject Boolean/number substitutions, missing or extra fields, partial rows and
duplicates using the actual current and archived comparison expressions.

Both isolated and [installed 80-SDK reproduction](parity/audit_block_complete_installed_sdks_5_0_6.json.gz)
preserve all 3,560 generated code, guidance and data files and 2,467 protected
artifacts. Each manifest changes exactly one parser-hash literal. The
[exact installation delta](parity/audit_block_complete_installed_delta_5_0_6.json.gz)
checks those bytes, the four source changes and all 1,813 C++ input identities
against the frozen [complete native 893-test checkpoint](native_provenance_integration_5_0_6.md).
The full execution retains its original parser identities; the current Java
continuation has separate source, caller and reproduction proofs.

All 17 current [native source/comparator/admission/inventory guards](parity/audit_block_complete_installed_source_guards_5_0_6.json.gz)
pass without skips or failures. The [frontend gate](parity/audit_block_complete_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its check,
locked upstream/generator verification and module-path verification. These are
scoped current checks joined to the unchanged-C++ full native checkpoint.

The [milestone receipt](parity/audit_block_complete_milestone_5_0_6.json.gz)
joins the current scopes, exact source/provenance delta and unchanged verdicts.
The [cleanup receipt](parity/audit_block_complete_cleanup_5_0_6.json.gz) verifies
removal of twelve task-owned probe directories containing 763,843,052 measured
file bytes after their processes exited. Sources, fixtures, compact evidence,
stable preset builds and shared caches are retained.

## Acceptance boundary

Full expression/statement/item dispatch, remaining type/path/binder/pattern and
lexical dependencies, every model configuration and complete metadata callers
remain AP02/G14 exits. Descriptor and privacy-classification callers do not
independently establish protected SQL or whole application behavior. G13/SQ02,
G17 and complete P0-P7 signoff remain open. Capability status stays **14 qualified
/ 3 partial**, with runtime source dispositions and the three approved
compatibility exceptions unchanged.
