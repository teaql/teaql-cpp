# Protected hot-cache execution and repository lookup failures

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This advances G09's complete source/branch review. It also supplies scoped cache,
readonly-model and protected-execution evidence to G13/G15/G17; complete
capability and P0-P7 acceptance remain separate.

## Actual default stores and the missing execution combination

Both implementations already provide real in-memory ID-set and continuous cursor
stores by default. Rust constructs them with UserContext; native resolves them
lazily into the explicit User arena. Installing an observer's replacement store
is not what makes ordinary queries use a real store.

Earlier single-execution protected matrices exercise misses and fallback paths.
The new unchanged-original/native observers keep one Context alive across complete
requests, then repeat the request, change Policy, modify matching data, clear the
aggregation cache or replace pagination stores. Replacements happen between
completed operations; safe Rust permits this mutation. The earlier E0502 controls
for replacement while a future is still borrowed remain unchanged.

The 1,056 scenarios contain 6,480 actual generated executions. Four private or
inherited models cross all eight execution entries, ordinary/ID-set/continuous
configuration, scalar/readonly/facet/grouped shapes and aggregation cache on/off.
The principal sequence warms the request twice, denies it, restricts it, allows
it again and advances its offset. Additional list/one/page sequences replace
stores and change membership through actual SQLite writes before explicit clear
and replacement. Public page execution keeps its own offset/limit contract.

The cache wrapper delegates every operation to InMemoryAggregationCache. Literal
keys, ages, hits/misses, puts, full query/Policy Debug, typed JSON, load flags,
computed values, scalars/totals/errors, SELECT callbacks and stable safe SQL
records compare exactly. ID-set and continuous plan observations retain their
actual values, including observations left by previous operations that a later
entry does not change. No generated cursor IDs, absolute expiry times or volatile
execution timestamps are added to this stable boundary.

Source observations include 816 ID_SET_HIT plans, 384 ID_SET_BUILD plans,
192 CURSOR_SEEK plans and 1,412 actual aggregation cache hits. Plan values can
persist through an operation that does not update them; these are not counts of
store callbacks. Eight [physical execution witnesses](parity/hot_execution_physical_witnesses_5_0_6.json.gz)
disable aggregation caching and verify the initial two SQL records versus one on
an ID-set hit, or actual seek SQL after advancing the continuous cursor.
All 912 Policy denials
produce no SELECT, aggregation-cache access or SQL record. The 78 original
`IN requires at least one value` errors remain failures. Source behavior under
stale membership, cache clearing and store replacement is retained rather than
replaced with an invented automatic refresh rule.

The root's sensitive binding redactions remain exact. Generated facets have their
own independent application comment: the original preserves its explicit local
canary text when that text is not a classified child binding. That output is kept
verbatim, not silently masked to satisfy an overbroad observer assertion. Request
intent must not contain secrets. The initial assertion failure and the initial
unordered configuration are retained in the source-preparation artifacts; the
qualified corpus uses explicit ID ordering to exercise real cursor seek.

## Reproduced public lookup-result gap

A read of the complete generated request and runtime adapter bodies exposed a
missing error branch outside the earlier SQL/facet failures. A fresh UserContext
without an installed entity data service must return a generated Graph error.
The [initial paired record](parity/entry_lookup_initial_5_0_6.json.gz) reproduces
all 32 failures: four source models times all eight public execution methods.
Original Rust returns `graph write error: missing entity data service for entity: ...`;
native instead throws a synchronous exception before returning the operation.
Policy, SQL, diagnostics and result decoding are never reached.

Shared EntityQuery now converts failed service lookup into a Context-owned
operation containing the corresponding Graph result. Seven execution bodies use
one generic adapter; executeForFirst retains its executeForOne delegation. Count,
exists and compact rows keep their existing public Entity-result constraints.
Ordinary bindings and intent are still validated before lookup. This repair does
not convert expired Context borrows or invalid construction into business errors.

The failure state owns its error payload and creates no result entity or cursor.
It has no provider work to join. Sixteen native controls construct each operation
without polling, then explicitly cancel or actually delete its UserContext; they
require zero retained User bytes with default PMR allocation disabled. This is a
C++ ownership control, not an invented Rust Context-deletion execution.

The original lazy stream still delivers lookup failure when polled; native
execution returns the error from startup under the previously documented protocol
adaptation. Complete observed error text and all other fields agree. The
[repaired observations](parity/entry_lookup_repaired_5_0_6.json.gz) retain every
source/native output without field or error normalization.

## Verification and remaining requirements

The final 1,056 scenarios and 6,480 executions remain exactly equal after the
lookup repair, including all 5,668 stable SQL diagnostic records and 7,686 Policy
snapshots. The 32 root lookup failures match original results without exceptions
or output normalization. Native integration passes 4/4; a further 3/3 checks
preserve all-eight foreign/expired Context bindings, source/result ownership and
the retained 702-case adapted-model corpus. Imported-module checks pass 4/4.
ASan/UBSan passes 2/2 for the new 32-case error path and 16 cancellation/
Context-deletion controls, with leak scanning disabled. The sanitizer selection
does not represent a replay of the full hot-cache matrix.

Current compiler contracts pass one positive and 24 independent negative cases
across all eight public execution entries. Formatting/check covers 1,639 C++
files; 790 canonical module bodies and 1,495 catalog exports verify. No generator
or generated API changed, so Java/artifact regeneration evidence remains at its
separate unchanged-producer checkpoints.

The complete source/body review closes EX11's twelve slice convenience methods
using the existing 8,448 original observations, explicit fluent-stage adaptations
and proof that their bodies remain unchanged through the runtime repairs. The
whole G09 capability remains partial: configured source/result and assembly
responsibility reconciliation and the full phase exits are not inferred from
this evidence. The earlier compound questions remain archived intact.

## Boundaries and reproduction

The [complete pre-repair headers](parity/entry_lookup_before_5_0_6.json.gz) and
`scripts/lookup_failure_qualification.py` bound the production change to these
seven lookup branches and one error-operation adapter. The marker verifier first
proves that complete later delta, then replays its unchanged historical marker
and stream-guard checkpoints. No producer or generated artifact changes.

The hot-cache observer reuses the canonical composed-query fixture. The fixture's
only change is an explicit final return from main, needed when it is included
under a renamed entry point. All original query, SQL, observation and execution
bodies are unchanged; the complete previous file is archived.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_entry_lookup_probe teaql_hot_execution_probe
ctest --preset native --output-on-failure \
  -R '^(entry_lookup_rust_parity|entry_lookup_context_contracts|hot_execution_rust_parity)$'
```

The [terminal record](parity/execution_hot_cache_lookup_terminal_5_0_6.json.gz)
records current gates, full outputs, source identities and the exact remaining
G09 obligations. Model/source-result configuration reconciliation, complete
assembly and independent G08/G13-G17/P4/P7 exits are not inferred from this
checkpoint. Physical-device and external-provider claims remain outside its scope.
