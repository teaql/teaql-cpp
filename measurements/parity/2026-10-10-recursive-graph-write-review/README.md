# Recursive graph write review, 2026-10-10

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

Retained insert_graph_node_scoped/upsert_graph_node_scoped now have independent
C++ entry boundaries and Context-owned recursive operations. They preserve
one-child writes before the parent and many-child writes afterwards, source
reference/removal/scoping distinctions, forced/missing-ID insertion, declaration/
cardinality/target errors, attachment and canonical duplicate checks, checker/
dirty update projection, optimistic versions and authoritative reads. The public
batch planner remains separate. Captured parent traces, relation frames and
pending children allocate within User; no shared owner extends its lifetime.

792 controlled-provider inputs run twice through the actual original private
methods with access-only archive bridges. Complete returned graph Debug, all
provider/query/event/Checker calls and errors match on native, ASan/UBSan and
MIPS/QEMU. Cases cover all operations, typed/signed/full-width identities,
comments, one/many/empty/duplicate/nested relations, metadata/target/foreign-key
errors, dirty sets, snapshots, failures, missing-row insertion and custom IDs.
The earlier 1620/541/520 matrices remain in the selected checks.

Real SQLite tests enable foreign keys and triggers, proving actual child-parent-
many write order and per-node authoritative rereads. Recursive upsert preserves
versions, dirty updates and reference attachment; caller rollback removes every
row. Pending nested validation forwards wakes and cancels once before User
reclamation. Full recursive results are explicitly copied to Global and compared
after actual User destruction. Default PMR is disabled.

ASan and QEMU initially exposed a test-fixture ownership violation: stack
GraphNodes used User allocation but destructed after User reset. Those nodes
now use user.make<GraphNode>(); source bodies/expectations are unchanged. The
before-sanitize-ctest.txt and before-mips-ctest.txt logs preserve the failure.
Unmanaged PMR stack objects must never outlive their allocation Context.

After correction each five-check selection passes: native 4.44 seconds,
ASan/UBSan 10.57 seconds, MIPS/QEMU 6.60 seconds. Compiler/linker processes
finish successfully before use, and post-test selected targets report no work.
Sanitizer leak scanning is disabled. Full consumer and complete source-composition
validation remain in progress; these focus results alone do not accept Stage 5.
The generator/dependencies are unchanged; physical-router testing is deferred.


The later complete consumer/source-composition checkpoint is recorded in the
sibling runtime-graph-source-acceptance directory. It passes native 387/387 with
426 nonempty dependency records and ASan/UBSan/MIPS 44/44 after correcting an
older automatic-audited fixture with the same stack-PMR lifetime violation.
Before composition failures are retained here. That checkpoint accepts the
complete graph/save group and Stage 5; Stage 6 remains independent.
