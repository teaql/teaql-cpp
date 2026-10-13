# Raw LIKE provenance correction

Status: production correction applied after complete CTest reaches terminal
state. Actual locked Rust/native query replay passes 1,089/1,089; the related
30-test native selection passes. Earlier isolated/failing records remain intact.
Reference: Rust 5.0.6, `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-core/src/expr.rs`.

## Actual mismatch

The nine separately replayed historical API families contribute **4,007** actual
original observations. Eight complete families match current native C++:
DataType (10), Entity defaults (102), SafeExpression (448), owned Value (171),
Value API (115), metadata (344), mutation models (1,688), and SQL models (40).
That is **2,918/2,918** matching observations. The remaining query family matches
1,047/1,089 before correction.

All **42** query failures differ only in two fields: the structural debug output
of `like` and `not_like`. C++ routes both through the typed string-match builder,
which emits a LikePattern leaf. Actual Rust constructs an ordinary Text Value.
The source intentionally reserves LikePattern provenance for typed
contain/begin/end helpers, preserving their original operand separately from the
execution pattern. Raw SQL LIKE patterns must retain their ordinary literal AST.
This is a runtime defect; replacing only expectations would conceal it.

The [complete difference record](parity/query_api_source_gap_5_0_6.json) preserves
all failing input indexes and exact differing paths/values. It is independent of
the complete CTest run, whose query comparator still uses historical evidence.

## Staged correction and isolated verification

The proposed correction changes only Expr::like and Expr::not_like to construct
Binary LIKE/NOT LIKE with a Text Value. The typed helper and its provenance
remain unchanged. No signature, Context ownership, allocation scope, SQL layer
or dependency changes. Raw wildcard bytes remain caller-supplied.

An isolated copy of expr.cpp is compiled with the locked native compile flags;
its object is linked with the existing, finalized query probe object and native
archives into a separate temporary executable. Native build outputs and source
are not overwritten. Every compiler/linker process exits successfully before
execution. Actual locked Rust plus that variant matches **1,089/1,089** query
observations, without output normalization or input changes.

The initial link orchestration incorrectly treated CMake's `: && ... && :`
wrapper as an executable; both attempts fail before linking/execution. The
corrected invocation extracts the compiler arguments and succeeds. Those failed
attempts are not qualification results. The isolated source comparison is a
validated proposal, not an applied production fix or a whole-suite pass.

The complete 499-test run terminates with 355 passes/144 historical-baseline
failures; its compiler gate passes. The two-function correction is then applied,
with explicit raw-leaf/typed-provenance regression and pinned format/check
(989 files). Actual production query replay passes 1,089/1,089. All 25 migrated
source families (12,282 observations) plus query model/API, trace and log privacy
contracts pass 30/30; their probes are relinked against the corrected core.
Active selections use the new corpora, while old evidence stays intact.
The [terminal selected record](parity/raw_like_and_api_migration_5_0_6.json) does
not replace either complete-run record. Four replayed failing families retain
their historical selections pending actual correction. Complete G06/G07/G13/G17
and P7 remain open; no sanitizer/module/MIPS acceptance is inferred.
