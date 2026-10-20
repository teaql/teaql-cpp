# Composed general query, Policy and diagnostic boundaries

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint advances G06/G08/G09/G10/G13/G14/G17 integration; complete
capability and P0–P7 acceptance remain separate requirements.

## Actual source execution

The observer calls the complete unchanged generated inheritance-graph library,
its original runtime and SQLite provider. It uses the existing schema, rows and
public synthetic private-field canary. Source dependencies retain the original
registry versions and checksums. No runtime algorithm or generated artifact is
replaced by an oracle implementation.

The 4,864 cases combine four models (School, Academy, ResearchAcademy and
Enrollment), all eight execution entries, plain/readonly/raw-field/raw-filter/
raw-statement/facet/grouped/missing-table shapes, four application policies,
logging on/off, ordinary/ID-set/continuous configuration and blank intent.
Optimization combinations apply to list/first/one/page; other entries retain
their independent execution contracts. Streams consume up to two items and close.

The declared outputs retain complete request Debug, Policy before/after Debug,
raw-surface detection, typed JSON and load states, readonly values, compact rows,
scalar/page results, errors, SQLite SELECT authorization callbacks and complete
stable SQL diagnostic fields. Wall-clock timestamps and elapsed durations are
outside this cross-process observation. Policy snapshots are trusted test
observations of requests, not safe diagnostic publication.

Original results contain 2,112 successes, 1,984 errors and 768 blank-intent
panics, with 4,664 Policy snapshots and 1,528 retained SQL records. Every original
observation is retained; provider failures are not converted into successful
queries or empty results.

## Reproduced runtime gaps

Initial native execution matches 4,080/4,864 outcomes. The
[complete initial record](parity/composed_general_initial_5_0_6.json.gz)
preserves both implementations' results.

Sixteen differences expose free-text intent in raw-statement diagnostics:
stream execution and continuous-page derived COUNT retain the canary in native
`comment`/`purpose`, while the original redacts it. SQL, business results,
parameter visibility and Policy callbacks already agree. This is a diagnostic
source gap, not a change to which rows the operation returns.

The remaining 768 differences are blank-intent error text. Rust panics during
purpose staging or generated comment validation. C++ retains its previously
accepted execution-time validation and raises `std::invalid_argument`. The
observation encloses construction and execution and records these language-level
failures in the same field; it does not claim identical panic/exception timing.
The native message now distinguishes the missing purpose and comment and matches
the corresponding original text. Both still reject before Policy and database I/O.

## Shared correction

The original stream entry always owns a query-scoped service built from its
explicit diagnostic source or its actual request. Native stream execution now
uses the existing scoped-query operation in the same way. That Context-owned
operation preserves diagnostic ancestry until delayed cursor consumption finishes.

The original prepared complex-query path similarly constructs a scoped service
with the full request and separately supplied aggregates. Native row execution
previously constructed an empty diagnostic scope in that branch. It now passes
a complete source snapshot, including aggregate diagnostic children, through the
existing checked scope builder and propagates scope-construction failures.

Execution queries and Policy invocation remain unchanged. The existing privacy
collector performs redaction; no model-specific masking is added to the observer,
generator or SQL provider. Queries, diagnostic children and operation state use
the execution Context. Wakes, cancellation and resource cleanup remain delegated
to the existing operation/stream protocols. No dependency or ownership domain is
added, and generated libraries do not need regeneration.

## Tested authorization and raw SQL boundaries

An independent evidence check verifies:

- 768 blank-intent failures reach neither Policy nor SQLite SELECT callbacks.
- 1,536 Policy denials reach no SELECT callback and publish no SQL record or row.
- All sixteen reproduced private-intent cases now contain the original redaction.
- Logging-disabled cases retain no SQL entries while execution/validation continue.
- All declared results and Policy query snapshots match actual original execution.

The application policy explicitly rejects raw surfaces when configured to do so.
A trusted complete SQL statement can define its own row/slice semantics; adding
an ordinary query predicate is not a SQL sanitizer or an authorization guarantee
for that literal statement. The tests retain actual original behavior and errors.
They do not rewrite trusted SQL to make it appear policy-safe.

The resource observation checks original pool-mutex availability and native
SQLite statement idleness after close. These are explicit ownership observations,
not a claim that both implementations use the same lease representation. Existing
native stream-reuse and deferred cancellation/Context-deletion checks provide
additional provider-lifetime evidence. No physical-router behavior is inferred.

## Source and evidence preservation

The [updated source review](parity/execution_responsibility_after_composed_general_5_0_6.json)
retains the earlier 37 complete locked execution/pagination bodies unchanged and
adds three complete original scope bodies. Current native input hashes identify
the reviewed implementation; counts/hashes alone are not semantic acceptance.
Earlier source reviews, the initial reduced panic observer, full original
preparation outputs and pre-repair runtime files are preserved separately.

The [paired repaired record](parity/composed_general_repaired_5_0_6.json.gz)
contains all 4,864 original/native outcomes with no SQL/result/error normalization.
The inherited models and their generated sources remain unchanged. Wider model,
configured provider, protected sink, async and final phase obligations remain.

## Completed checks

| Check | Result |
| --- | --- |
| Actual original/native live replay | 4,864/4,864 |
| Relevant native regression | 7/7 plus the independent Policy regression 1/1 |
| Imported-module comparison | 1/1, same 4,864 cases |
| Targeted ASan/UBSan | 2/2, leak scanning disabled |
| Independent authorization/privacy evidence | 768 intent failures, 1,536 denials and 16 precise privacy repairs |
| Locked source/generator inventory and reviewed field bodies | Verified; source identities are not full acceptance |
| Pinned C++ formatting and check | 1,629 inputs |
| Canonical imported bodies / catalog exports | 780 bodies / 1,495 exports |

The native selection includes real private-data sinks, provider stream cleanup,
deferred continuous-page source/context checks and generated scalar execution
ownership. No generator change, Java run or regeneration is required here.
Final formatter and canonical consumer checks are retained in terminal evidence.

## Reproduction

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_composed_general_probe
ctest --preset native --output-on-failure -R '^composed_general_rust_parity$'
python3 scripts/verify_composed_general_evidence_5_0_6.py
```

Final native/module/lifetime results, formatter checks and source hashes are
retained in [terminal evidence](parity/composed_general_terminal_5_0_6.json.gz).
No full-suite, Java/generator regeneration, MIPS or external-service result is
claimed for this runtime correction.
