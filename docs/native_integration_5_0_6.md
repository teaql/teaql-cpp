# Native integration while upgrading to 5.0.6

Status: the complete native build passes. Full native CTest acceptance remains
open; a successful build or selected regression is not P7 acceptance.

## Build and repaired fixtures

The first diagnostic build continued through all available targets and found ten
compilation failures. Current request interfaces require immutable query and
mutation intent; several historical probes still assigned the removed request
comment member or constructed mutations without intent. Private test functions
named `request` also collided with the exported request namespace.

Those targets now compile with explicit fixture intent, current request
accessors and descriptive private helper names. No default intent or runtime
bypass was introduced. A subsequent complete native build reports
`ninja: no work to do` and exits successfully.

Eleven selected native regressions pass after further fixture migration:
six generated Checker/Business-ID/inheritance/graph/variant/return-type contracts,
plus SQL move-only errors, streaming, transactions, dynamic relations and the
production SQLite regression. Generated fixtures reuse the root already prepared
by ordinary setup. Manual inherited assembly installs scoped persistent IDs
before bootstrap and updates the existing root row.

These are maintained C++ contracts. They do not replace actual 5.0.6 Rust
comparisons or relabel the unchanged historical Rust fixtures.

## Full test-run disposition

The initial full native selection completed with **292 passed / 197 failed out
of 489**, exit code 8. The [terminal per-test record](parity/native_integration_5_0_6.json)
preserves that run, before the later selected fixture/corpus repairs. Generated
compiler positive/negative contracts passed in 1,850.72 seconds.

The ledger execution fixture was unresponsive for over 30 minutes and was
terminated with SIGTERM; its failure remains in the initial record. Running it
as a debugger child located the wait in a root transport read while the native
transaction held its SQLite connection lease. The test now reads uncommitted
rows through the transaction query executor, retains rollback/Global-copy/actual
User destruction assertions, and has a 30-second timeout. Its native test passes
after the repair; targeted ASan/UBSan also passes **1/1** with leak scanning
disabled. Runtime lease isolation was preserved.

Already completed failures identify three required follow-up groups:

- Historical corpora explicitly reject the active lock because their observations
  belong to 5.0.5. Preserve them and replay the actual original calls against the
  locked 5.0.6 source to create separately identified evidence. Changing a commit
  label is not requalification.
- Native runtime and handwritten-example fixtures still enter queries or private
  mutations without the new explicit intent. Their operation/error/lifetime
  assertions must run with valid intent, including callback close, foreign-domain
  cleanup and pending work; fail-closed early rejection does not prove those
  downstream contracts.
- Additional trace, string-match AST, candidate and pagination/cache assertions
  require comparison with changed source contracts. The 5.0.6 source retains
  explicit purpose in normalized query identity and canonicalizes request trace
  comments. Do not restore the older behavior merely to satisfy stale assertions.

The initial full run also recorded repeated active-root assignment in six
generated fixtures. The selected follow-up above fixes those failures; it does
not erase them from the initial run or claim a later whole-suite pass.

The existing generated sample-execution source comparison passes in the full run
(324.20 seconds). Ordinary runtime assembly and privacy metadata retain their
separate source-comparison scope and provenance in
[generated_runtime_assembly_5_0_6.md](generated_runtime_assembly_5_0_6.md) and
[generated_privacy_metadata_5_0_6.md](generated_privacy_metadata_5_0_6.md).

A separately versioned [system-time error replay](system_time_error_scope_5_0_6.md)
now compares all 7,056 actual Rust observations with C++ without replacing the
historical corpus. Its active CTest family and the repaired ledger test pass
together **2/2**. This is subsequent scoped evidence, not a whole-suite rerun.

The same repeated root binding also affected five existing C++ oracle probes.
They now verify/reuse the generated root, preserving their original corpora and
frozen Rust drivers. All six selected comparison families pass: inheritance
SQLite, return type, audited save, optional graphs, branched variants and inherited
streams. Their scoped corpus counts remain 144, 702, 1,350, 2,313, 375 and 432
observations respectively. A final complete native build after all these repairs
again exits successfully with no work pending. No later whole-suite pass is
claimed.

## Completion requirements

Migrate/replay the remaining contracts,
rerun the checks affected by each correction and finish the integration gate.
The capability ledger remains partial and all 17 full capability qualifications
remain open. No sanitizer/MIPS/physical-device acceptance is inferred from the
native build.

Subsequent [runtime entry/example checkpoint](runtime_intent_entry_5_0_6.md) passes native 10/10 and targeted ASan/UBSan 7/7 (leak scan disabled). It supplies explicit intent so callback, foreign-domain and native completion cleanup tests reach their intended paths; two example scenarios and 34 ordered SQL entries now have separately versioned actual Rust replay. This corrects further initial failures without altering the initial result record or claiming whole-suite completion.

The later [native contract migration](native_contract_migration_5_0_6.md) preserves
a fresh 20/20-failing diagnostic and records a final selected **21/21** pass:
17 repaired maintained contracts, two reentrancy cases and two actual-source
comparison families. The 242 dual-scope and 336 public graph-plan observations
qualify the Unicode whitespace, Remove scope and positive trace-ID corrections.
Targeted sanitizer passes **12/12** and canonical module consumers **2/2**.
SQLite ID-space, automatic audited plain entity and relation execution from that
diagnostic remain pending, along with other historical source/corpus migrations.
The original complete 292/489 result remains unchanged; full CTest/P7 is open.

The [SQLite entry follow-up](sqlite_entry_followup_5_0_6.md) resolves the remaining
three fixtures from the 20-case diagnostic, with a combined **28/28** selected
native run, targeted sanitizer **3/3** and canonical module consumers **3/3**.
Actual locked Rust replay creates separate 5.0.6 transport (**798**) and ID-space
(**261**) corpora; active comparators use them without changing old evidence.
The historical async transaction/business-ID and other source families remain
separate pending migrations; the full-suite initial record is not replaced.

Post-readonly complete native run: [terminal record](parity/native_post_readonly_integration_5_0_6.json) accounts for all 499 registered tests, with 355 passes/144 failures and exit 8. The generated API compiler gate passes in 1,746.76 seconds. Every failure is a historical corpus/active-lock rejection after manual review of the three ambiguous triage entries. This does not qualify the subsequently replayed source families: independent actual comparisons expose raw LIKE, row diagnostics, invalid-intent fixture handling and missing audit-event identity/bootstrap/provenance responsibilities. The original 292/489 record remains unchanged.
