# Source qualification checkpoint (Rust 5.0.6)

This is a progress checkpoint, not complete migration acceptance. The runtime
and generator remain pinned to the commits in `upstream.lock.json`. Source
dispositions remain **118 accepted / 4 pending_review / 2 excluded_external**;
capabilities remain **11 qualified / 6 partial**.

## Current source reviews

The following reviews retain complete responsibility boundaries and fresh
canonical imported-consumer execution:

| Source | Current evidence | Disposition |
| --- | --- | --- |
| `teaql-runtime/src/graph.rs` | [Review](runtime_graph_source_review_5_0_6.md): 606 freshly reproduced original observations and 14/14 imported CTests | Pending complete integration and acceptance reconciliation |
| `teaql-runtime/src/data_service/resolved.rs` | [Review](runtime_resolved_source_review_5_0_6.md): 75 definitions, 26 responsibility groups and 37/37 imported equivalent command bodies | Pending complete integration and acceptance reconciliation |
| `teaql-runtime/src/lib.rs` | [Review](runtime_root_source_review_5_0_6.md): 75 original root contracts, 11/11 imported CTests and three rejection controls | Pending complete integration and acceptance reconciliation |
| `teaql-provider-sqlite/src/lib.rs` | [Review](sqlite_provider_source_review_5_0_6.md): 15,676 exact imported observations and 27/27 imported CTests | Pending complete integration and SP01–SP30 acceptance |

The [full native terminal receipt](parity/runtime_graph_model_full_native_terminal_5_0_6.json.gz)
records **876/877** passing tests, one failure, zero errors and zero skips.
The build, compiler contracts, no-work checks and all 11,412 frozen input checks
passed. The only failure was `stream_shapes_rust_parity`: its comparator still
rewrote 6,012 original observations to require the old eager C++ stream boundary.
The installed deferred runtime already matched all 13,104 raw source results.

The [stream review](protected_stream_shapes_5_0_6.md) records removal of that
obsolete rewrite, the independent strict replay and the historical negative
control. The [focused registered receipt](parity/stream_shapes_strict_registered_commit_5_0_6.json.gz)
passes the native and imported tests with **13,104/13,104** raw equalities each.
These tests are recorded separately from the failed full epoch. No passing
full-suite result is inferred from combining those receipts.
Original corpora and previous failure evidence remain immutable.

## Open metadata grammar differences

The [fresh grammar probe](parity/audit_grammar_current_probe_5_0_6.json.gz)
completed 222 inputs and found **18 differences** between the unchanged locked
Rust parser and the current Java C++ producer parser. `complete` in this receipt
means that the probe finished; it does not mean parity passed.

The differences cover nested attributes on struct fields, match arms and struct
patterns, plus chained comparison/range expressions. Each of six expressions is
tested directly, parenthesized and inside a block. Rust continues to the later
`audit_value_max_len=29` setting; Java retains the earlier value 17. These are
unresolved producer discrepancies. This checkpoint changes no Java parser or
generated artifact and grants no compatibility exception for these cases.

Disposable probe/controller outputs were removed after their processes exited;
compact receipts retain commands, hashes, complete observations and cleanup
accounting. Stable preset build directories are retained. This checkpoint does
not add sanitizer, MIPS/QEMU or physical-device qualification.
