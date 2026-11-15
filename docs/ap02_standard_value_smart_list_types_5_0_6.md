# AP02 standard-value and SmartList projection candidate

Status: compiled copied candidate; production installation is pending. The
current producer remains `cpp_support` commit
`5a6c7c898eb30cd1e401e890e5152747da7255d9`. The runtime and its managed memory,
query, execution and provider contracts are unchanged.

The locked references remain Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source responsibility and reproduced gaps

The original container attribute parser accepts a Rust type for
`reverse_relation.json_type`. That type must resolve to the actual projected
entity while retaining its scope and type requirements. The locked
`teaql-core/src/list.rs` independently defines owned, borrowed and mutable
`SmartList` iteration and a slice dereference target. The runtime's public
export is `teaql_core::SmartList`; its private module is a separate boundary.

The [standard-value diagnostic](parity/ap02_standard_value_type_diagnostic_5_0_6.json.gz)
builds 15 complete original/C++ model pairs. It reproduces seven C++ refusals
for valid owned/borrowed strings, slices and arrays. Unknown types and array
lengths, unsized errors and non-cloneable owned values retain checked refusals.
Both implementations already accept the tested boxed/referenced tuple with a
slice tail; the repair preserves that behavior.

The [SmartList diagnostic](parity/ap02_smart_list_type_diagnostic_5_0_6.json.gz)
builds ten complete model pairs and reproduces eight C++ refusals for valid
core-type projections. The owned and borrowed iterator item, mutable item,
intermediate iterator, owned value, slice target and erased Result error all
retain their independent Rust responsibilities.

## Copied implementation and actual evidence

The [standard-value candidate](parity/ap02_standard_value_type_candidate_5_0_6.json.gz)
passes 183 Java tests and 34 actual original/C++ compiler pairs: 23 accepted
and 11 refused. The production parser fails four of the five proposed
regression methods.

The [combined candidate](parity/ap02_standard_smart_list_candidate_5_0_6.json.gz)
passes 186 Java tests and 21 further actual original/C++ compiler pairs:
14 accepted and seven refused. It freshly regenerates all 34 standard-value
SDKs, checks their complete byte equality and explicitly reuses their prior
compiler results. The standard-value-only parser passes its five methods and
fails exactly the three additional SmartList methods.

The resolver retains distinct string, unsized string, array, slice and
SmartList shapes. Generic arguments keep recursive `Clone`/`Sized` checks and
modeled entity scope across associated-type erasure. These are generation-time
Rust type checks. Array-length interpretation currently covers integer literals
with no suffix or `usize`; other const-expression configurations remain AP02
work.

The [reviewable source patch](proposals/standard_value_smart_list_projection.patch)
changes one Java resolver and adds eight regression methods in two classes.
`git apply --check` passes against the unchanged production workspace. The
[patch receipt](parity/ap02_standard_smart_list_proposal_5_0_6.json.gz) binds its
complete text, candidate identity and producer revision. No generated fixture
has been hand-edited.

The [actual SQLite consumers](parity/ap02_standard_smart_list_candidate_sql_5_0_6.json.gz)
execute eight original/C++ model pairs with 40 matching query, E and JSON
results. This includes owned/borrowed strings, a borrowed slice, an array item,
owned/borrowed SmartList items, its owned slice target and a modeled child item.
The [candidate checkpoint](parity/ap02_standard_smart_list_candidate_checkpoint_5_0_6.json.gz)
binds the complete evidence and unchanged 11,983-input production cohort.

Next, install the reviewed producer change, qualify current production generation
and compiler outcomes, reproduce all retained SDKs, and admit the exact source
transition with ordinary/optimized controls and the affected registered
callers. A copied candidate does not establish installed production acceptance.

Complete G14/GP11, G17/GP16 and P0-P7 remain open at **14 qualified / 3 partial**.
The pending Option-model and column-attribute decisions are independent of
these confirmed C++ defects.

All owned probe output directories are reaped and removed after their compiler,
linker and executable processes finish. The checkpoint records approximately
2.47 GiB of measured removed outputs and 10.84 MiB of retained compact receipts.
Source, input models, commands, dependency identities, full outcomes and the
reviewable patch remain available for the production continuation.
