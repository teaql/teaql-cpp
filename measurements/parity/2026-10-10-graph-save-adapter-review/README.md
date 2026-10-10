# Retained graph save adapter review, 2026-10-10

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

All six original internal entity/core-graph save adapters now have independent
C++ entry boundaries in EntityDataServiceAccess and entity_save_adapters.hpp.
They call repository extraction and governed graph save. Only status-only save
skips before extraction for no-persistence states. Commented status save does
not have that shortcut. Deleted status clears children; explicit Create and
comment overrides retain source ordering. Core EntityGraph conversion groups
duplicate ordered child names, preserves ordinary mutation values/comments,
maps Save/Delete to Upsert/Remove and starts without dirty/snapshot metadata.

The original methods execute unchanged through access-only bridges appended to
an ignored git archive. The source checkout and entity generator remain intact.
541 inputs were observed twice with identical outputs. All three live comparisons
match 541/541: complete returned GraphNode Debug, getter order, metadata/name
errors, status combinations, comments, original snapshots, provider/query/event
requests, failures and recursive core conversion are retained without projection.
The existing 520 execution observations also remain covered by the selected
CTest group. Provider/sink fixtures implement only their source SPIs.

The native, ASan/UBSan and MIPS/QEMU selections each pass 3/3, respectively in
1.72, 3.88 and 2.25 seconds. All compiler/linker sessions finish successfully
before execution; post-test selected builds report no work. Sanitizer leak
scanning is disabled. Lifecycle contracts disable default PMR and check lazy
construction, no-persistence/error boundaries, pre-poll cancellation, pending
provider wake forwarding, exactly-once cancellation during actual User destruction,
complete arena reclamation and explicit Global graph/snapshot/comment retention.

A stale native probe initially omitted the newly added raw outcome field.
That observation prompted removal of its exact compiled object and a fresh
terminal-successful rebuild; no expected output or source body was weakened.
Current raw diagnostics are compared in full.

The latest whole native 381-check checkpoint predates these adapter changes.
383 checks are currently registered; no complete 383 run is claimed here.
Retained recursive insert/upsert, reference/remove validation, checker update
and scoped deletion remain open. The ledger stays 53 accepted / 2 partial,
and stages 5–6 remain open. Physical-router execution is explicitly deferred.
