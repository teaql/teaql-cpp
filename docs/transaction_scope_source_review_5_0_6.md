# Complete Context transaction source review

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This review covers all 266 lines of
`teaql-runtime/src/context/transaction.rs`. The complete C++ counterpart is
`teaql_runtime/include/teaql/runtime/transaction.hpp`, with its public Context
declaration in `context.hpp`. The source file has no embedded original tests.

Status: pending. The current C++ implementation already contains the new
transaction-local audit collection and post-commit delivery. It lacks the two
new named Send entry points. An isolated additive header repair passes without
changing the execution inputs of the running full native epoch. Its
[terminal shadow receipt](parity/transaction_send_shadow_5_0_6.json.gz) records
68 exact original/shadow SQLite outcomes, both expected Rust Send refusals and
two complete C++ move-only cleanup programs. Neither a temporary repair nor
historical 5.0.5 evidence establishes installed 5.0.6 source acceptance.

## Whole-source responsibility map

| ID | Original responsibility | C++ mapping and boundary |
| --- | --- | --- |
| CT01 | Scope structure and must-use contract | A distinct `TransactionScope<E>` borrows its Context, owns access to one native transaction and remembers flushed ledger identities. C++ marks the scope `nodiscard` and prevents copying; retaining a wrapper does not retain its Context. |
| CT02 | Private typed start | `ExecuteInTransaction::poll` requires the registered exact executor type, awaits begin, maps errors to `RuntimeError::Transaction`, then allocates the scope. Begin failure does not invoke the business callback. |
| CT03 | Ordinary Context entry | `execute_in_transaction` stores one callback and returns a Context-bound operation. Source callback output has no Clone requirement. Ordinary source work may use a thread-local future. |
| CT04 | New Send Context entry | Rust adds `execute_in_send_transaction`, requiring the callback's boxed future to implement Send. The current C++ name is absent; the isolated repair delegates to the existing execution implementation under the documented owner-thread adaptation. |
| CT05 | Context getter and ambient execution | `context()` returns the original Context. Scope services and requests use the transaction executor. Explicitly calling the surrounding Context still uses its ambient executor; opening a scope does not install an implicit transaction into Context. |
| CT06 | Transaction access after completion | Rust consumes `self` and takes its optional transaction at completion. C++ prevents repeat execution and later provider access using checked state and native consumed state. Repeated public completion is a C++ misuse diagnostic, not a new source Result variant. |
| CT07 | Transaction-bound entity service | Check entity registration first, preserve `MissingEntityDataService`, then construct the service against the actual transaction. No ambient executor fallback is allowed for that service. |
| CT08 | Query observation | Build the select observer from the Context flag, execute through the transaction's observed query protocol, map provider errors, then record returned metadata. `ScopeRequest` retains distinct QueryRequest/QueryResult types. |
| CT09 | Mutation observation | Independently gate mutation observation and use the transaction mutation protocol. Intent, provenance and privacy continue through Context metadata handling; the scope does not invent a second logging backend. |
| CT10 | Audited ledger save | `ScopeAuditedSave` calls the shared ledger saver with the same transaction and Context. Returned entities retain their source loading and ownership contracts. This entry does not duplicate graph persistence. |
| CT11 | Pending ledger collection | Remember only a returned ledger, deduplicate by ledger identity and keep pending intent until commit succeeds. Rust recovers a poisoned ledger mutex; the approved C++ owner-thread profile uses a Context-owned vector rather than a separately poisonable mutex. |
| CT12 | Ordinary scope execution | Invoke the FnOnce callback once inside transaction-local audit collection. C++ `ContextOnceFunction`, `AuditActivation` and `ScopeExecution` retain callback work across suspension without putting the queue on Context or extending Context lifetime. |
| CT13 | New Send scope execution | Rust adds `execute_send` with the same body and completion sequence as ordinary `execute`, changing the callback future bound. The C++ named entry is absent; the isolated repair delegates to `execute<T>` without duplicating completion logic. |
| CT14 | Successful completion | Consume the transaction and await commit. Only successful commit drains and clears every distinct remembered ledger. C++ holds callback output across a suspended terminal operation and supports move-only output. |
| CT15 | Failed commit | Return the mapped transaction error before ledger clearing or audit delivery. Pending facts are discarded, and provider cleanup releases the consumed transaction. A failed commit is distinct from post-commit audit delivery failure. |
| CT16 | Business failure and rollback | Await rollback. Successful rollback returns the original business error; failed rollback returns the exact combined operation/rollback diagnostic. Pending ledger intent remains retryable. |
| CT17 | Post-commit audit delivery | After commit and ledger clearing, deliver the collected events through the shared audit helper. Delivery failure returns `AuditAfterCommit`; it cannot mean that already committed writes rolled back. The helper continues attempting later events. |
| CT18 | Cancellation, exceptions and allocation failure | C++ explicitly cancels and joins child and terminal work, abandons the native transaction, restores audit activation and discards queued facts before arena reclamation. These are lifetime adaptations of owned Rust futures and provider Drop, not fabricated source Result branches. |
| CT19 | Generic and ownership boundaries | The original introduces no Clone, Default, equality, Debug, Hash or serde contract for Scope. Context/source identity and cross-domain callback checks remain native ownership requirements; there is no query-private arena or implicit shared owner. |
| CT20 | Nested scopes and provider boundaries | This source introduces no universal prohibition on nested transactions. The provider decides whether an attempted begin is supported. Audit collection has its own operation-local nesting/restoration contract; it is not evidence of nested database transaction support. |

## Exact 5.0.5 to 5.0.6 changes

The previous source has 212 lines. The current source adds the Context Send
entry, the Scope Send entry, transaction-local audit collection around ordinary
execution, post-completion delivery, and a shared private `complete` method.
The ordinary and Send execution bodies both collect callback facts, await
completion, then deliver. Their difference is the callback future's Send bound.
The source does not add a blanket Send bound to F or a Clone bound to T.

The [historical source contract](transaction_scope_api_contract.md) already
records the C++ adaptation of Rust Mutex/Send/Sync to explicit Context-owner
execution. It accepts neither unrestricted concurrent Context allocation nor
thread transfer. Restoring the two source entry names must preserve that
boundary; adding a method named Send is not proof of a C++ Send auto-trait.

## Existing current-baseline evidence and its limits

The [intent checkpoint](transaction_scope_intent_5_0_6.md) exercises the actual
complete original runtime and SQLite provider. Its 64 request/intent cases and
four LF-framing cases compare steps, persisted rows and safe log fields. It
does not exercise the newly named Send entry or prove every generic completion
path. The older 1,288-case scope replay and non-Clone fixtures remain separate
evidence with their own observation projections and dependency provenance.

The accepted [commit-audit source](commit_audit_source_review_5_0_6.md) covers
the actual private helper, original tests, fan-out, nesting, activation
restoration, reverse completion and raw event preservation. Existing real SQLite
scope tests cover shared transaction writes/reads, rollback, ledger clearing and
explicit Global retention. These checks support CT05-CT18 but cannot supply the
missing public entry names or automatically qualify this whole Context file.

## Isolated repair and remaining integration

The temporary Rust fixture calls both actual 5.0.6 Send entries. Positive
compilation checks a Send callback and preserves the ordinary entry's ability
to hold Rc across an await point. Two negative builds must fail specifically
because an Rc held across await cannot satisfy the Send callback future bound.
The original SQLite observer is copied with exactly one entry-name substitution;
no original runtime or provider implementation changes.

The C++ shadow uses copies of only `context.hpp` and `transaction.hpp`. It adds
the Context declaration, Context delegating definition and Scope delegating
definition. A diff guard rejects removal of any existing line. A compiled
public API probe distinguishes both missing names in the installed source from
both present names in the shadow. Ordinary completion logic stays shared.

The direct-scope move-only fixture initially treated `Ref<Ambient>` as a
resource borrow, then as a pointer. Both compiler rejections are preserved in
the [initial receipt](parity/transaction_send_shadow_initial_5_0_6.json.gz) and
the [accessor continuation](parity/transaction_send_shadow_accessor_failure_5_0_6.json.gz).
The actual reference API is `user_context.get(reference)`; the corrected fixture
resolves the executor through its owning Context before beginning the provider
transaction. No runtime accessor or ownership rule is changed to accommodate
the fixture. A failed compiler output is never linked or executed.

The header gate also compiles and runs the name probe before and after the
additive patch: `0 0` becomes `1 1`. Both source-locked original ordinary and
Send observers match all 68 unchanged observations. The Context Send entry
and direct Scope Send entry each pass the complete move-only fixture covering
suspended success, failed commit and actual Context destruction. Unique output
destruction, callback invocation count, native transaction cleanup and zero
remaining User bytes retain their assertions. All 129 resolved registry
identities belong to the exact reference lock.

A separate [module shadow receipt](parity/transaction_send_module_shadow_5_0_6.json.gz)
records compilation of the unchanged runtime and SQLite facade sources against
the additive headers, followed by canonical imported consumers. That gate is
terminal and successful: 68 exact original/imported outcomes, both new names and
both complete move-only programs pass. Eight canonical consumer/fixture bodies
retain their prepared semantics. Original facade sources, installed BMIs,
module/native archives and all frozen execution inputs stay unchanged. This
is temporary imported qualification, independently of installed module acceptance
and the separate full native epoch.

The [full-body continuation](parity/transaction_send_full_contracts_5_0_6.json.gz)
reuses the complete current `transaction_scope_tests.cpp` and
`commit_audit_transaction_tests.cpp`, replacing only the Context entry name.
It compares original and prepared canonical bodies before creating each variant.
Header and imported variants retain the original assertions for real SQLite
reads/writes, begin/commit/rollback failures, pending ledgers, foreign work,
callback failures, allocation failure after begin and actual Context deletion.
The audit body retains physical commit visibility, ledger clearing before
delivery, cancellation and the distinct post-commit sink failure. Its automatic
graph-save cases remain unchanged controls. All four programs pass: complete
scope and audit bodies through headers and through canonical imports. Every
compiler and linker terminates successfully before execution; pinned formatting
and its check pass for each temporary variant. The terminal receipt verifies
unchanged original fixtures, installed archives/BMIs, temporary facade artifacts,
all 10,653 frozen execution inputs and both exact reference identities.

The full native epoch's 10,653 execution-input hashes, both reference identities
and completed native link archives are checked independently. Source, generator,
generated artifacts and existing test bodies remain unchanged. Temporary C++
files use the pinned formatter and its check. Each compiler/linker must exit
successfully before the controller uses its output.

## Prepared permanent integration

The [staged patch](parity/transaction_send_staged_patch_isolated_bins_5_0_6.json.gz) preserves
all eleven proposed file contents, original/staged hashes and reviewable diffs.
The working copy is `/tmp/teaql_transaction_send_install_5_0_6`; nothing in that
patch is installed while the current native epoch runs. It includes both public
entries, a six-scenario runtime unit, the original Rust callback trait fixture,
an exact-source reproduction command, native/module registration and optional
per-contract module selection.
Its ordinary and Send Rust observers use distinct binary names in the shared
development Cargo target. The [earlier staged patch](parity/transaction_send_staged_patch_5_0_6.json.gz)
is preserved before that verifier-only naming refinement.

The [candidate receipt](parity/transaction_send_candidate_5_0_6.json.gz) is
terminal and successful. The dedicated unit passes headers and an actual
`import teaql_runtime` consumer, linking without the SQLite provider or SQLite
archive. The new optional `import_module` field selects both the prepared
import and its CMake dependency. Existing contracts keep their default facade:
all 1,027 consumer/fixture files and the manifest reproduce byte-for-byte.
Including the new entry and fixture, all 1,029 canonical bodies and declared
import prologues verify. The import guard checks the generated prologue
separately from authentic imports in the preserved source body.

The initial experiment retained the default provider import while selecting a
new runtime BMI and an old provider BMI. Its compiler exits with SIGSEGV; the
[initial receipt](parity/transaction_send_candidate_initial_5_0_6.json.gz)
preserves that invalid mixed-input experiment. No failed object is linked or
executed. The corrected consumer imports only runtime, removes the provider
mapping and links the matching runtime facade. The
[missing-fixture continuation](parity/transaction_send_candidate_fixture_failure_5_0_6.json.gz)
and [overbroad import-guard failure](parity/transaction_send_candidate_import_guard_failure_5_0_6.json.gz)
also remain preserved. Their continuation copies the required original fixture
and checks only the generated prologue, without changing original bodies or
rejecting their authentic conditional imports.

The staged permanent reproduction command Python-parses and provides its CLI;
installed execution is still required after the epoch ends and the patch is
applied. It reuses both immutable intent corpora and complete existing
transaction/audit bodies through each frontend. No installed CMake configure or
consumer result is claimed for a patch that has not yet been applied.

After the native epoch terminates, the additive entries still need installation,
permanent compiler/source reproduction, canonical module consumers and relevant
native/module tests. No new sanitizer, MIPS, physical-device result, source
promotion or complete G17/P0-P7 acceptance follows from this temporary check.
Source dispositions remain 66 accepted, 56 pending and two external exclusions.
