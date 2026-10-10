# Graph node command review, 2026-10-10

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

Four original private entry boundaries are restored in graph_node_commands.hpp.
Reference validation rejects children/mutable fields before I/O, checks identity,
existence, strict I64 deletion/version state and reconstructs an authoritative
Reference with no caller comment/dirty/snapshot. Remove validation checks identity,
children, existence/deletion but deliberately does not compare expected versions.
The update helper marks Update before Checker/Fix, clears the marker after ordinary
check errors, preserves original values and strict I64 version, and projects only
mapped non-ID/non-version dirty fields. Scoped deletion preserves declared identity,
strict expected version and parent-to-child audit lineage before normal deletion.

The actual original private bodies run unchanged through access-only bridges in
an ignored locked git archive. 1620 inputs agree across two original runs and
match all three live C++ profiles, without output normalization. The matrix retains
complete raw models/commands, I64/U64/typed-null/text/full-width identity/version
shapes, metadata/children/mutable-field errors, empty/read-failing snapshots,
comments/parent scopes, Checker/Fix/error/erase cases, markers and provider/query/
event order. The existing 541 save-adapter and 520 execution observations pass too.

All four scoped checks pass on native (3.30 seconds), ASan/UBSan (7.31 seconds)
and MIPS/QEMU (4.33 seconds), after each compiler/linker process finishes. Post-test
selected builds report no work. Sanitizer leak scanning is disabled. Lifecycle
contracts disable default PMR, close a foreign parent-scope Context before lazy
deletion, preserve explicit copied lineage and signed-to-unsigned trace identities,
forward pending read wakes, cancel provider work exactly once on actual User close,
reclaim all User blocks and retain authoritative reference values through Global.
ScopedCommentNode remains a synchronous borrowed cons-list; the asynchronous
delete operation explicitly copies its filtered trace into the destination Context.

The six earlier save adapters plus these four boundaries are implemented. Retained
recursive insert_graph_node_scoped/upsert_graph_node_scoped and full source/stage
composition remain open. 385 tests are registered; the earlier complete native
381-check snapshot predates these edits. The ledger remains 53 accepted / 2 partial.
The source generator and dependencies are unchanged. Hardware execution remains
explicitly deferred; target evidence uses cross-compilation and QEMU.
