# Current query comparator and registration review

Status: finite source/registration review, not complete SQ02 acceptance. The
current native registry contains 736 tests. The unchanged simple inventory
resolves 236 direct active-baseline bindings and leaves 136 custom registrations
for their actual source/provenance/adaptation review. These counts supersede
older registry counts only as locators; they do not qualify runtime semantics.

The [source and command receipt](parity/current_dispatcher_locator_review_5_0_6.json.gz)
retains the complete registry, exact CLI commands, current inventory and complete
shared/execution comparator source bodies and hashes.

## Existing replay evidence must not be repeated as missing work

The active base/Context, entity-stream and SQL-stream CMake bindings already
select separate 5.0.6 corpora. Historical missing-intent inputs remain negative
evidence; they must not be described as successful provider execution. Explicit
intent matrices supply the corresponding positive paths:

| Existing qualification | Actual scope |
| --- | --- |
| [Base/Context replay](layered_query_replay_5_0_6.md) | 724 unchanged historical observations and 2,172 separate explicit-purpose observations; fixture recording is distinct from final SQL/privacy publication. |
| [Typed query/stream replay](typed_query_replay_5_0_6.md) | 943 complete observations over five families, including 193 entity-stream inputs; separate provider/model combinations retain their wider obligations. |
| [SQL protocol replay](sql_protocol_5_0_6.md) | Actual SQL/executor/stream protocol inputs and their explicit intent controls; inspect its source/dependency and observer boundaries before whole-source acceptance. |

Some older G17 questions still describe these historical probes as awaiting
their first baseline replay. Reconcile that wording with these completed scoped
receipts, preserving the actual broader source/model/call-site obligations.
The ledger itself is an execution input of the running native epoch and remains
frozen until that epoch terminates.

## Comparison completeness and command selection

The shared `compare_values.observe` executes the actual command, checks its exit
status, frames records only on LF and rejects a different output-record count.
The shared `same` checks JSON types, complete object keys, array cardinality and
floating signed zero. A downstream zip over two already checked equal-length
streams is not by itself a reproduced false pass.

The reviewed generated execution/options/result-model/hot-cache dispatchers
use that observation boundary before their whole-output comparisons and
additional invariants. Stream-shape comparison separately applies the existing
explicit eager-C++/lazy-Rust startup disposition. Its unadapted source outputs,
adapted requirements and native results remain distinct. Reading these helpers
does not establish every corpus's dependency provenance or accept their complete
normalization/call-site responsibility.

All **127** active commands whose local parser explicitly declares `--cpp`
supply that argument. Optional source-only modes therefore are not selected by
those registered commands. This check does not resolve inherited parsers,
alternative executable flags, actual result correctness or all 136 custom
registrations. In particular, invariant-only helpers need their own review;
they are not fabricated missing C++ comparisons.

No comparator, corpus, source observer or production implementation changes.
No new false-pass defect is claimed. The prior
[reproduced resource-comparator repairs](comparator_result_contracts_5_0_6.md)
remain independent concrete defect evidence. The
[new native integration epoch](native_integration_5_0_6.md) is running with all
execution inputs frozen; its outcome and complete SQ02/G13/P0-P7 acceptance
remain pending.
